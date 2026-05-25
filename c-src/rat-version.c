#include <stdio.h>
#include <string.h>

#define RAT_VERSION "0.1-dev"

static void print_help(void) {
    printf("rat-version - show BigRatOS rat package tools version\n\n");
    printf("Usage:\n");
    printf("  rat-version\n");
    printf("  rat-version --version\n");
    printf("  rat-version --help\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("rat package tools %s\n", RAT_VERSION);
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "usage: rat-version [--version|--help]\n");
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("rat package tools %s\n", RAT_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    fprintf(stderr, "error: unknown option '%s'\n", argv[1]);
    fprintf(stderr, "try: rat-version --help\n");
    return 1;
}
