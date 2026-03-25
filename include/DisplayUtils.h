/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef DISPLAYUTILS_H
#define DISPLAYUTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "Args.h"

typedef struct SearchResults SearchResults;

// Utility functions for rendering the hex dump, calculating line widths, 
// handling color coding, and searching for byte patterns in the input file.
typedef struct {
    FILE *out;
    options *option;
    const SearchResults *search_results;
    size_t addr_display;
    size_t search_match_index;
    int addr_width;
    int visible_columns;
    bool no_newline;
} display_state;

// Search match for a specific byte pattern, 
// used to determine which lines to highlight based on search results.
typedef struct {
    size_t line;
    size_t addr;
} SearchMatch;

// Collection of search matches, including the count of matches found.
struct SearchResults {
    SearchMatch *matches;
    size_t count;
};


#define MAX_BUFF_SIZE 16384
#define MAX_LINE_SIZE 32768

void append_to_line(char *line, size_t line_size, size_t *line_pos, const char *fmt, ...);
void append_header_columns(char *line, size_t line_size, size_t *line_pos, const options *option, int column_count);
void append_magic_summary(char *buffer, size_t buffer_size, size_t *pos);

int _calc_visible_columns(const options *option);
int calc_row_width(const options *option, int addr_width, int column_count);
int count_found_magic(void);
void find_magic_bytes_in_stream_header(FILE *file);
FILE *open_input_file(options *option);
void render_line(display_state *state, int processed, int line_len);
int _hex_digits_size_t(size_t value);

void reset_display_utils_state(void);
unsigned char *get_display_buffer(void);

SearchResults *search_matches(const options *option, FILE *file);
void free_search_results(SearchResults *results);

#endif
