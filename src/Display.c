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
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Display.h"
#include "DisplayUtils.h"
#include "Args.h"
#include "File.h"
#include "Utils.h"

typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} decoded_bytes;

static inline bool is_reverse_separator(int c) {
    return isspace((unsigned char)c) || c == ',' || c == ';' || c == ':' || c == '|';
}

static inline int hex_value(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static void push_decoded_byte(decoded_bytes *decoded, unsigned char value) {
    if (decoded->len == decoded->cap) {
        size_t new_cap = decoded->cap == 0 ? 4096 : decoded->cap * 2;
        unsigned char *tmp = realloc(decoded->data, new_cap);
        if (!tmp) {
            perror("Malloc failed");
            free(decoded->data);
            exit(EXIT_FAILURE);
        }
        decoded->data = tmp;
        decoded->cap = new_cap;
    }

    decoded->data[decoded->len++] = value;
}

static void parse_oct_token(const char *token, size_t token_len, decoded_bytes *decoded) {
    if (token_len == 0) return;

    for (size_t i = 0; i < token_len; i++) {
        if (token[i] < '0' || token[i] > '7') {
            fprintf(stderr, "Reverse oct parse error: invalid digit <%c>\n", token[i]);
            free(decoded->data);
            exit(EXIT_FAILURE);
        }
    }

    if (token_len > 3 && (token_len % 3) == 0) {
        for (size_t i = 0; i < token_len; i += 3) {
            char part[4] = {token[i], token[i + 1], token[i + 2], '\0'};
            unsigned long value = strtoul(part, NULL, 8);
            if (value > 255) {
                fprintf(stderr, "Reverse oct parse error: value out of range <%s>\n", part);
                free(decoded->data);
                exit(EXIT_FAILURE);
            }
            push_decoded_byte(decoded, (unsigned char)value);
        }
        return;
    }

    if (token_len > 3) {
        fprintf(stderr, "Reverse oct parse error: use separators or 3-digit groups\n");
        free(decoded->data);
        exit(EXIT_FAILURE);
    }

    {
        char part[4] = {0};
        memcpy(part, token, token_len);
        unsigned long value = strtoul(part, NULL, 8);
        if (value > 255) {
            fprintf(stderr, "Reverse oct parse error: value out of range <%s>\n", part);
            free(decoded->data);
            exit(EXIT_FAILURE);
        }
        push_decoded_byte(decoded, (unsigned char)value);
    }
}

static void parse_dec_token(const char *token, size_t token_len, decoded_bytes *decoded) {
    if (token_len == 0) return;

    for (size_t i = 0; i < token_len; i++) {
        if (!isdigit((unsigned char)token[i])) {
            fprintf(stderr, "Reverse dec parse error: invalid digit <%c>\n", token[i]);
            free(decoded->data);
            exit(EXIT_FAILURE);
        }
    }

    if (token_len > 3) {
        fprintf(stderr, "Reverse dec parse error: use separators between byte values\n");
        free(decoded->data);
        exit(EXIT_FAILURE);
    }

    {
        char part[8] = {0};
        memcpy(part, token, token_len);
        unsigned long value = strtoul(part, NULL, 10);
        if (value > 255) {
            fprintf(stderr, "Reverse dec parse error: value out of range <%s>\n", part);
            free(decoded->data);
            exit(EXIT_FAILURE);
        }
        push_decoded_byte(decoded, (unsigned char)value);
    }
}

static decoded_bytes decode_reverse_stream(FILE *file, const options *option) {
    decoded_bytes decoded = {0};

    if (option->output_mode == 0) {
        int c;
        int hi = -1;

        while ((c = fgetc(file)) != EOF) {
            if (is_reverse_separator(c)) {
                continue;
            }

            int v = hex_value(c);
            if (v < 0) {
                fprintf(stderr, "Reverse hex parse error: invalid character <%c>\n", (char)c);
                free(decoded.data);
                exit(EXIT_FAILURE);
            }

            if (hi < 0) hi = v;
            else {
                push_decoded_byte(&decoded, (unsigned char)((hi << 4) | v));
                hi = -1;
            }
        }

        if (hi >= 0) {
            fprintf(stderr, "Reverse hex parse error: odd number of hex digits\n");
            free(decoded.data);
            exit(EXIT_FAILURE);
        }

        return decoded;
    }

    if (option->output_mode == 1) {
        int c;
        int bits = 0;
        unsigned int value = 0;

        while ((c = fgetc(file)) != EOF) {
            if (is_reverse_separator(c)) {
                continue;
            }

            if (c != '0' && c != '1') {
                fprintf(stderr, "Reverse bin parse error: invalid character <%c>\n", (char)c);
                free(decoded.data);
                exit(EXIT_FAILURE);
            }

            value = (value << 1) | (unsigned int)(c - '0');
            bits++;

            if (bits == 8) {
                push_decoded_byte(&decoded, (unsigned char)value);
                bits = 0;
                value = 0;
            }
        }

        if (bits != 0) {
            fprintf(stderr, "Reverse bin parse error: incomplete byte (need 8 bits per byte)\n");
            free(decoded.data);
            exit(EXIT_FAILURE);
        }

        return decoded;
    }

    {
        int c;
        char token[256] = {0};
        size_t token_len = 0;

        while ((c = fgetc(file)) != EOF) {
            if (is_reverse_separator(c)) {
                if (token_len > 0) {
                    if (option->output_mode == 2) parse_oct_token(token, token_len, &decoded);
                    else parse_dec_token(token, token_len, &decoded);
                    token_len = 0;
                }
                continue;
            }

            if (token_len + 1 >= sizeof(token)) {
                fprintf(stderr, "Reverse parse error: token too long\n");
                free(decoded.data);
                exit(EXIT_FAILURE);
            }

            token[token_len++] = (char)c;
            token[token_len] = '\0';
        }

        if (token_len > 0) {
            if (option->output_mode == 2) parse_oct_token(token, token_len, &decoded);
            else parse_dec_token(token, token_len, &decoded);
        }
    }

    return decoded;
}

static void print_reverse(options *option, FILE *file, display_state *state, dump_analysis *analysis) {
    decoded_bytes decoded = decode_reverse_stream(file, option);
    unsigned char *display_buffer = get_display_buffer();

    size_t start = option->offset_read;
    if (start > decoded.len) start = decoded.len;

    size_t end = decoded.len;
    if (option->read_size != 0) {
        size_t requested_end = start + option->read_size;
        if (requested_end < end) end = requested_end;
    } else if (option->limit_read != 0 && option->limit_read < end) {
        end = option->limit_read;
    }

    if (end < start) end = start;

    size_t pos = start;
    while (pos < end) {
        size_t chunk = end - pos;
        if (chunk > MAX_BUFF_SIZE) chunk = MAX_BUFF_SIZE;

        memcpy(display_buffer, decoded.data + pos, chunk);
        analyse(analysis, display_buffer, chunk);

        int processed = 0;
        while ((size_t)processed < chunk) {
            int line_len = option->buff_size;
            if ((size_t)processed + (size_t)line_len > chunk) {
                line_len = (int)(chunk - (size_t)processed);
            }

            analysis->line_count++;
            render_line(state, processed, line_len);
            state->addr_display += (size_t)line_len;
            processed += line_len;
        }

        pos += chunk;
    }

    free(decoded.data);
}

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
    size_t total_size = analysis->total_bytes;
    
    float zero_pct = 0.0;
    float printable_pct = 0.0;
    float control_pct = 0.0;
    float extended_pct = 0.0;
    if (!option->pipeline) {
        get_file_metadata(option->filename, &meta);
        if (meta.exists) total_size = meta.file_size;
        if (meta.has_created) format_time_local(meta.created_at, created_at, sizeof(created_at));
        if (meta.has_modified) format_time_local(meta.modified_at, modified_at, sizeof(modified_at));
    }

    if (analysis->total_bytes > 0) {
        zero_pct = ((float)analysis->zero_bytes / analysis->total_bytes) * 100.0f;
        printable_pct = ((float)analysis->printable / analysis->total_bytes) * 100.0f;
        control_pct = ((float)analysis->control / analysis->total_bytes) * 100.0f;
        extended_pct = ((float)analysis->extended_ascii / analysis->total_bytes) * 100.0f;
    }

    

    append_magic_summary(magic_line, sizeof(magic_line), &magic_pos);

    if (option->color) fprintf(out, "%s", BORDER_COLOR);
    for (int i = 0; i < row_width; i++) fputc('-', out);
    
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);

    // ------ Analysis summary line ------

    // summary of found magic byte signatures
    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "summary");

    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "%s  |%s size %zu B ; zero %zu (%.1f%%) ; magic %d\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            total_size,
            analysis->zero_bytes,
            zero_pct,            
            analysis->magic_count);
    
    // stats        
    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "stats");

    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "%s    |%s printable %zu (%.1f%%) ; control %zu (%.1f%%) ; extended %zu (%.1f%%)\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            analysis->printable,
            printable_pct,
            analysis->control,
            control_pct,
            analysis->extended_ascii,
            extended_pct);

    // magic byte signatures found in the file
    if (analysis->magic_count > 0) {
        if (option->color) fprintf(out, "%s", HEADER_COLOR);
        fprintf(out, "magic");
        
        if (option->color) fprintf(out, "%s", RESET);
        fprintf(out, "%s    |%s %s\n",
            option->color ? BORDER_COLOR : "",
            option->color ? ANALYSIS_TEXT_COLOR : "",
            magic_line);
    }
    
    // view options
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
    else if (option->limit_read == total_size) fprintf(out, "limit eof");
    else if (option->limit_read != 0) fprintf(out, "limit %zu", option->limit_read);

    else fprintf(out, "limit eof");
    fputc('\n', out);
    
    // time and metadata
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
void print_output(options *option) {
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

    if (option->search_len > 0 && option->reverse_mode) {
        fprintf(stderr, "Search is currently not supported in reverse mode\n");
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

    // Search for magic byte signatures in the file header when dumping binary input.
    if (!option->skip_header && !option->reverse_mode) find_magic_bytes_in_stream_header(file);
    
    // Print header if not in raw mode.
    if (!option->raw && !option->skip_header) print_header(out, option, state.addr_width);

    int bytes_read = 0; // Bytes read in last read_file_to_buffer call.
    analysis.magic_count = count_found_magic();

    if (option->reverse_mode) {
        print_reverse(option, file, &state, &analysis);
    } else {
        // --- Output Loop ---
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

                analysis.line_count++;

                render_line(&state, processed, line_len);

                state.addr_display += (size_t)line_len;
                processed += line_len;
            }
        }
    }

    if (!option->raw && !option->skip_header) print_footer(out, option, state.addr_width, &analysis);
    if (!option->raw && !option->skip_header) fprintf(out, "\n");

    // Closes the file handle opened at the beginning of print_output.
    if (!option->pipeline) fclose(file);
    if (option->pager) pclose(out);
    free_search_results(search_results);
}
