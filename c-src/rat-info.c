#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DB_DIR "/usr/ports/db"
#define INSTALLED_FILE "/usr/ports/db/installed"

static void print_file(const char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("none\n");
        return;
    }

    int ch;
    bool empty = true;

    while ((ch = fgetc(file)) != EOF) {
        empty = false;
        putchar(ch);
    }

    if (empty) {
        printf("none\n");
    }

    fclose(file);
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

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf(">> Installed packages:\n");
        print_file(INSTALLED_FILE);
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "usage: rat-info [pkg]\n");
        return 1;
    }

    const char *pkg = argv[1];

    if (!package_is_installed(pkg)) {
        printf(">> %s is not installed\n", pkg);
        return 1;
    }

    printf(">> Package: %s\n", pkg);
    printf(">> Status: installed\n");
    printf(">> Files:\n");

    char files_path[512];
    snprintf(files_path, sizeof(files_path), "%s/%s.files", DB_DIR, pkg);

    print_file(files_path);

    return 0;
}
