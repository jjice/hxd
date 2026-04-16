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
void cleanup_colors(void);

#endif // CONFIG_H