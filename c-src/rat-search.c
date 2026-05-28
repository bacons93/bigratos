#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

#define REPO_URL "https://dists.jewguard.xyz/bigratos/"

static void print_help(void) {
    printf("rat-search - search BigRatOS repository packages\n\n");
    printf("Usage:\n");
    printf("  rat-search <query>\n");
    printf("  rat-search --help\n");
    printf("  rat-search --version\n\n");
    printf("Examples:\n");
    printf("  rat-search bash\n");
    printf("  rat-search gcc\n");
}

static bool extract_package_name(const char *line, char *out, size_t out_size) {
    const char *href = strstr(line, "href=\"");
    if (href == NULL) {
        return false;
    }

    href += 6;

    const char *end = strstr(href, ".tar.xz");
    if (end == NULL || end <= href) {
        return false;
    }

    size_t len = (size_t)(end - href);

    if (len >= out_size) {
        return false;
    }

    memcpy(out, href, len);
    out[len] = '\0';
    return true;
}

static FILE *open_repo_stream(pid_t *child_pid) {
    int fds[2];

    if (pipe(fds) != 0) {
        fprintf(stderr, "error: failed to create pipe\n");
        return NULL;
    }

    pid_t pid = fork();

    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        fprintf(stderr, "error: failed to fork curl process\n");
        return NULL;
    }

    if (pid == 0) {
        close(fds[0]);

        if (dup2(fds[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }

        close(fds[1]);

        execlp("curl", "curl", "-fsSL", REPO_URL, (char *)NULL);
        _exit(127);
    }

    close(fds[1]);

    FILE *stream = fdopen(fds[0], "r");
    if (stream == NULL) {
        close(fds[0]);
        fprintf(stderr, "error: failed to open curl output stream\n");
        return NULL;
    }

    *child_pid = pid;
    return stream;
}

static int wait_for_curl(pid_t pid) {
    int status;

    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "error: failed to wait for curl\n");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "error: failed to fetch repository index\n");
        return 1;
    }

    return 0;
}

static int search_repo(const char *query) {
    if (!rat_is_valid_simple_name(query)) {
        fprintf(stderr, "error: invalid search query '%s'\n", query);
        return 1;
    }

    pid_t curl_pid;
    FILE *stream = open_repo_stream(&curl_pid);

    if (stream == NULL) {
        return 1;
    }

    char line[2048];
    char pkg[512];
    bool found = false;

    while (fgets(line, sizeof(line), stream) != NULL) {
        if (!extract_package_name(line, pkg, sizeof(pkg))) {
            continue;
        }

        if (strstr(pkg, query) != NULL) {
            printf("%s\n", pkg);
            found = true;
        }
    }

    fclose(stream);

    if (wait_for_curl(curl_pid) != 0) {
        return 1;
    }

    if (!found) {
        printf(">> no results for '%s'\n", query);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: rat-search <query>\n");
        fprintf(stderr, "try: rat-search --help\n");
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("rat-search %s\n", RAT_VERSION);
        return 0;
    }

    return search_repo(argv[1]);
}
