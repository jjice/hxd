/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

/* Implementation of the display functions for the hex dumper 
 * How it works:
 * - Reads the input file in chunks defined by buff_size.
 * - For each chunk, it constructs a formatted line of output based on the selected output mode (hex, octal, binary, decimal).
 * - Applies coloring based on heatmap, string, or color options.
 * - At the end of the dump, it prints a footer with a summary of the analysis and metadata.
 */

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Display.h"
#include "DisplayUtils.h"
#include "Args.h"
#include "File.h"
#include "Utils.h"

// Print the header with file information and current dump settings.
static void print_header(FILE *out, options *option, int addr_width) {
    const char *src = option->pipeline ? "<pipe>" : option->filename;
    int column_count = _calc_visible_columns(option);
    int row_width = calc_row_width(option, addr_width, column_count);
    char header_line[MAX_LINE_SIZE] = {0};
    size_t header_pos = 0;

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "\ndump for %s:\n", src);

    append_to_line(header_line, sizeof(header_line), &header_pos, "%-*s | ", addr_width, "offset");

    append_header_columns(header_line, sizeof(header_line), &header_pos, option, column_count);

    if (option->ascii) {
        append_to_line(header_line, sizeof(header_line), &header_pos, "| ascii");
        while ((int)header_pos < row_width) {
            append_to_line(header_line, sizeof(header_line), &header_pos, " ");
        }
    }

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fputs(header_line, out);
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);

    if (option->color) fprintf(out, "%s", BORDER_COLOR);
    for (int i = 0; i < row_width; i++) fputc('-', out);
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);
}


// Print compact footer with analysis and metadata.
static void print_footer(FILE *out, options *option, int addr_width, const dump_analysis *analysis) {
    int column_count = _calc_visible_columns(option);
    int row_width = calc_row_width(option, addr_width, column_count);
    file_metadata meta = {0};
    char created_at[32] = "n/a";
    char modified_at[32] = "n/a";
    char magic_line[1024] = {0};
    size_t magic_pos = 0;
    size_t shown_size = analysis->total_bytes;
    double zero_pct = 0.0;

    if (!option->pipeline) {
        get_file_metadata(option->filename, &meta);
        if (meta.exists) shown_size = meta.file_size;
        if (meta.has_created) format_time_local(meta.created_at, created_at, sizeof(created_at));
        if (meta.has_modified) format_time_local(meta.modified_at, modified_at, sizeof(modified_at));
    }

    if (analysis->total_bytes > 0) {
        zero_pct = ((double)analysis->zero_bytes / (double)analysis->total_bytes) * 100.0;
    }

    append_magic_summary(magic_line, sizeof(magic_line), &magic_pos);

    if (option->color) fprintf(out, "%s", BORDER_COLOR);
    for (int i = 0; i < row_width; i++) fputc('-', out);
    
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);

    // ------ Analysis summary line ------

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "summary");

    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "%s  |%s size %zu B ; zero %zu (%.1f%%) ; magic %d\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            shown_size,
            analysis->zero_bytes,
            zero_pct,            
            analysis->magic_count);

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "stats");

    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "%s    |%s printable %.1f%% ; control %.1f%% ; high-byte %.1f%% ; whitespace %.1f%%\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            analysis->printable,
            analysis->control,
            analysis->high_byte,
            analysis->whitespace);

    if (analysis->magic_count > 0) {
        if (option->color) fprintf(out, "%s", HEADER_COLOR);
        fprintf(out, "magic");
        
        if (option->color) fprintf(out, "%s", RESET);
        fprintf(out, "%s    |%s %s\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            magic_line);
    }

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "view");

    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "%s     |%s width %d ; offset %zu ; lines %zu ; ",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            option->buff_size,
            option->offset_read,
            analysis->line_count);

    if (option->read_size != 0) fprintf(out, "read %zu", option->read_size);
    else fprintf(out, "limit eof");
    fputc('\n', out);

    if (!option->pipeline) {
        if (option->color) fprintf(out, "%s", HEADER_COLOR);
        fprintf(out, "time");
        
        if (option->color) fprintf(out, "%s", RESET);
        fprintf(out, "%s     |%s created %s ; modified %s\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            created_at,
            modified_at);
    }
}

// Main function to print the hex dump based on the provided options
void print_hex(options *option) {
    // Open pager if requested, otherwise use stdout
    FILE *out = stdout;
    unsigned char *display_buffer = get_display_buffer();
    SearchResults *search_results = NULL;

    bool no_newline = false;
    if (option->buff_size == 0) {
        no_newline = true;
        option->buff_size = 1024;
    }

    if (option->search_len > 0 && option->pipeline) {
        fprintf(stderr, "Search is currently only supported for files, not stdin\n");
        exit(EXIT_FAILURE);
    }

    // Reset line/search carry-over state before rendering.
    reset_display_utils_state();

    display_state state = {0};
    dump_analysis analysis = {0};
    state.out = out;
    state.option = option;
    state.search_results = NULL;
    state.addr_display = option->offset_read;
    state.search_match_index = 0;
    state.addr_width = 8;
    state.visible_columns = _calc_visible_columns(option);
    state._max = 255;
    state._min = 0;
    state.no_newline = no_newline;

    // Calculates dynamic address width to keep header and rows aligned.
    size_t max_addr = option->offset_read;
    
    if (option->read_size != 0) max_addr = option->offset_read + option->read_size;
    else if (option->limit_read != 0) max_addr = option->limit_read;
    
    int needed_digits = _hex_digits_size_t(max_addr);
    if (needed_digits > state.addr_width) state.addr_width = needed_digits;

    FILE *file = open_input_file(option);

    if (option->search_len > 0) {
        search_results = search_matches(option, file);
        state.search_results = search_results;
    }

    if (option->search_len != 0 &&state.search_results->count == 0) {
        if(option->color && option->search_len > 0) fprintf(stderr, "\n%sNo matches found for search string%s\n", ERROR_COLOR, RESET);
        else printf("No matches found for search string\n");
        free(search_results);
        exit(EXIT_SUCCESS);
    }

    if (option->pager) {
        out = open_pager();
        state.out = out;
    }

    if (option->heatmap == 1) {
        find_extrema(&state._max, &state._min, file);
    }

    // Search for magic byte signatures in the file header if not skipped
    if (!option->skip_header) find_magic_bytes_in_stream_header(file);
    
    // Print header if not in raw mode.
    if (!option->raw && !option->skip_header) print_header(out, option, state.addr_width);

    int bytes_read = 0; // Bytes read in last read_file_to_buffer call.
    analysis.magic_count = count_found_magic();

    // --- Hex Output Loop ---
    while (1) {
        size_t limit = 0;

        if (option->read_size != 0) limit = option->offset_read + option->read_size;
        else if (option->limit_read != 0) limit = option->limit_read;

        read_stream_to_buffer(&bytes_read, file, option->offset_read, limit, display_buffer, false);
        if (bytes_read == 0) break;

        int processed = 0;

        // Init Analysis for this chunk
        analyse(&analysis, display_buffer, (size_t)bytes_read);

        // Process the buffer in lines of buff_size, ensuring we don't exceed bytes_read.
        while (processed < bytes_read) {
            int line_len = option->buff_size;
            if (processed + line_len > bytes_read) {
                line_len = bytes_read - processed;
            }

            for (int i = 0; i < line_len; i++) {
                if (display_buffer[processed + i] == 0) analysis.zero_bytes++;
            }

            analysis.total_bytes += (size_t)line_len;
            analysis.line_count++;

            render_line(&state, processed, line_len);

            state.addr_display += (size_t)line_len;
            processed += line_len;
        }
    }

    if (!option->raw && !option->skip_header) print_footer(out, option, state.addr_width, &analysis);
    if (!option->raw && !option->skip_header) fprintf(out, "\n");

    // Closes the file handle opened at the beginning of print_hex.
    if (!option->pipeline) fclose(file);
    if (option->pager) pclose(out);
    free_search_results(search_results);
}
