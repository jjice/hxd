/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef ARGS_H
#define ARGS_H

#include <stdlib.h>
#include <stdbool.h>

#define MAX_SEARCH_LEN 32

//static unsigned char search[MAX_SEARCH_LEN] = {0};
//static int search_len = 0; 

// options (no getopt)
typedef struct {
    char *filename;        // Path to the file to be read.
    bool pipeline;         // Flag to determine if a pipeline is active (bypass check_file)
    int output_mode;       // 0 = hex, 1 = binary, 2 = octal, 3 = decimal
    int heatmap;           // Enable and config heatmap view.
    int buff_size;         // The size of the buffer/line to display (e.g., 16 bytes).
    int grouping;          // Grouping size for hex output (e.g., 1 for 1 bytes grouped together). Default is 1.
    bool ascii;            // Flag to determine if the ASCII representation column should be shown.
    bool color;            // Flag to determine if colored output is enabled
    bool string;           // Flag to determine if strings shoud be highlighted
    bool entropie;         // Future options: -e, --entropy to show entropy graph per line
    bool skip_header;      // Flag to determine if header will be skipped
    bool skip_zero;        // Flag to determine if lines with only zero bytes should be skipped
    size_t offset_read;    // Bytes to skip until print
    size_t limit_read;     // Byte to stop reading
    size_t read_size;      // Bytes to read
    unsigned char *search; // Pointer to the parsed search bytes
    size_t search_len;     // Parsed search length in bytes
    bool pager;            // Flag to determine if output should be sent to a pager (e.g., less)
    bool raw;              // Flag to determine if output should be raw
} options;

options *get_options(int argc, char *argv[]);

#endif
