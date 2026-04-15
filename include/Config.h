/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "Args.h"
#include "Utils.h"

#include <stdbool.h>
#include <string.h>

#define IS_CONFIG false

typedef struct {
    bool enable_color;
    bool show_offsets;
    bool show_ascii;
    bool show_header;
    bool show_footer;
    bool show_analysis;
    bool heatmap_enabled;
    char *color_scheme;
} hxed_config;

/*
color scheme in utils reinmachen und dann immer nur rgbs reinparsen in escape sachen
*/

void set_config(options *opt);

#ifdef _WIN32
#include <direct.h>
#define mkdir_p(path) _mkdir(path)
#else
#include <sys/stat.h>
#define mkdir_p(path) mkdir(path, 0755)
#endif

#endif

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