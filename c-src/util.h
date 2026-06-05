#ifndef RAT_UTIL_H
#define RAT_UTIL_H

#include <stdbool.h>


#define DB_DIR "/usr/ports/db"
#define INSTALLED_FILE "/usr/ports/db/installed"
#define REPO_URL "https://dists.jewguard.xyz/bigratos/"
#define REPO_X64_RUNIT_URL "https://dists.jewguard.xyz/bigratos/x64-runit/"

#define RAT_NAME_MAX 255
// update version here!!
#define RAT_VERSION "0.1-dev"

bool rat_is_valid_simple_name(const char *name);

#endif
