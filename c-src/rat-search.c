#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RAT_SEARCH_VERSION "0.1-dev"
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

static int search_repo(const char *query) {
    FILE *pipe = popen("curl -fsSL " REPO_URL, "r");

    if (pipe == NULL) {
        fprintf(stderr, "error: failed to run curl\n");
        return 1;
    }

    char line[2048];
    char pkg[512];
    bool found = false;

    while (fgets(line, sizeof(line), pipe) != NULL) {
        if (!extract_package_name(line, pkg, sizeof(pkg))) {
            continue;
        }

        if (strstr(pkg, query) != NULL) {
            printf("%s\n", pkg);
            found = true;
        }
    }

    int status = pclose(pipe);

    if (status != 0) {
        fprintf(stderr, "error: failed to fetch repository index\n");
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
        printf("rat-search %s\n", RAT_SEARCH_VERSION);
        return 0;
    }

    return search_repo(argv[1]);
}
