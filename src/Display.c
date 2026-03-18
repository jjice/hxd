/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Display.h"
#include "Args.h"
#include "File.h"
#include "MagicBytes.h"
#include "Utils.h"

#define MAX_BUFF_SIZE 16384
#define MAX_LINE_SIZE 32768

static unsigned char buffer[MAX_BUFF_SIZE] = {0}; // Static buffer to avoid dynamic allocation overhead. Size is MAX_BUFF_SIZE.

static unsigned char lookback[64] = {0};
static size_t lookback_len = 0;
static const size_t MAX_LOOKBACK = sizeof(lookback);

static char prev_line[MAX_LINE_SIZE] = {0};
static size_t prev_line_pos = 0;
static bool have_prev = false;

typedef struct {
    FILE *out;
    options *option;
    size_t addr_display;
    int addr_width;
    unsigned char _max;
    unsigned char _min;
    bool no_newline;
} display_state;

typedef struct {
    size_t total_bytes;
    size_t zero_bytes;
    size_t line_count;
    int magic_count;
} dump_analysis;

// Return number of hex digits needed to represent a value.
static int hex_digits_size_t(size_t value) {
    int digits = 1;
    while (value >= 16) {
        value >>= 4;
        digits++;
    }
    return digits;
}

// Calculate visible row width of one rendered data line.
static int calc_row_width(const options *option, int addr_width) {
    int width = addr_width + 3;          // "<addr> | "
    width += option->buff_size * 3;      // "HH "
    if (option->ascii) {
        width += 2;                      // "| "
        width += option->buff_size;      // ASCII chars
    } else width-= 1; // Remove trailing space if no ASCII section
    return width;
}

// Append formatted content to a line buffer without writing out of bounds.
static void append_to_line(char *line, size_t line_size, size_t *line_pos, const char *fmt, ...) {
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

// Calculate the Shannon entropy of a byte buffer
static float calc_entropy(unsigned char *data, size_t len) {
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

// Check for search-signatures in the header of the file stream and print matches
static bool contains_query_in_line_or_overlap(unsigned char *current_line, int current_len, options *option) {
    // If no search query is set, we consider it a match (fail open).
    if (!option->search || option->search[0] == '\0') {
        return true;
    }

    // Convert the search query (ASCII or hex) into a byte array for comparison.
    unsigned char needle_buf[256];
    size_t needle_len = 0;

    if (option->search_hex) {
        // hex-mode: "4d5a90" -> {0x4d, 0x5a, 0x90}
        const char *s = (const char *)option->search;
        size_t slen = strlen(s);

        needle_len = 0;
        for (size_t i = 0; i < slen && needle_len < sizeof(needle_buf);) {
            // whitespaceskipping: "4d 5a 90" -> "4d5a90"
            while (i < slen && isspace((unsigned char)s[i])) i++;

            if (i + 1 >= slen) break;

            char c1 = tolower((unsigned char)s[i]);
            char c2 = tolower((unsigned char)s[i + 1]);

            unsigned char byte = 0;

            // first nibble
            if (c1 >= '0' && c1 <= '9') byte = (c1 - '0') << 4;
            else if (c1 >= 'a' && c1 <= 'f') byte = (c1 - 'a' + 10) << 4;
            else break;

            // lower nibble
            if (c2 >= '0' && c2 <= '9') byte |= (c2 - '0');
            else if (c2 >= 'a' && c2 <= 'f') byte |= (c2 - 'a' + 10);
            else break;

            needle_buf[needle_len++] = byte;
            i += 2;
        }

        if (needle_len == 0) {
            // No valid hex bytes found in the search string, consider it a match (fail open).
            return true;
        }
    } else {
        // ascii-mode: "MZ" -> {0x4d, 0x5a}
        needle_len = strlen((const char *)option->search);
        if (needle_len == 0) return true;

        if (needle_len > sizeof(needle_buf)) needle_len = sizeof(needle_buf);
        memcpy(needle_buf, option->search, needle_len);
    }

    if (current_len >= (int)needle_len) {
        for (int i = 0; i <= current_len - (int)needle_len; i++) {
            if (memcmp(current_line + i, needle_buf, needle_len) == 0) {
                return true;
            }
        }
    }

    // Keep up to needle_len-1 bytes so we can detect matches across line boundaries.
    size_t keep = needle_len > 0 ? needle_len - 1 : 0;

    // Check for matches that span the boundary between the lookback buffer and the current line.
    if (lookback_len > 0 && current_len > 0) {
        size_t combined_len = lookback_len + (size_t)current_len;
        if (combined_len < needle_len) goto save_lookback;

        size_t max_start = lookback_len + (size_t)current_len - needle_len;
        if (max_start > lookback_len) max_start = lookback_len;

        for (size_t start = 0; start <= max_start; start++) {
            bool match = true;
            for (size_t k = 0; k < needle_len; k++) {
                size_t pos = start + k;
                unsigned char byte;

                if (pos < lookback_len) {
                    byte = lookback[pos];
                } else {
                    size_t idx = pos - lookback_len;
                    if (idx >= (size_t)current_len) {
                        match = false;
                        break;
                    }
                    byte = current_line[idx];
                }

                if (byte != needle_buf[k]) {
                    match = false;
                    break;
                }
            }
            if (match) return true;
        }
    }

save_lookback:
    if (current_len >= (int)keep) {
        memcpy(lookback, current_line + current_len - (int)keep, keep);
        lookback_len = keep;
    } else {
        if (lookback_len + (size_t)current_len > MAX_LOOKBACK) {
            size_t shift = lookback_len + (size_t)current_len - MAX_LOOKBACK;
            memmove(lookback, lookback + shift, lookback_len - shift);
            lookback_len -= shift;
        }
        memcpy(lookback + lookback_len, current_line, (size_t)current_len);
        lookback_len += (size_t)current_len;
    }

    return false;
}

// Print the header with file information and current dump settings.
static void print_header(FILE *out, options *option, int addr_width) {
    const char *src = option->pipeline ? "<pipe>" : option->filename;
    int row_width = calc_row_width(option, addr_width);
    
    // Maybe added later
    //int border_width = row_width > title_len ? row_width : title_len; 
    //int title_len = (int)strlen("dump for ") + (int)strlen(src);

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "\ndump for %s:\n", src);

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "%-*s | ", addr_width, "offset");

    // Prints column headers (offsets within the line: 00, 01, 02 ...).
    for (int i = 0; i < option->buff_size; i++) {
        fprintf(out, "%02X ", i);
    }
    if (option->ascii) fprintf(out, "| ascii");
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);

    // Prints the horizontal separator line.
    int ascii_sep_pos = addr_width + 3 + (option->buff_size * 3);
    if (option->color) fprintf(out, "%s", BORDER_COLOR);
    for (int i = 0; i < row_width; i++) {
        if (option->ascii == true && i == ascii_sep_pos) {
            fputc('+', out);
        } else fputc('-', out);
    }
    if (option->color) fprintf(out, "%s", RESET);
    fputc('\n', out);
}

static int found_magic_arr[256] = {0}; // Array to track which magic signatures have been found, indexed by signature ID.

// Count how many known magic bytes were found.
static int count_found_magic(void) {
    int found_count = 0;
    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        if (found_magic_arr[sig_id] == 1) found_count++;
    }
    return found_count;
}

// Append a compact list of found magic bytes.
static void append_magic_summary(char *buffer, size_t buffer_size, size_t *pos) {
    int printed = 0;

    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        if (found_magic_arr[sig_id] != 1) continue;

        const MagicSignature *sig = &Magic_Signatures[sig_id];
        if (printed > 0) append_to_line(buffer, buffer_size, pos, " | ");
        append_to_line(buffer, buffer_size, pos, "%s @ %d", sig->description, sig->offset);

        printed++;
        if (printed == 3) break;
    }

    if (count_found_magic() > printed) {
        append_to_line(buffer, buffer_size, pos, " | +%d more", count_found_magic() - printed);
    }
}

// Print compact footer with analysis and metadata.
static void print_footer(FILE *out, options *option, int addr_width, dump_analysis *analysis) {
    int row_width = calc_row_width(option, addr_width);
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

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "summary");
    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "  | size %zu B | zero %zu (%.1f%%) | lines %zu | magic %d\n",
            shown_size,
            analysis->zero_bytes,
            zero_pct,
            analysis->line_count,
            analysis->magic_count);

    if (!option->pipeline) {
        if (option->color) fprintf(out, "%s", HEADER_COLOR);
        fprintf(out, "time");
        if (option->color) fprintf(out, "%s", RESET);
        fprintf(out, "     | created %s | modified %s\n", created_at, modified_at);
    }

    if (option->color) fprintf(out, "%s", HEADER_COLOR);
    fprintf(out, "view");
    if (option->color) fprintf(out, "%s", RESET);
    fprintf(out, "     | width %d | offset %zu | ",
            option->buff_size,
            option->offset_read);

    if (option->read_size != 0) fprintf(out, "read %zu", option->read_size);
    else if (option->limit_read != 0) fprintf(out, "limit %zu", option->limit_read);
    else fprintf(out, "limit eof");
    fputc('\n', out);

    if (analysis->magic_count > 0) {
        if (option->color) fprintf(out, "%s", HEADER_COLOR);
        fprintf(out, "magic");
        if (option->color) fprintf(out, "%s", RESET);
        fprintf(out, "    | %s\n", magic_line);
    }
}

// Scan the beginning of the file stream for known magic byte signatures.
static void find_magic_bytes_in_stream_header(FILE *file) {
    if (file == NULL) return;
    if (file == stdin) return;

    // Get real file size once
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size == 0) return;

    // Read a chunk of the file into a buffer for signature scanning.
    #define READ_PREFIX_SIZE 65536
    size_t to_read = (file_size < READ_PREFIX_SIZE) ? file_size : READ_PREFIX_SIZE;
    unsigned char *magic_buffer = malloc(to_read);
    if (!magic_buffer) {
        perror("Malloc failed for file header");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(magic_buffer, 1, to_read, file);

    // Scan all known signatures
    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        const MagicSignature *sig = &Magic_Signatures[sig_id];

        // Skip if file too small or offset beyond what we read
        if (sig->offset + sig->len > file_size || sig->offset + sig->len > bytes_read) {
            continue;
        }

        // Compare directly in the buffer
        if (memcmp(magic_buffer + sig->offset, sig->bytes, sig->len) == 0) {
            found_magic_arr[sig_id] = 1;
        }
    }

    free(magic_buffer);
}

// Open and return the input stream based on options.
static FILE *open_input_file(options *option) {
    FILE *file = NULL;

    // checks if filename is empty, then use stdin as file
    if (!option->filename) {
        file = stdin;
    }

    // Opens the file for reading in binary mode.
    else {
        file = fopen(option->filename, "rb");
        if (!file) {
            perror("Cant open file / No accsess to pipeline");
            exit(EXIT_FAILURE);
        }
    }

    return file;
}

// Resolve byte color based on selected mode and mutate the byte if needed.
static const char *resolve_byte_color(unsigned char *byte, display_state *state) {
    options *option = state->option;
    unsigned char b = *byte;
    const char *col = RESET;

    // Color logic for heatmap and string options.
    if (option->heatmap == 1) {
        col = heatmap_colors[INDEX_MAP(b, state->_min, state->_max)];
    } else if (option->heatmap == 2) {
        col = heatmap_colors[(int)(b / 16)];
    } else if (option->string) {
        if (b >= 32 && b < 127) col = ASCII_COLOR;
        else if (b == 0) col = NULL_BYTE_COLOR; // Null Byte
        else if (b == 9) col = NON_PRINT_COLOR; // Tab
        else if (b == 10) col = NON_PRINT_COLOR; // Line Feed (LF)
        else if (b == 13) col = NON_PRINT_COLOR; // Carriage Return (CR)
        else if (b == 24) col = NON_PRINT_COLOR; // Control characters like CAN
        else if (b == 26) col = NON_PRINT_COLOR; // Control characters like SUB
        else if (b == 27) col = NON_PRINT_COLOR; // Control characters like ESC
        else if (b == 127) col = NON_PRINT_COLOR; // DEL
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

// Append the address prefix of a line.
static void append_line_prefix(char *line, size_t *line_pos, display_state *state) {
    // Address output with optional color. Uses addr_display for the current line's starting address.
    if (state->option->raw) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "");
    } else if (state->option->color) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%0*zX%s | ", ADDR_COLOR, state->addr_width, state->addr_display, RESET);
    } else {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%0*zX | ", state->addr_width, state->addr_display);
    }
}

// Append the HEX section of a line.
static void append_hex_section(char *line, size_t *line_pos, display_state *state, int processed, int line_len) {
    // HEX loop: Iterates through each byte in the current line,
    // applying coloring based on heatmap, string, or color options.
    for (int i = 0; i < state->option->buff_size; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);

            // Prints the hex value of the byte with appropriate spacing.
            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%02x", b);
            else if (state->option->string && b == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "   ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%02x ", col, b);
            }
        } else {
            append_to_line(line, MAX_LINE_SIZE, line_pos, "   ");
        }
    }
}

static void append_octal_section(char *line, size_t *line_pos, display_state *state, int processed, int line_len) {
    // OCTAL loop: Similar to the HEX loop but formats bytes in octal representation.
    for (int i = 0; i < state->option->buff_size; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%03o", b);
            else if (state->option->string && b == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "    ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03o ", col, b);
            }
        } else {
            append_to_line(line, MAX_LINE_SIZE, line_pos, "    ");
        }
    }
}

static void append_binary_section(char *line, size_t *line_pos, display_state *state, int processed, int line_len) {
    // BINARY loop: Similar to the HEX loop but formats bytes in binary representation.
    for (int i = 0; i < state->option->buff_size; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%08b", b);
            else if (state->option->string && b == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "         ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%08b ", col, b);
            }
        } else {
            append_to_line(line, MAX_LINE_SIZE, line_pos, "         ");
        }
    }
}

static void append_decimal_section(char *line, size_t *line_pos, display_state *state, int processed, int line_len) {
    // DECIMAL loop: Similar to the HEX loop but formats bytes in decimal representation.
    for (int i = 0; i < state->option->buff_size; i++) {
        if (i < line_len) {
            unsigned char b = buffer[processed + i];
            const char *col = resolve_byte_color(&b, state);

            if (state->option->raw) append_to_line(line, MAX_LINE_SIZE, line_pos, "%03u", b);
            else if (state->option->string && b == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "    ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%03u ", col, b);
            }
        } else {
            append_to_line(line, MAX_LINE_SIZE, line_pos, "    ");
        }
    }
}

// Append the ASCII section of a line and return rendered chars.
static int append_ascii_section(char *line, size_t *line_pos, display_state *state, int processed, int line_len) {
    int char_written = 0;

    // ASCII output loop: Similar to the HEX loop,
    // but focuses on printing the ASCII representation of the bytes.
    if (state->option->ascii) {
        append_to_line(line, MAX_LINE_SIZE, line_pos, "\x1b[0m| ");

        for (int i = 0; i < line_len; i++) {
            unsigned char c = buffer[processed + i];
            const char *col = resolve_byte_color(&c, state);
            char disp = (c >= 32 && c < 127) ? (char)c : '.';

            if (state->option->string && c == 0) {
                append_to_line(line, MAX_LINE_SIZE, line_pos, " ");
            } else {
                append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%c", col, disp);
            }

            char_written += 1;
        }
    }

    return char_written;
}

// Append entropy bar for a line if enabled.
static void append_entropy_bar(char *line, size_t *line_pos, display_state *state, int processed, int line_len, int char_written, options *option) {
    // Heatmap bar logic: If heatmap is enabled, calculates the entropy of the current line.
    if (state->option->entropie) {
        float entropy = calc_entropy(buffer + processed, (size_t)line_len);

        const char *bar;
        if (entropy < 2.0f) bar = "░";
        else if (entropy < 4.0f) bar = "▒";
        else if (entropy < 6.0f) bar = "▓";
        else bar = "█";

        const char *col = heatmap_colors[INDEX_MAP(entropy, 0.0f, 8.0f)];

        int space_for_bar = 0;

        if (!option->ascii) space_for_bar = 1;
        else space_for_bar = state->option->buff_size - char_written + 1;
        
        append_to_line(line, MAX_LINE_SIZE, line_pos, "%s%*s%s", col, space_for_bar, "", bar);
    }
}

// Emit current line or cache it as previous line depending on search result.
static void emit_line_or_cache(char *line, size_t line_pos, display_state *state, int processed, int line_len) {
    // Writes the constructed line to the output (pager or stdout).
    bool matches_search = (state->option->search == NULL || state->option->search[0] == '\0') ||
                          contains_query_in_line_or_overlap(buffer + processed, line_len, state->option);

    if (matches_search && !is_space((size_t)state->option->buff_size, (unsigned char *)line)) {
        // Treffer -> vorherige Zeile (falls vorhanden) zuerst ausgeben
        if (have_prev && prev_line_pos > 0) {
            fwrite(prev_line, 1, prev_line_pos, state->out);
        }

        // aktuelle Zeile ausgeben
        fwrite(line, 1, line_pos, state->out);

        have_prev = false; // verbraucht
    } else {
        // Kein Treffer -> für die nächste Iteration merken
        memcpy(prev_line, line, line_pos);
        prev_line_pos = line_pos;
        have_prev = true;
    }
}

// Build and emit one full output line.
static void render_line(display_state *state, int processed, int line_len) {
    char line[MAX_LINE_SIZE] = {0}; // Buffer for each line
    size_t line_pos = 0;            // Line position (cursor for appending)

    append_line_prefix(line, &line_pos, state);
    append_hex_section(line, &line_pos, state, processed, line_len);

    int char_written = append_ascii_section(line, &line_pos, state, processed, line_len);
    append_entropy_bar(line, &line_pos, state, processed, line_len, char_written, state->option);

    if (!state->no_newline) append_to_line(line, MAX_LINE_SIZE, &line_pos, "\x1b[0m\n");

    emit_line_or_cache(line, line_pos, state, processed, line_len);
}

// Main function to print the hex dump based on the provided options
void print_hex(options *option) {
    // Open pager if requested, otherwise use stdout
    FILE *out = stdout;
    if (option->pager) {
        out = open_pager();
    }

    // Reset line/search carry-over state before rendering.
    memset(lookback, 0, sizeof(lookback));
    lookback_len = 0;
    memset(prev_line, 0, sizeof(prev_line));
    prev_line_pos = 0;
    have_prev = false;
    memset(found_magic_arr, 0, sizeof(found_magic_arr));

    display_state state = {0};
    dump_analysis analysis = {0};
    state.out = out;
    state.option = option;
    state.addr_display = option->offset_read;
    state.addr_width = 8;
    state._max = 255;
    state._min = 0;
    state.no_newline = false;

    // Calculates dynamic address width to keep header and rows aligned.
    size_t max_addr = option->offset_read;
    
    if (option->read_size != 0) max_addr = option->offset_read + option->read_size;
    else if (option->limit_read != 0) max_addr = option->limit_read;
    
    int needed_digits = hex_digits_size_t(max_addr);
    if (needed_digits > state.addr_width) state.addr_width = needed_digits;

    FILE *file = open_input_file(option);

    // Search for magic byte signatures in the file header if not skipped
    if (!option->skip_header) find_magic_bytes_in_stream_header(file);
    
    // Print header if not in raw mode.
    if (!option->raw && !option->skip_header) print_header(out, option, state.addr_width);

    if (option->heatmap == 1) {
        find_extrema(&state._max, &state._min, file);
    }

    // No newline logic
    if (option->buff_size == 0) {
        state.no_newline = true;
        option->buff_size = 1024;
    }

    int bytes_read = 0; // Bytes read in last read_file_to_buffer call.
    analysis.magic_count = count_found_magic();

    // --- Hex Output Loop ---
    while (1) {
        size_t limit = 0;

        if (option->read_size != 0) limit = option->offset_read + option->read_size;
        else if (option->limit_read != 0) limit = option->limit_read;

        read_stream_to_buffer(&bytes_read, file, option->offset_read, limit, buffer, false);
        if (bytes_read == 0) break;

        int processed = 0;

        // Process the buffer in lines of buff_size, ensuring we don't exceed bytes_read.
        while (processed < bytes_read) {
            int line_len = option->buff_size;
            if (processed + line_len > bytes_read) {
                line_len = bytes_read - processed;
            }

            for (int i = 0; i < line_len; i++) {
                if (buffer[processed + i] == 0) analysis.zero_bytes++;
            }

            analysis.total_bytes += (size_t)line_len;
            analysis.line_count++;

            render_line(&state, processed, line_len);

            state.addr_display += (size_t)line_len;
            processed += line_len;
        }
    }

    fprintf(out, "\n");
    if (!option->raw && !option->skip_header) print_footer(out, option, state.addr_width, &analysis);
    if (!option->raw && !option->skip_header) fprintf(out, "\n");

    // Closes the file handle opened at the beginning of print_hex.
    if (!option->pipeline) fclose(file);
}
