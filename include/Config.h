/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "Args.h"
#include "hxed_config.h"

#include <stdbool.h>

#define IS_CONFIG false
#define CONFIG_FILE_NAME "hxed.conf"

#ifdef _WIN32
#define CONFIG_FILE_PATH GLOBAL_CONFIG_DIR "\\" CONFIG_FILE_NAME

#else
#define CONFIG_FILE_PATH GLOBAL_CONFIG_DIR "/" CONFIG_FILE_NAME

#endif

void set_config(options *opt);
void print_config(void);
void cleanup_colors(void);

#endif // CONFIG_H