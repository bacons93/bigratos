#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RAT_INFO_VERSION "0.1-dev"
#define DB_DIR "/usr/ports/db"
#define INSTALLED_FILE "/usr/ports/db/installed"

static void print_help(void) {
    printf("rat-info - show BigRatOS package information\n\n");
    printf("Usage:\n");
    printf("  rat-info\n");
    printf("  rat-info <pkg>\n");
    printf("  rat-info --help\n");
    printf("  rat-info --version\n\n");
    printf("Examples:\n");
    printf("  rat-info\n");
    printf("  rat-info bash\n");
}

static int print_file_or_none(const char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("none\n");
        return 1;
    }

    int ch;
    bool empty = true;

    while ((ch = fgetc(file)) != EOF) {
        empty = false;
        putchar(ch);
    }

    if (ferror(file)) {
        fclose(file);
        fprintf(stderr, "error: failed while reading '%s'\n", path);
        return 1;
    }

    if (empty) {
        printf("none\n");
    }

    fclose(file);
    return 0;
}

static bool package_is_installed(const char *pkg) {
    FILE *file = fopen(INSTALLED_FILE, "r");

    if (file == NULL) {
        return false;
    }

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, pkg) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

static int show_installed_packages(void) {
    printf(">> Installed packages:\n");
    return print_file_or_none(INSTALLED_FILE);
}

static int show_package_info(const char *pkg) {
    if (!package_is_installed(pkg)) {
        printf(">> %s is not installed\n", pkg);
        return 1;
    }

    printf(">> Package: %s\n", pkg);
    printf(">> Status: installed\n");
    printf(">> Files:\n");

    char files_path[512];
    int written = snprintf(files_path, sizeof(files_path), "%s/%s.files", DB_DIR, pkg);

    if (written < 0 || written >= (int)sizeof(files_path)) {
        fprintf(stderr, "error: package name/path is too long\n");
        return 1;
    }

    print_file_or_none(files_path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return show_installed_packages();
    }

    if (argc != 2) {
        fprintf(stderr, "usage: rat-info [pkg]\n");
        fprintf(stderr, "try: rat-info --help\n");
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("rat-info %s\n", RAT_INFO_VERSION);
        return 0;
    }

    return show_package_info(argv[1]);
}
