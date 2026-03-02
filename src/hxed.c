/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "Utils.h"
#include "Args.h"
#include "MagicBytes.h"


#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_BUFF_SIZE 16384

static unsigned char buffer[16384] = {0}; // Static buffer to avoid dynamic allocation overhead. Size is MAX_BUFF_SIZE.

static unsigned char lookback[64] = {0};           // z. B. 64 → reicht für die meisten Magic-Bytes
static size_t lookback_len = 0;                    // wie viele Bytes sind gültig
static const size_t MAX_LOOKBACK = sizeof(lookback);

static char prev_line[4096] = {0};
static int  prev_line_pos = 0;
static bool have_prev = false;

// Read a chunk of the file into the buffer, starting from read_start and respecting read_limit
void read_stream_to_buffer(int *out_read, FILE *file, size_t read_start, size_t read_limit, unsigned char *_buffer, bool _no_seek) {
    
    // Return if file is stdin
    if (file == stdin) {
        *out_read = fread(_buffer, 1,  MAX_BUFF_SIZE, file);
        return;
    }
    
    // Track the last read_start position to detect when we need to fseek
    static size_t last_read_start = (size_t)-1;

    // Apply start_offset only if read_start has changed
    if (last_read_start != read_start) {
        last_read_start = read_start;
        fseek(file, read_start, SEEK_SET);
    }

    // Current File cursor position
    size_t current_pos = ftell(file);

    // Absolut limit to read
    
    //size_t end_limit = read_start + read_limit;
    size_t end_limit = read_limit;

    // Remaining bytes to read
    size_t remaining = 0;
    if (current_pos < end_limit) {
        remaining = end_limit - current_pos;
    }

    // If remaining is smaller than buff_size: read remaining else read buffsize
    size_t to_read = remaining < (size_t)MAX_BUFF_SIZE ? remaining : (size_t)MAX_BUFF_SIZE;

    // Read filecontent to buffer, output read bytes
    *out_read = fread(_buffer, 1, to_read, file);

    if (_no_seek) {
        // If no_seek is true, we do not want to change the file cursor position after reading.
        // So we seek back to the original position before the read.
        fseek(file, current_pos, SEEK_SET);
    }

    return;
}

// Check if file exists and is not empty, also checks if offset, read and limit are in range of filesize
void check_file(options *option) {

    FILE *fp = fopen(option->filename, "rb");

    if(option->pipeline == true) return;

    if(fp == NULL) {

        // File not found or permission denied.
        perror("File not found / No permission to read");
        exit(EXIT_FAILURE);
    }
    else {

        // Check for zero size
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        if(file_size == 0) {
            printf("File is empty"); // Prints error message
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        // Ceck if filesize is in range of limit
        if (file_size < option->limit_read) {
            printf("Filesize is out of range, check limit | keep empty for EOF");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        
        // Ceck if filesize is in range of offset + read_size
        if (file_size <= option->offset_read + option->read_size) {
            printf("Filesize is out of range, check offset|read");
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        // Check if file limit has been placed, else EOF
        if (option->limit_read == 0) {
            fseek(fp, 0, SEEK_END);
            option->limit_read = ftell(fp);
        }

        // Close the file handle after successful check.
        fclose(fp);
    }
}

// Find the minimum and maximum byte values in the file for heatmap scaling
void find_extrema (unsigned char *_max, unsigned char *_min, FILE *file) {

    unsigned char _buffer[MAX_BUFF_SIZE] = {0};
    int read_bytes = 0;

    *_min = 255;
    *_max = 0;

    // Get file size for read_limit
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read through the file in chunks and update min/max values
    while (1) {
        read_stream_to_buffer(&read_bytes, file, 0, file_size, _buffer, false);
        if (read_bytes <= 0) break;

        for (size_t index = 0; index < (size_t) read_bytes; index++) {
            if (*_min > _buffer[index]) *_min = _buffer[index];
            if (*_max < _buffer[index]) *_max = _buffer[index];

            if (*_min == 0 && *_max == 255) return;
        }
    }

    // Reset file position to beginning
    fseek(file, 0, SEEK_SET);
}

// Calculate the Shannon entropy of a byte buffer
float calc_entropy (unsigned char *data, size_t len) {
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
bool contains_query_in_line_or_overlap(unsigned char *current_line, int current_len, options *option) {
    // If no search query is set, we consider it a match (fail open).
        if (!option->search || option->search[0] == '\0') {
        return true;
    }

    // Convert the search query (ASCII or hex) into a byte array for comparison. 
    // We use a fixed-size buffer (e.g., 256 bytes) to hold the converted search pattern, and keep track of its length.
    unsigned char needle_buf[256];
    size_t needle_len = 0;

    if (option->search_hex) {
        // hex-mode: "4d5a90" → {0x4d, 0x5a, 0x90}
        const char *s = (const char *)option->search;
        size_t slen = strlen(s);

        needle_len = 0;
        for (size_t i = 0; i < slen && needle_len < sizeof(needle_buf); ) {
            // whitespaceskipping: "4d 5a 90" → "4d5a90"
            while (isspace((unsigned char)s[i])) i++;

            if (i + 1 >= slen) break;

           
            char c1 = tolower((unsigned char)s[i]);
            char c2 = tolower((unsigned char)s[i+1]);

           
            while (isspace((unsigned char)s[i])) i++;

            unsigned char byte = 0;

            // first nibble
            if      (c1 >= '0' && c1 <= '9') byte = (c1 - '0') << 4;
            else if (c1 >= 'a' && c1 <= 'f') byte = (c1 - 'a' + 10) << 4;
            else break;

            // lower nibble
            if      (c2 >= '0' && c2 <= '9') byte |= (c2 - '0');
            else if (c2 >= 'a' && c2 <= 'f') byte |= (c2 - 'a' + 10);
            else break;

            needle_buf[needle_len++] = byte;
            i += 2;
        }

        if (needle_len == 0) {
            // No valid hex bytes found in the search string, consider it a match (fail open).
            return true;
        }
    }
    else {
        // ascii-mode: "MZ" → {0x4d, 0x5a}
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

    // Check for matches that span the boundary between the lookback buffer and the current line.
    if (lookback_len > 0 && current_len > 0) {
        size_t combined_len = lookback_len + current_len;
        if (combined_len < needle_len) goto save_lookback;

        size_t max_start = lookback_len + current_len - needle_len;
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

// Update the lookback buffer with the end of the current line for the next iteration. 
// We keep up to MAX_LOOKBACK bytes from the end of the current line, which allows us to detect matches 
// that span across lines without needing to re-read previous data from the file.
save_lookback:;

    size_t keep = needle_len > 0 ? needle_len - 1 : 0;

    if (current_len >= (int)keep) {
        memcpy(lookback, current_line + current_len - keep, keep);
        lookback_len = keep;
    } else {
        if (lookback_len + current_len > MAX_LOOKBACK) {
            size_t shift = lookback_len + current_len - MAX_LOOKBACK;
            memmove(lookback, lookback + shift, lookback_len - shift);
            lookback_len -= shift;
        }
        memcpy(lookback + lookback_len, current_line, current_len);
        lookback_len += current_len;
    }

    return false;
}

// Print the header with file information and magic byte detection results
void print_header(options *option) {

    print_color(HEADER_COLOR, option->color);
    if (option->pipeline) printf("<pipe>:\n\n");
    else printf("\n<%s>:\n\n", option->filename);
    print_color(RESET, option->color);
    
    // Prints spaces to align column headers with the hex data area.
    printf("%*s", 11, " ");
    
    // Prints column headers (offsets within the line: 00, 01, 02...).
    for (int i = 0; i < option->buff_size; i++) {
        printf("%02X ", i);
    }
    
    // Calculates the total length of the separating line based on buff_size and ASCII flag.
    int n = 0;
    if (option->ascii == true) n = 12 + option->buff_size * 3 + option->buff_size; 
    else n = 10 + option->buff_size * 3;
    
    fputc('\n', stdout);
    
    // Prints the horizontal separator line. Includes logic to place a '+' if ASCII is enabled.
    for (int i = 0; i < n; i++) {
        if (option->ascii == true && i == 11 + option->buff_size * 3) {
            fputc('+', stdout);
        }
        fputc('-', stdout);
    }
    //fputc('\n', stdout);
}

static int found_magic_arr [256] = {0}; // Array to track which magic signatures have been found, indexed by signature ID.

// Scan the beginning of the file stream for known magic byte signatures and print any matches.
void find_magic_bytes_in_stream_header(FILE *file, options *option) {
    if (file == NULL) return;

    // Get real file size once
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size == 0) return;

    // Read a chunk of the file into a buffer for signature scanning. We read a fixed size (e.g., 64KB) 
    // which should cover all signatures up to the maximum offset we defined.
    #define READ_PREFIX_SIZE 65536  
    size_t to_read = (file_size < READ_PREFIX_SIZE) ? file_size : READ_PREFIX_SIZE;
    unsigned char *buffer = malloc(to_read);
    if (!buffer) {
        perror("Malloc failed for file header");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(buffer, 1, to_read, file);

    // Scan all known signatures
    for (int sig_id = 0; sig_id < Magic_Signatures_Count; sig_id++) {
        const MagicSignature *sig = &Magic_Signatures[sig_id];

        // Skip if file too small or offset beyond what we read
        if (sig->offset + sig->len > file_size ||
            sig->offset + sig->len > bytes_read) {
            continue;
        }

        // Compare directly in the buffer
        if (memcmp(buffer + sig->offset, sig->bytes, sig->len) == 0) {
            found_magic_arr[sig_id] = 1;   // ← use the global array!
            print_color(MAGIC_COLOR, option->color);
            printf("\n<-> Found %s at offset %d\n", sig->description, sig->offset);
            print_color(RESET, option->color);
        }
    }

    free(buffer);
}

// Main function to print the hex dump based on the provided options
void print_hex(options *option){
    
    // Open pager if requested, otherwise use stdout
    FILE *out = stdout;
    if (option->pager) {
        out = open_pager();
    }

    // Redefine printf to write to the pager or stdout, ensuring all output goes to the correct destination.
    #define printf(...) fprintf(out, __VA_ARGS__)

    int addr_display = 0;
    
    // Apply starting addr
    addr_display += option->offset_read;
    
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

    if (!option->skip_header) find_magic_bytes_in_stream_header(file, option);
    if (!option->raw && !option->skip_header) print_header(option);
    if (!option->raw) fputc('\n', out);
    
    static char line[4096];  // Buffer for each line
    int line_pos = 0;        // Line position
    int bytes_read = 0;      // Bytes read in last read_file_to_buffer call.

    unsigned char _max = 255, _min = 0;  // For heatmap scaling

    if (option->heatmap == 1) {
        find_extrema(&_max, &_min, file);
    }

    // No newline logic
    bool no_newline = false;
    if (option->buff_size == 0) {
        no_newline = true; 
        option->buff_size = 1024;
    }

    
    // --- Hex Output Loop ---
    while (1) {
        size_t limit = 0;

        if (option->read_size != 0) limit = option->offset_read + option->read_size;
        else if (option->limit_read != 0) limit = option->limit_read;

        read_stream_to_buffer(&bytes_read, file, option->offset_read, limit, buffer, false);
        if (bytes_read == 0) break;

        int processed = 0;
        //unsigned char _buffer[MAX_BUFF_SIZE] = {0};
        
        // Process the buffer in lines of buff_size, ensuring we don't exceed bytes_read.
        while (processed < bytes_read) {
            int line_len = option->buff_size;
            if (processed + line_len > bytes_read) {
                line_len = bytes_read - processed;
            }

            line_pos = 0;

            // Address output with optional color. Uses addr_display for the current line's starting address.
            if (option->raw) {line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                    "");}

            else if (option->color) {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                    "%s%08X%s | ", RESET, addr_display, RESET);
            } 
            else {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                    "%08X | ", addr_display);
            }

            // HEX loop: Iterates through each byte in the current line, 
            // applying coloring based on heatmap, string, or color options.
            for (int i = 0; i < option->buff_size; i++) {
                if (i < line_len) {
                    unsigned char b = buffer[processed + i];

                    const char *col = RESET;

                    // Color logic for heatmap and string options. 
                    // Determines the appropriate color code for the current byte.
                    if (option->heatmap == 1) {
                        col = heatmap_colors[INDEX_MAP(b, _min, _max)];
                    }

                    else if (option->heatmap == 2) {
                        col = heatmap_colors[(int)(b / 16)];
                    }

                    else if (option->string) {
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
                    }

                    else if (option->color) {
                        if (b == 0)        col = NULL_BYTE_COLOR;
                        else if (b >= 32 && b < 127) col = ASCII_COLOR;
                        else               col = NON_PRINT_COLOR;
                    }                    

                    // Prints the hex value of the byte with appropriate spacing. 
                    // If string mode is enabled and the byte is 0, it prints spaces instead of "00".

                    if (option->raw) line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "%02x", b);
                    
                    else if (option->string && b == 0) {
                        line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "   ");
                    }
                    else line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "%s%02x ", col, b);
                    
                } 
                else {
                    line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "   ");
                }
            }

            int char_written = 0;

            // ASCII output loop: Similar to the HEX loop, 
            // but focuses on printing the ASCII representation of the bytes.
            if (option->ascii) {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "\x1b[0m| ");

                for (int i = 0; i < line_len; i++) {
                    unsigned char c = buffer[processed + i];
                    const char *col = RESET;
                    
                    if (option->heatmap == 1) {
                        col = heatmap_colors[INDEX_MAP(c, _min, _max)];
                    }
                    
                    else if (option->heatmap == 2) {
                        col = heatmap_colors[(int)(c / 16)];
                    }

                    else if (option->string) {
                        if (c >= 32 && c < 127) col = ASCII_COLOR;
                        else if (c == 0) col = NULL_BYTE_COLOR; // Null Byte
                        else if (c == 9) col = NON_PRINT_COLOR; // Tab
                        else if (c == 10)  col = NON_PRINT_COLOR; // Line Feed (LF)
                        else if (c == 13) col = NON_PRINT_COLOR; // Carriage Return (CR)
                        else if (c == 24) col = NON_PRINT_COLOR; // Control characters like CAN
                        else if (c == 26) col = NON_PRINT_COLOR; // Control characters like SUB
                        else if (c == 27) col = NON_PRINT_COLOR; // Control characters like ESC
                        else if (c == 127) col = NON_PRINT_COLOR; // DEL
                        else {
                            col = ASCII_COLOR;
                            c = 0;
                        }
                    }

                    else if (option->color) {
                        if (c == 0)        col = NULL_BYTE_COLOR;
                        else if (c < 32 || c >= 127) col = NON_PRINT_COLOR;
                        else               col = ASCII_COLOR;
                    }

                    char disp = (c >= 32 && c < 127) ? c : '.';

                    if (option->string && c == 0) {
                        line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, " ");
                    }
                    else line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "%s%c", col, disp);
                    
                    //_buffer[processed + i] = c;
                    char_written += 1;
                }
            }

            // Heatmap bar logic: If heatmap is enabled, calculates the entropy of the current line 
            // and appends a colored bar to visually represent the entropy level.
            if (option->entropie) {
                float entropy = calc_entropy(buffer, line_len);

                const char* bar;
                if (entropy < 2.0)      bar = " ";
                else if (entropy < 4.0) bar = "░";
                else if (entropy < 6.0) bar = "▒";
                else bar = "▓";
                
                const char *col = heatmap_colors[INDEX_MAP(entropy, 0.0f, 8.0f)];

                int space_for_bar = option->buff_size - char_written + 1;

                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "%s%*s%s", col, space_for_bar, "", bar);
            }

            if (!no_newline) line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "\x1b[0m\n");

            // Writes the constructed line to the output (pager or stdout). 
            // Uses fwrite for binary safety, especially if line contains null bytes.
            bool matches_search = (option->search == NULL || option->search[0] == '\0') ||
                                  contains_query_in_line_or_overlap(buffer + processed, line_len, option);

            if (matches_search && !is_space(option->buff_size, (unsigned char *)line)) {
                // Treffer → vorherige Zeile (falls vorhanden) zuerst ausgeben
                if (have_prev && prev_line_pos > 0) {
                    fwrite(prev_line, 1, prev_line_pos, out);
                }

                // aktuelle Zeile ausgeben
                fwrite(line, 1, line_pos, out);

                have_prev = false;   // verbraucht
            }
            else {
                // Kein Treffer → für die nächste Iteration merken
                memcpy(prev_line, line, line_pos);
                prev_line_pos = line_pos;
                have_prev = true;
            }
            


            addr_display += line_len;
            processed += line_len;
        }
    }

    printf("\n");
    // Closes the file handle opened at the beginning of print_hex.
    if (!option->pipeline) fclose(file);
    #undef printf
}


int main(int argc, char *argv[]) {

    // Enable ANSI escape code processing on Windows 10 
    // and later for colored output in the console. (optional on terminal-app)
    #ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif

    // 1. Parse command line arguments.
    options *option = get_options(argc, argv);

    // 2. Perform initial file existence and emptiness checks.
    check_file(option);

    // 3. Execute the hex dump logic.
    print_hex(option);

    // 4. Clean up allocated memory for options structure.
    free(option);
    print_color(RESET, true);

    return EXIT_SUCCESS;
}