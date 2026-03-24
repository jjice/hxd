/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include "DisplayUtils.h"

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MagicBytes.h"
#include "Utils.h"

static unsigned char buffer[MAX_BUFF_SIZE] = {0};       // Shared buffer for reading file chunks and rendering lines.
static int found_magic_arr[256] = {0};                  // Array to track which magic byte signatures have been found in the file header.

// Calculates the Shannon entropy of a given data buffer, which can 
// be used to determine the randomness of the data in that buffer.
static inline float calc_entropy(const unsigned char *data, size_t len) {
    if (len == 0) return 0.0f;

    size_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) {
        freq[data[i]]++;
    }

    float entropy = 0.0f;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            float p = (float)freq[i] / (float)len;
            entropy -= p * log2f(p);
        }
    }

    return entropy;
}

// Determines how many characters to use for each byte based on the selected output mode.
static inline int get_chars_per_byte(const options *option) {
    switch (option->output_mode) {
        case 1:
            return 8;
        case 2:
        case 3:
            return 3;
        case 0:
        default:
            return 2;
    }
}

// Maps a byte value to a color code based on the current heatmap, string, and color options.
static inline const char *resolve_byte_color(unsigned char *byte, const display_state *state) {
    const options *option = state->option;
    unsigned char b = *byte;
    const char *col = RESET;

    if (option->heatmap == 1) {
        col = heatmap_colors[INDEX_MAP(b, state->_min, state->_max)];
    } else if (option->heatmap == 2) {
        col = heatmap_colors[(int)(b / 16)];
    } else if (option->string) {
        if (b >= 32 && b < 127) col = ASCII_COLOR;
        else if (b == 0) col = NULL_BYTE_COLOR;
        else if (b == 9) col = NON_PRINT_COLOR;
        else if (b == 10) col = NON_PRINT_COLOR;
        else if (b == 13) col = NON_PRINT_COLOR;
        else if (b == 24) col = NON_PRINT_COLOR;
        else if (b == 26) col = NON_PRINT_COLOR;
        else if (b == 27) col = NON_PRINT_COLOR;
        else if (b == 127) col = NON_PRINT_COLOR;
        else {
            col = ASCII_COLOR;
            b = 0;
        }
    } else if (option->color) {
        if (b == 0) col = NULL_BYTE_COLOR;
        else if (b >= 32 && b < 127) col = ASCII_COLOR;
        else col = NON_PRINT_COLOR;
    }

    *byte = b;
    return col;
}

// Appends the line prefix, which includes the address offset and border, with appropriate coloring based on options.
static inline void append_line_prefix(char *line, size_t *line_pos, display_state *state) {
    if (state->option->raw) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "");
    } else if (state->option->color) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%0*zX %s| %s",
                       ADDR_COLOR, state->addr_width, state->addr_display, BORDER_COLOR, RESET);
    } else {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%0*zX | ",
                       state->addr_width, state->addr_display);
    }
}

// Determines how many columns to render based on the options and the buffer size, ensuring we don't exceed the buffer.
static inline int get_render_column_count(const display_state *state) {
    if (state->visible_columns > 0 && state->visible_columns < state->option->buff_size) {
        return state->visible_columns;
    }
    return state->option->buff_size;
}

static bool line_has_search_match(display_state *state, size_t line_start, int line_len) {
    if (!state->search_results || state->option->search_len == 0) {
        return true;
    }

    size_t line_end = line_start + (size_t)line_len;
    size_t needle_len = state->option->search_len;

    while (state->search_match_index < state->search_results->count) {
        size_t match_start = state->search_results->matches[state->search_match_index].addr;
        size_t match_end = match_start + needle_len;
        if (match_end > line_start) {
            break;
        }
        state->search_match_index++;
    }

    for (size_t i = state->search_match_index; i < state->search_results->count; i++) {
        size_t match_start = state->search_results->matches[i].addr;
        size_t match_end = match_start + needle_len;

        if (match_start >= line_end) {
            break;
        }

        if (match_start < line_end && match_end > line_start) {
            return true;
        }
    }

    return false;
}

static bool byte_is_highlighted(const display_state *state, size_t abs_addr) {
    if (!state->search_results || state->option->search_len == 0) {
        return false;
    }

    for (size_t i = state->search_match_index; i < state->search_results->count; i++) {
        size_t match_start = state->search_results->matches[i].addr;
        if (match_start > abs_addr) {
            break;
        }

        if (abs_addr < match_start + state->option->search_len) {
            return true;
        }
    }

    return false;
}

// Appends spacing between groups of bytes based on the grouping option, and adds an extra 
// space at the end of the line if it's the last column. -> X Y  X Y ...
static inline void append_group_spacing(char *line, size_t *line_pos, const display_state *state, int i) {
    int grouping = state->option->grouping;
    int column_count = get_render_column_count(state);

    if (i == column_count - 1) {
        if (grouping != 0) {
            append_to_line(line, MAX_LINE_SIZE, line_pos, " ");
        }
        return;
    }

    if (grouping == 0) {
        return;
    }

    append_to_line(line, MAX_LINE_SIZE, line_pos, " ");
    if (grouping > 1 && (i + 1) % grouping == 0) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, " ");
    }
}

// Appends a blank cell to the line, which is used for padding when the line has fewer bytes 
// than the column count, or when a null byte is represented as blank in string mode.
static inline void append_blank_cell(char *line, size_t *line_pos, const display_state *state, int i, int cell_width) {
    append_to_line(line, MAX_LINE_SIZE, line_pos, "%*s", cell_width, "");
    if (!state->option->raw) {
        append_group_spacing(line, line_pos, state, i);
    }
}

// Appends the hex representation of bytes to the line, applying coloring and spacing based on options.
static void append_hex_section(char *line, size_t *line_pos, const display_state *state, int processed, int line_len) {
    int column_count = get_render_column_count(state);

    for (int i = 0; i < column_count; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);
            bool highlight = state->option->color &&
                             byte_is_highlighted(state, state->addr_display + (size_t)i);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%02x", b);
            else if (highlight) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%02x%s", HIGHLIGHT_COLOR, b, RESET);
                append_group_spacing(line, line_pos, state, i);
            } else if (state->option->string && b == 0) append_blank_cell(line, line_pos, state, i, 2);
            else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%02x", col, b);
                append_group_spacing(line, line_pos, state, i);
            }
        } else {
            append_blank_cell(line, line_pos, state, i, 2);
        }
    }
}

// Similar to append_hex_section but for octal representation of bytes.
static void append_octal_section(char *line, size_t *line_pos, const display_state *state, int processed, int line_len) {
    int column_count = get_render_column_count(state);

    for (int i = 0; i < column_count; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);
            bool highlight = state->option->color &&
                             byte_is_highlighted(state, state->addr_display + (size_t)i);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%03o", b);
            else if (highlight) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03o%s", HIGHLIGHT_COLOR, b, RESET);
                append_group_spacing(line, line_pos, state, i);
            } else if (state->option->string && b == 0) append_blank_cell(line, line_pos, state, i, 3);
            else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03o", col, b);
                append_group_spacing(line, line_pos, state, i);
            }
        } else {
            append_blank_cell(line, line_pos, state, i, 3);
        }
    }
}

// Similar to append_hex_section but for binary representation of bytes.
static void append_binary_section(char *line, size_t *line_pos, const display_state *state, int processed, int line_len) {
    int column_count = get_render_column_count(state);

    for (int i = 0; i < column_count; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);
            bool highlight = state->option->color &&
                             byte_is_highlighted(state, state->addr_display + (size_t)i);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%08b", b);
            else if (highlight) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%08b%s", HIGHLIGHT_COLOR, b, RESET);
                append_group_spacing(line, line_pos, state, i);
            } else if (state->option->string && b == 0) append_blank_cell(line, line_pos, state, i, 8);
            else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%08b", col, b);
                append_group_spacing(line, line_pos, state, i);
            }
        } else {
            append_blank_cell(line, line_pos, state, i, 8);
        }
    }
}

// Similar to append_hex_section but for decimal representation of bytes.
static void append_decimal_section(char *line, size_t *line_pos, const display_state *state, int processed, int line_len) {
    int column_count = get_render_column_count(state);

    for (int i = 0; i < column_count; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);
            bool highlight = state->option->color &&
                             byte_is_highlighted(state, state->addr_display + (size_t)i);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%03u", b);
            else if (highlight) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03u%s", HIGHLIGHT_COLOR, b, RESET);
                append_group_spacing(line, line_pos, state, i);
            } else if (state->option->string && b == 0) append_blank_cell(line, line_pos, state, i, 3);
            else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03u", col, b);
                append_group_spacing(line, line_pos, state, i);
            }
        } else {
            append_blank_cell(line, line_pos, state, i, 3);
        }
    }
}

// Appends the ASCII representation of bytes to the line, applying coloring and spacing based on options.
static int append_ascii_section(char *line, size_t *line_pos, const display_state *state, int processed, int line_len) {
    int char_written = 0;

    if (state->option->ascii) {
        char *c = (state->option->color) ? BORDER_COLOR : "";
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%s| ", c);

        for (int i = 0; i < line_len; i++) {
            unsigned char c = buffer[processed + i];
            const char *col = resolve_byte_color(&c, state);
            char disp = (c >= 32 && c < 127) ? (char)c : '.';
            bool highlight = state->option->color &&
                             byte_is_highlighted(state, state->addr_display + (size_t)i);

            if (highlight) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%c%s", HIGHLIGHT_COLOR, disp, RESET);
            } else if (state->option->string && c == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, " ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%c", col, disp);
            }

            char_written += 1;
        }
    }

    return char_written;
}

// Appends an entropy bar to the line based on the calculated entropy of the bytes in the line, 
// using different characters and colors to represent different entropy levels.
static void append_entropy_bar(char *line, size_t *line_pos, const display_state *state,
                               int processed, int line_len, int char_written, const options *option) {
    if (state->option->entropie) {
        float entropy = calc_entropy(buffer + processed, (size_t)line_len);

        const char *bar;
        if (entropy < 2.0f) bar = "░";
        else if (entropy < 4.0f) bar = "▒";
        else if (entropy < 6.0f) bar = "▓";
        else bar = "█";

        const char *col = heatmap_colors[INDEX_MAP(entropy, 0.0f, 8.0f)];
        int space_for_bar = !option->ascii ? 1 : get_render_column_count(state) - char_written + 1;

        append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%*s%s", col, space_for_bar, "", bar);
    }
}

// Appends formatted text to the line buffer, ensuring we don't exceed the line size and updating the current position. 
// (smart strcat with formatting and bounds checking.)
void append_to_line(char *line, size_t line_size, size_t *line_pos, const char *fmt, ...) {
    if (*line_pos >= line_size) return;

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(line + *line_pos, line_size - *line_pos, fmt, args);
    va_end(args);

    if (written < 0) return;

    size_t w = (size_t)written;
    if (w >= (line_size - *line_pos)) {
        *line_pos = line_size - 1;
    } else {
        *line_pos += w;
    }
}

// Calculates the number of visible columns to render based on the buffer size, read size, and any read limits set in the options.
int _calc_visible_columns(const options *option) {
    size_t visible = (size_t)option->buff_size;

    if (option->read_size != 0 && option->read_size < visible) {
        visible = option->read_size;
    } else if (option->limit_read > option->offset_read) {
        size_t remaining = option->limit_read - option->offset_read;
        if (remaining < visible) visible = remaining;
    }

    return (int)visible;
}

// Calculates the total width of each line to be rendered, which includes the address offset, 
// hex/octal/decimal columns, ASCII representation, and any spacing or grouping based on options.
int calc_row_width(const options *option, int addr_width, int column_count) {
    int chars_per_byte = get_chars_per_byte(option);
    int grouping = option->grouping;
    int group_gaps = 0;
    int byte_gaps = 0;

    if (column_count > 1 && grouping != 0) {
        byte_gaps = column_count - 1;
    }

    if (column_count > 0 && grouping > 1) {
        group_gaps = (column_count - 1) / grouping;
    }

    int width = addr_width + 3;
    width += column_count * chars_per_byte;
    width += byte_gaps;
    width += group_gaps;

    if (option->ascii) {
        width += 2;
        width += column_count;
    } else {
        if (grouping != 0) {
            width += 1;
        }
    }

    return width;
}

// Counts how many magic byte signatures have been found in the file header, 
// which can be used to display a summary of detected file types.
int count_found_magic(void) {
    int found_count = 0;

    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        if (found_magic_arr[sig_id] == 1) found_count++;
    }

    return found_count;
}

// Appends a summary of found magic byte signatures to the output line, showing the description and 
// offset of each detected signature, and indicating if there are more signatures than can be displayed.
void append_magic_summary(char *buffer_out, size_t buffer_size, size_t *pos) {
    int printed = 0;
    int found_count = count_found_magic();

    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        if (found_magic_arr[sig_id] != 1) continue;

        const MagicSignature *sig = &Magic_Signatures[sig_id];
        if (printed > 0) append_to_line(buffer_out, buffer_size, pos, " | ");
        append_to_line(buffer_out, buffer_size, pos, "%s @ %d", sig->description, sig->offset);

        printed++;
        if (printed == 3) break;
    }

    if (found_count > printed) {
        append_to_line(buffer_out, buffer_size, pos, " | +%d more", found_count - printed);
    }
}

// Reads the first part of the file and checks for known magic byte signatures in the header,
// updating the found_magic_arr to indicate which signatures were detected.
void find_magic_bytes_in_stream_header(FILE *file) {
    if (file == NULL || file == stdin) return;

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size == 0) return;

    enum { READ_PREFIX_SIZE = 65536 };
    size_t to_read = (file_size < READ_PREFIX_SIZE) ? file_size : READ_PREFIX_SIZE;
    unsigned char *magic_buffer = malloc(to_read);
    if (!magic_buffer) {
        perror("Malloc failed for file header");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(magic_buffer, 1, to_read, file);

    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        const MagicSignature *sig = &Magic_Signatures[sig_id];

        if (sig->offset + sig->len > file_size || sig->offset + sig->len > bytes_read) {
            continue;
        }

        if (memcmp(magic_buffer + sig->offset, sig->bytes, sig->len) == 0) {
            found_magic_arr[sig_id] = 1;
        }
    }

    free(magic_buffer);
}

// Opens the input file based on the options, returning a FILE pointer to either the specified file or stdin if no file is provided.
FILE *open_input_file(options *option) {
    FILE *file = NULL;

    if (!option->filename) {
        file = stdin;
    } else {
        file = fopen(option->filename, "rb");
        if (!file) {
            perror("Cant open file / No accsess to pipeline");
            exit(EXIT_FAILURE);
        }
    }

    return file;
}

// Appends the header line with column labels (e.g., 00 01 02 ... for hex) to the output, applying spacing and grouping based on options.
void append_header_columns(char *line, size_t line_size, size_t *line_pos, const options *option, int column_count) {
    int grouping = option->grouping;
    int cell_width = get_chars_per_byte(option);

    for (int i = 0; i < column_count; i++) {
        append_to_line(line, line_size, line_pos, "%02X", i);

        if (cell_width > 2) {
            append_to_line(line, line_size, line_pos, "%*s", cell_width - 2, "");
        }

        if (i == column_count - 1) {
            if (option->ascii && grouping != 0) {
                append_to_line(line, line_size, line_pos, " ");
            }
            continue;
        }

        if (grouping == 0) {
            continue;
        }

        append_to_line(line, line_size, line_pos, " ");
        if (grouping > 1 && (i + 1) % grouping == 0) {
            append_to_line(line, line_size, line_pos, " ");
        }
    }
}

// Renders a single line of output based on the current display state, including the address offset, 
// hex/octal/decimal columns, ASCII representation, and entropy bar, while applying coloring and spacing based on options.
void render_line(display_state *state, int processed, int line_len) {
    char line[MAX_LINE_SIZE] = {0};
    size_t line_pos = 0;

    if (!line_has_search_match(state, state->addr_display, line_len)) {
        return;
    }

    if (state->option->skip_zero) {
        bool all_zero = true;
        for (int i = 0; i < line_len; i++) {
            if (buffer[processed + i] != 0) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) return;
    }

    append_line_prefix(line, &line_pos, state);

    switch (state->option->output_mode) {
        case 1:
            append_binary_section(line, &line_pos, state, processed, line_len);
            break;
        case 2:
            append_octal_section(line, &line_pos, state, processed, line_len);
            break;
        case 3:
            append_decimal_section(line, &line_pos, state, processed, line_len);
            break;
        case 0:
        default:
            append_hex_section(line, &line_pos, state, processed, line_len);
            break;
    }

    int char_written = append_ascii_section(line, &line_pos, state, processed, line_len);
    append_entropy_bar(line, &line_pos, state, processed, line_len, char_written, state->option);

    if (!state->no_newline) append_to_line(line, MAX_LINE_SIZE, &line_pos, "\x1b[0m\n");

    if (!is_space((size_t)state->option->buff_size, (unsigned char *)line)) {
        fwrite(line, 1, line_pos, state->out);
    }
}

// Calculates the number of hexadecimal digits needed to represent a size_t value, 
// which is used for determining the width of the address offset in the output.
int _hex_digits_size_t(size_t value) {
    int digits = 1;

    while (value >= 16) {
        value >>= 4;
        digits++;
    }

    return digits;
}

// Resets the internal state of the display utilities, clearing buffers and resetting flags, 
// which can be useful when processing multiple files or streams in a single run.
void reset_display_utils_state(void) {
    memset(buffer, 0, sizeof(buffer));
    memset(found_magic_arr, 0, sizeof(found_magic_arr));
}

// Provides access to the shared buffer used for reading file chunks and rendering lines, 
// allowing other parts of the program to read from or write to this buffer as needed.
unsigned char *get_display_buffer(void) {
    return buffer;
}

static void show_progress(const char *label, size_t current, size_t total, const options *option) {
    if (total == 0) {
        return;
    }

    const size_t bar_width = 40;
    size_t filled = current * bar_width / total;
    size_t percent = current * 100 / total;

    if (option->color) {
        fprintf(stderr, "%s", HEADER_COLOR);
    }

    fprintf(stderr, "\r%s [", label);
    for (size_t i = 0; i < bar_width; ++i) {
        fputc(i < filled ? '#' : '.', stderr);
    }
    fprintf(stderr, "] %zu%%", percent);
    fflush(stderr);

    if (option->color) {
        fprintf(stderr, "%s", RESET);
    }

}

SearchResults *search_matches(const options *option, FILE *file) {
    if (!file || !option->search || option->search_len == 0) {
        return NULL;
    }

    if (file == stdin) {
        return NULL;
    }

    size_t file_size = 0;
    size_t search_start = option->offset_read;
    size_t search_limit = 0;
    size_t chunk_size = MAX_BUFF_SIZE;

    fseek(file, 0, SEEK_END);
    file_size = (size_t)ftell(file);

    if (option->read_size != 0) {
        search_limit = option->offset_read + option->read_size;
    } else if (option->limit_read != 0) {
        search_limit = option->limit_read;
    } else {
        search_limit = file_size;
    }

    if (search_limit > file_size) {
        search_limit = file_size;
    }

    if (search_start >= search_limit) {
        fseek(file, search_start, SEEK_SET);
        return NULL;
    }

    fseek(file, search_start, SEEK_SET);

    unsigned char *read_buf = malloc(chunk_size);
    unsigned char *overlap = malloc(option->search_len > 1 ? option->search_len - 1 : 1);
    unsigned char *search_buf = malloc(chunk_size + option->search_len - 1);

    if (!read_buf || !overlap || !search_buf) {
        perror("Malloc failed");
        free(read_buf);
        free(overlap);
        free(search_buf);
        exit(EXIT_FAILURE);
    }

    SearchResults *results = malloc(sizeof(SearchResults));
    if (!results) {
        perror("Malloc failed for results");
        free(read_buf);
        free(overlap);
        free(search_buf);
        exit(EXIT_FAILURE);
    }

    size_t capacity = 1024;
    results->matches = malloc(sizeof(SearchMatch) * capacity);
    if (!results->matches) {
        perror("Malloc failed for matches");
        free(read_buf);
        free(overlap);
        free(search_buf);
        free(results);
        exit(EXIT_FAILURE);
    }
    results->count = 0;

    size_t overlap_len = 0;
    size_t total_read = 0;
    size_t total_bytes = search_limit - search_start;

    while (search_start + total_read < search_limit) {
        size_t remaining = search_limit - (search_start + total_read);
        size_t to_read = remaining < chunk_size ? remaining : chunk_size;
        size_t read_bytes = fread(read_buf, 1, to_read, file);

        if (read_bytes <= 0) {
            break;
        }

        memcpy(search_buf, overlap, overlap_len);
        memcpy(search_buf + overlap_len, read_buf, read_bytes);

        size_t search_len = overlap_len + read_bytes;

        for (size_t i = 0; i + option->search_len <= search_len; i++) {
            if (memcmp(search_buf + i, option->search, option->search_len) == 0) {
                /* Skip matches fully inside old overlap, they were already checked */
                if (i + option->search_len <= overlap_len) {
                    continue;
                }

                size_t abs_pos = search_start + total_read - overlap_len + i;

                if (results->count == capacity) {
                    capacity *= 2;
                    SearchMatch *tmp = realloc(results->matches, sizeof(SearchMatch) * capacity);
                    if (!tmp) {
                        perror("Malloc failed for matches");
                        free(results->matches);
                        free(results);
                        free(read_buf);
                        free(overlap);
                        free(search_buf);
                        exit(EXIT_FAILURE);
                    }
                    results->matches = tmp;
                }

                results->matches[results->count].addr = abs_pos;
                results->matches[results->count].line = (abs_pos - option->offset_read) / (size_t)option->buff_size;
                results->count++;
            }
        }

        if (option->search_len > 1) {
            overlap_len = (search_len < (size_t)(option->search_len - 1))
                        ? search_len
                        : (size_t)(option->search_len - 1);

            memcpy(overlap, search_buf + search_len - overlap_len, overlap_len);
        } else {
            overlap_len = 0;
        }

        total_read += read_bytes;
        show_progress("search", total_read, total_bytes, option);
    }

    if (total_bytes > 0 && total_read < total_bytes) {
        show_progress("search", total_bytes, total_bytes, option);
    }

    if (total_bytes > 0) {
        fputc('\n', stderr);
    }

    fseek(file, search_start, SEEK_SET);

    free(read_buf);
    free(overlap);
    free(search_buf);

    return results;
}

void free_search_results(SearchResults *results) {
    if (!results) {
        return;
    }

    free(results->matches);
    free(results);
}
