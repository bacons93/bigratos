#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

#define PORTS_DIR "/usr/ports/tmp"
#define RAT_NAME_MAX 255
#define MAX_RETRIES 10

static char rat_tmpfile[512];
static char workdir[512];
static char rat_before[512];
static char rat_after[512];

static void cleanup(void) {
    char cmd[1200];

    if (rat_tmpfile[0] != '\0') {
        remove(rat_tmpfile);
    }

    if (workdir[0] != '\0') {
        snprintf(cmd, sizeof(cmd), "rm -rf -- '%s'", workdir);
        system(cmd);
    }

    if (rat_before[0] != '\0') {
        remove(rat_before);
    }

    if (rat_after[0] != '\0') {
        remove(rat_after);
    }
}

static void handle_signal(int sig) {
    cleanup();
    if (sig == SIGINT) {
        exit(130);
    }
    if (sig == SIGTERM) {
        exit(143);
    }
    exit(1);
}

static void fail(const char *msg) {
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

static int is_valid_name(const char *s) {
    size_t i;
    size_t len;

    if (s == NULL || s[0] == '\0') {
        return 0;
    }

    len = strlen(s);
    if (len > RAT_NAME_MAX) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];

        if (!(isalnum(c) || c == '.' || c == '_' || c == '+' || c == '-')) {
            return 0;
        }
    }

    return 1;
}

static void lower_string(char *s) {
    size_t i;

    for (i = 0; s[i] != '\0'; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

static void trim_newline(char *s) {
    size_t len = strlen(s);

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static char *capture_cmd(const char *cmd) {
    FILE *fp;
    char buf[4096];
    char *out = NULL;
    size_t used = 0;
    size_t cap = 0;

    fp = popen(cmd, "r");
    if (fp == NULL) {
        return NULL;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        size_t n = strlen(buf);

        if (used + n + 1 > cap) {
            size_t new_cap = cap == 0 ? 8192 : cap * 2;
            char *tmp;

            while (used + n + 1 > new_cap) {
                new_cap *= 2;
            }

            tmp = realloc(out, new_cap);
            if (tmp == NULL) {
                free(out);
                pclose(fp);
                return NULL;
            }

            out = tmp;
            cap = new_cap;
        }

        memcpy(out + used, buf, n);
        used += n;
        out[used] = '\0';
    }

    pclose(fp);

    if (out == NULL) {
        out = calloc(1, 1);
    }

    return out;
}

static int run_cmd(const char *cmd) {
    int status = system(cmd);

    if (status == -1) {
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
}

static void check_dep(const char *path, const char *msg) {
    if (access(path, X_OK) != 0) {
        fail(msg);
    }
}

static const char *cmd_to_pkg(const char *cmd) {
    struct map {
        const char *cmd;
        const char *pkg;
    };

    static const struct map maps[] = {
        {"autoreconf", "autoconf"},
        {"autoheader", "autoconf"},
        {"autom4te", "autoconf"},
        {"autoscan", "autoconf"},
        {"autoupdate", "autoconf"},
        {"ifnames", "autoconf"},
        {"automake", "automake"},
        {"aclocal", "automake"},
        {"libtoolize", "libtool"},
        {"libtool", "libtool"},
        {"ltmain", "libtool"},
        {"pkg-config", "pkgconf"},
        {"pkgconf", "pkgconf"},
        {"cmake", "cmake"},
        {"ninja", "ninja"},
        {"ninja-build", "ninja"},
        {"meson", "meson"},
        {"python3", "python3"},
        {"python", "python3"},
        {"perl", "perl"},
        {"flex", "flex"},
        {"lex", "flex"},
        {"bison", "bison"},
        {"yacc", "bison"},
        {"m4", "m4"},
        {"gzip", "gzip"},
        {"xz", "xz-utils"},
        {NULL, NULL}
    };

    size_t i;

    for (i = 0; maps[i].cmd != NULL; i++) {
        if (strcmp(cmd, maps[i].cmd) == 0) {
            return maps[i].pkg;
        }
    }

    return cmd;
}

static char *resolve_pkg(const char *pkg) {
    char cmd[2048];
    char *resolved;

    snprintf(cmd, sizeof(cmd),
             "curl -fsSL '%s' 2>/dev/null "
             "| grep -o 'href=\"%s[^\"]*\\.tar\\.xz\"' "
             "| sed 's/href=\"//;s/\\.tar\\.xz\"//' "
             "| sort -V | tail -1",
             REPO_X64_RUNIT_URL, pkg);

    resolved = capture_cmd(cmd);
    if (resolved == NULL) {
        return NULL;
    }

    trim_newline(resolved);

    if (resolved[0] == '\0') {
        free(resolved);
        return NULL;
    }

    if (!is_valid_name(resolved)) {
        free(resolved);
        return NULL;
    }

    return resolved;
}

static char *resolve_dep_pkg(const char *pkg) {
    char *resolved;
    char nolib[RAT_NAME_MAX + 1];

    resolved = resolve_pkg(pkg);
    if (resolved != NULL) {
        return resolved;
    }

    if (strncmp(pkg, "lib", 3) == 0 && strlen(pkg) > 3) {
        snprintf(nolib, sizeof(nolib), "%s", pkg + 3);
        return resolve_pkg(nolib);
    }

    return NULL;
}

static int self_install_dep(const char *argv0, const char *dep) {
    pid_t pid = fork();

    if (pid < 0) {
        return 1;
    }

    if (pid == 0) {
        execl(argv0, argv0, dep, (char *)NULL);
        _exit(127);
    }

    for (;;) {
        int status;

        if (waitpid(pid, &status, 0) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }

        return 1;
    }
}

static char *detect_missing_dep(const char *log_path) {
    char cmd[4096];
    char *raw;

    snprintf(cmd, sizeof(cmd),
        "grep -oE "
        "\"Can't exec \\\\\\\"([a-zA-Z0-9_-]+)\\\\\\\"|"
        "([a-zA-Z0-9_-]+): command not found|"
        "failed to run ([a-zA-Z0-9_-]+)|"
        "not found[: ]+([a-zA-Z0-9_-]+)|"
        "cannot find -l([a-zA-Z0-9_-]+)|"
        "No package '([a-zA-Z0-9_-]+)' found|"
        "Could not find ([a-zA-Z0-9_-]+)|"
        "fatal error: ([a-zA-Z0-9_-]+)\\\\.h|"
        "Package ([a-zA-Z0-9_-]+) was not found|"
        "([a-zA-Z0-9_-]+) is required|"
        "missing: ([a-zA-Z0-9_-]+)\" '%s' "
        "| grep -oE '[a-zA-Z0-9][a-zA-Z0-9_-]+' "
        "| grep -v '^error$'   | grep -v '^fatal$' "
        "| grep -v '^not$'     | grep -v '^found$' "
        "| grep -v '^find$'    | grep -v '^could$' "
        "| grep -v '^package$' | grep -v '^command$' "
        "| grep -v '^failed$'  | grep -v '^run$' "
        "| grep -v '^exec$'    | grep -v '^Can$' "
        "| head -1",
        log_path);

    raw = capture_cmd(cmd);
    if (raw == NULL) {
        return NULL;
    }

    trim_newline(raw);

    if (raw[0] == '\0') {
        free(raw);
        return NULL;
    }

    if (!is_valid_name(raw)) {
        free(raw);
        return NULL;
    }

    return raw;
}

int main(int argc, char **argv) {
    char pkg[RAT_NAME_MAX + 1];
    char stack[4096];
    char new_stack[4096];
    char *resolved;
    char url[1024];
    char cmd[2048];
    char build_log_path[512];
    char installed_deps[4096] = "";
    int attempt;
    int build_ok = 0;
    pid_t pid = getpid();

    rat_tmpfile[0] = '\0';
    workdir[0] = '\0';
    rat_before[0] = '\0';
    rat_after[0] = '\0';

    atexit(cleanup);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc < 2 || argv[1][0] == '\0') {
        printf("usage: rat-add <pkg>\n");
        return 1;
    }

    if (strlen(argv[1]) > RAT_NAME_MAX) {
        char msg[320];
        snprintf(msg, sizeof(msg), "invalid package name '%s'", argv[1]);
        fail(msg);
    }

    snprintf(pkg, sizeof(pkg), "%s", argv[1]);
    lower_string(pkg);

    if (!is_valid_name(pkg)) {
        char msg[320];
        snprintf(msg, sizeof(msg), "invalid package name '%s'", pkg);
        fail(msg);
    }

    snprintf(stack, sizeof(stack), " %s ", getenv("RAT_INSTALL_STACK") ? getenv("RAT_INSTALL_STACK") : "");

    if (strstr(stack, pkg) != NULL) {
        char msg[320];
        snprintf(msg, sizeof(msg), "dependency cycle detected while installing '%s'", pkg);
        fail(msg);
    }

    snprintf(new_stack, sizeof(new_stack), "%s %s ", getenv("RAT_INSTALL_STACK") ? getenv("RAT_INSTALL_STACK") : "", pkg);
    setenv("RAT_INSTALL_STACK", new_stack, 1);

    printf(">> RAT-CONFTEST stage started\n");

    {
        char *arch = capture_cmd("uname -m");
        char *gcc_ver = capture_cmd("gcc -dumpfullversion 2>/dev/null");

        if (arch != NULL) {
            trim_newline(arch);
        }
        if (gcc_ver != NULL) {
            trim_newline(gcc_ver);
        }

        printf("[1/4] GCC-VERS: %s\n", (gcc_ver && gcc_ver[0]) ? gcc_ver : "UNKNOWN");
        printf("[2/4] ARCHITECTURE: %s\n", (arch && arch[0]) ? arch : "UNKNOWN");

        free(arch);
        free(gcc_ver);
    }

    check_dep("/bin/sh", "SH (core script dependency) is not executable");
    printf("[3/4] IS-SH-AVAILABLE: YES\n");

    check_dep("/bin/curl", "CURL (core script dependency) is not executable");
    printf("[4/4] IS-CURL-AVAILABLE: YES\n");

    printf(">> RAT-CONFTEST stage completed\n");

    printf(">> resolving package name...\n");
    resolved = resolve_pkg(pkg);
    if (resolved == NULL) {
        char msg[320];
        snprintf(msg, sizeof(msg), "package '%s' not found in repo", pkg);
        fail(msg);
    }

    printf(">> resolved: %s\n", resolved);

    snprintf(url, sizeof(url), "%s%s.tar.xz", REPO_X64_RUNIT_URL, resolved);
    snprintf(rat_tmpfile, sizeof(rat_tmpfile), "%s/%s.tar.xz", PORTS_DIR, resolved);
    snprintf(workdir, sizeof(workdir), "%s/%s", PORTS_DIR, resolved);
    snprintf(rat_before, sizeof(rat_before), "%s/rat-before-%ld.txt", PORTS_DIR, (long)pid);
    snprintf(rat_after, sizeof(rat_after), "%s/rat-after-%ld.txt", PORTS_DIR, (long)pid);
    snprintf(build_log_path, sizeof(build_log_path), "%s/rat-build-%ld.log", PORTS_DIR, (long)pid);

    free(resolved);

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s' '%s'", PORTS_DIR, DB_DIR);
    if (run_cmd(cmd) != 0) {
        fail("failed to create ports directory");
    }

    printf(">> fetching %s ...\n", pkg);
    snprintf(cmd, sizeof(cmd), "curl -fL '%s' -o '%s'", url, rat_tmpfile);
    if (run_cmd(cmd) != 0) {
        char msg[320];
        snprintf(msg, sizeof(msg), "failed to fetch %s", pkg);
        fail(msg);
    }

    printf(">> extracting\n");
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", workdir);
    if (run_cmd(cmd) != 0) {
        fail("failed to create work directory");
    }

    snprintf(cmd, sizeof(cmd), "tar -xJf '%s' --strip-components=1 -C '%s'", rat_tmpfile, workdir);
    if (run_cmd(cmd) != 0) {
        fail("failed to extract archive");
    }

    printf(">> compiling / installing\n");

    if (chdir(workdir) != 0) {
        fail("failed to enter work directory");
    }

    snprintf(cmd, sizeof(cmd),
             "find / -not -path '/proc/*' -not -path '/sys/*' -not -path '/usr/ports/*' "
             "-not -path '/dev/*' -not -path '/tmp/*' -type f 2>/dev/null | sort > '%s'",
             rat_before);

    if (run_cmd(cmd) != 0) {
        fail("failed to create pre-install file snapshot");
    }

    for (attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        int build_exit;
        char *build_log;

        printf(">> build attempt %d...\n", attempt);

        snprintf(cmd, sizeof(cmd), "sh -e ratbuild.sh > '%s' 2>&1", build_log_path);
        build_exit = run_cmd(cmd);

        build_log = capture_cmd((snprintf(cmd, sizeof(cmd), "cat '%s'", build_log_path), cmd));
        if (build_log != NULL) {
            fputs(build_log, stdout);
            free(build_log);
        }

        if (build_exit == 0) {
            printf(">> build succeeded\n");
            build_ok = 1;
            break;
        }

        printf(">> build failed, scanning for missing dependencies...\n");

        {
            char *raw_dep = detect_missing_dep(build_log_path);
            const char *mapped_dep;
            char dep_buf[RAT_NAME_MAX + 1];
            char *dep_resolved;

            if (raw_dep == NULL) {
                printf(">> could not identify missing dependency from build log\n");
                fail("build/install failed (unknown dependency)");
            }

            mapped_dep = cmd_to_pkg(raw_dep);
            snprintf(dep_buf, sizeof(dep_buf), "%s", mapped_dep);
            lower_string(dep_buf);

            printf(">> detected missing dependency: %s -> package: %s\n", raw_dep, dep_buf);
            free(raw_dep);

            if (strlen(installed_deps) + strlen(dep_buf) + 3 >= sizeof(installed_deps)) {
    fail("installed dependency list is too long");
}
{
                char dep_marker[RAT_NAME_MAX + 3];

                snprintf(dep_marker, sizeof(dep_marker), " %s ", dep_buf);

                if (strstr(installed_deps, dep_marker) != NULL) {
                    char msg[320];
                    snprintf(msg, sizeof(msg), "already tried installing '%s', still failing. giving up.", dep_buf);
                    fail(msg);
                }
            }

            dep_resolved = resolve_dep_pkg(dep_buf);
            if (dep_resolved == NULL) {
                char msg[320];
                snprintf(msg, sizeof(msg), "missing dependency '%s' not found in repo. cannot continue.", dep_buf);
                fail(msg);
            }

            printf(">> installing dependency: %s\n", dep_resolved);
            free(dep_resolved);

            if (self_install_dep(argv[0], dep_buf) != 0) {
                char msg[320];
                snprintf(msg, sizeof(msg), "failed to install dependency '%s'", dep_buf);
                fail(msg);
            }

            strncat(installed_deps, " ", sizeof(installed_deps) - strlen(installed_deps) - 1);
            strncat(installed_deps, dep_buf, sizeof(installed_deps) - strlen(installed_deps) - 1);
            strncat(installed_deps, " ", sizeof(installed_deps) - strlen(installed_deps) - 1);

            printf(">> retrying build of %s...\n", pkg);

            if (chdir(workdir) != 0) {
                fail("failed to re-enter work directory");
            }
        }
    }

    if (!build_ok) {
        char msg[128];
        snprintf(msg, sizeof(msg), "reached max retries (%d), giving up", MAX_RETRIES);
        fail(msg);
    }

    snprintf(cmd, sizeof(cmd),
             "find / -not -path '/proc/*' -not -path '/sys/*' -not -path '/usr/ports/*' "
             "-not -path '/dev/*' -not -path '/tmp/*' -type f 2>/dev/null | sort > '%s'",
             rat_after);

    if (run_cmd(cmd) != 0) {
        fail("failed to create post-install file snapshot");
    }

    snprintf(cmd, sizeof(cmd), "comm -13 '%s' '%s' > '%s/%s.files'", rat_before, rat_after, DB_DIR, pkg);
    if (run_cmd(cmd) != 0) {
        fail("failed to write installed file list");
    }

    snprintf(cmd, sizeof(cmd), "grep -qxF '%s' '%s' 2>/dev/null || echo '%s' >> '%s'", pkg, INSTALLED_FILE, pkg, INSTALLED_FILE);
    if (run_cmd(cmd) != 0) {
        fail("failed to update installed database");
    }

    printf(">> done.\n");

    remove(build_log_path);
    return 0;
}
