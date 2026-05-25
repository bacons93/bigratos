#include "util.h"

#include <ctype.h>
#include <stddef.h>

/* Only allow simple package/search names.
 * This avoids path traversal-like input such as '../bad',
 * avoids slash-separated names such as 'bad/name',
 * and rejects names longer than RAT_NAME_MAX.
 */
bool rat_is_valid_simple_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }

    size_t len = 0;

    for (size_t i = 0; name[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)name[i];

        if (len >= RAT_NAME_MAX) {
            return false;
        }

        if (
            isalnum(ch) ||
            ch == '-' ||
            ch == '_' ||
            ch == '.' ||
            ch == '+'
        ) {
            len++;
            continue;
        }

        return false;
    }

    return true;
}
