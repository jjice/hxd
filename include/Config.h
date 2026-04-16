/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "Args.h"

#include <stdbool.h>
#include <string.h>

#define IS_CONFIG false

#define CONFIG_FILE_NAME "hxed.conf"

/*
color scheme TODO
*/

void set_config(options *opt);

static inline void trim(char *str) {
    // Trim leading whitespace
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r'))
        str++;

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;

    // Null-terminate the trimmed string
    *(end + 1) = '\0';
}

#endif // CONFIG_H