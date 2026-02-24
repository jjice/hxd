/*
 * hxd - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef ARGS_H
#define ARGS_H

#include <stdlib.h>
#include <stdbool.h>

// options (no getopt)
typedef struct {
    char *filename;     // Path to the file to be read.
    int buff_size;      // The size of the buffer/line to display (e.g., 16 bytes).
    int heatmap;        // Enable and config heatmap view.
    bool ascii;         // Flag to determine if the ASCII representation column should be shown.
    size_t offset_read; // Bytes to skip until print
    size_t limit_read;  // Count of bytes to be read/displayed
    bool pipeline;      // Flag to determine if a pipeline is active (bypass check_file)
    bool color;         // Flag to determine if colored output is enabled
    bool string;        // Flag to determine if strings shoud be highlighted
    bool pager;         // Flag to determine if output should be sent to a pager (e.g., less)
    bool entropie;      // Future options: -e, --entropy to show entropy graph per line
} options;

options *get_options(int argc, char *argv[]);

#endif