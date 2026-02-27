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
    bool raw;           // Flag to determine if output should be raw
    bool search_ascii;  // Flag to determine if search for ascii string is active
    bool search_hex;    // Flag to determine if search for hex string is active      
    bool skip_header;   // Flag to determine if header will be skipped
    unsigned char *search; // Pointer to the search string (either ASCII or hex, depending on flags)
} options;

options *get_options(int argc, char *argv[]);

#endif