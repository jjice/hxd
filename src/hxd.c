/*
 * hxd - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "Utils.h"
#include "Args.h"
#include "MagicBytes.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_BUFF_SIZE 16384

/** 
 * This documentation has been generated with ai. Use with caution!
 */


/**
 * @brief Reads a chunk of data from an open file handle into a dynamically allocated buffer.
 *
 *
 * @param filename File path (ignored, as the FILE* handle is already passed).
 * @param out_read Pointer to store the actual number of bytes read.
 * @param buff_size The maximum buffer size to read.
 * @param file The already opened FILE* handle.
 * @param read_start Start Byte to read
 * @return const char* Pointer to the dynamically allocated buffer (must be freed by the caller).
 */


static unsigned char buffer[16384] = {0}; // Static buffer to avoid dynamic allocation overhead. Size is MAX_BUFF_SIZE.

void read_stream_to_buffer(int *out_read, FILE *file, size_t read_start, size_t read_limit, unsigned char *_buffer) {
    
    if (file == stdin) {
        // Bei stdin: Offset ignorieren, einfach lesen 
        *out_read = fread(buffer, 1,  MAX_BUFF_SIZE, file);
        return;
    }
    
    // Comulative offset variable for file cursor
    static bool offset_apply = false;   // If offset was firt time set -> true

    // Apply start_offset
    if (!offset_apply) {
        offset_apply = true;
        fseek(file, read_start, SEEK_SET);
    }

    // Current File cursor position
    size_t current_pos = ftell(file);

    // Absolut limit to read
    size_t end_limit = read_start + read_limit;

    // Remaining bytes to read
    size_t remaining = 0;
    if (current_pos < end_limit) {
        remaining = end_limit - current_pos;
    }

    // If remaining is smaller than buff_size: read remaining else read buffsize
    size_t to_read = remaining < (size_t)MAX_BUFF_SIZE ? remaining : (size_t)MAX_BUFF_SIZE;

    // Read filecontent to buffer, output read bytes
    *out_read = fread(_buffer, 1, to_read, file);
    return;
}

/**
 * @brief Checks if a file exists and verifies that its size is non-zero.
 *
 * @note Uses fseek/ftell to determine file size, which is efficient for checking emptiness.
 * @note Error message "File is empty" is printed to stdout, not stderr, which is non-standard for errors.
 *
 * @param filename The path to the file to be checked.
 */
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
        // Ceck if filesize is in range of offset till limit
        if (file_size < option->offset_read + option->limit_read) {
            printf("Filesize is out of range, check offset|limit");
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

void find_extrema (unsigned char *max, unsigned char *min, FILE *file) {
    unsigned char _buffer[MAX_BUFF_SIZE] = {0};
    int read_bytes = 0;

    *min = 255;
    *max = 0;

    while (1) {
        read_stream_to_buffer(&read_bytes, file, 0, 0, _buffer);
        if (read_bytes <= 0) break;

        for (size_t index = 0; index < read_bytes; index++) {
            if (*min > _buffer[index]) *min = _buffer[index];
            if (*max < _buffer[index]) *max = _buffer[index];

            if (*min == 0 && *max == 255) return;
        }
    }
}

/**
 * @brief Executes the core hex dump loop, handling output formatting and file reading.
 *
 * @param option Pointer to the configured options structure.
 */
void print_hex(options *option){

    FILE *out = stdout; // Standardmäßig normaler Output
    if (option->pager) {
        out = open_pager();
    }

    #define printf(...) fprintf(out, __VA_ARGS__)

    if (option->raw) {printf("RAW\n"); goto SKIP_HEADER;}

    int addr_display = 0;    // Starting memory address to display for the current row.
    
    // Apply starting addr
    addr_display += option->offset_read;

    // --- Header Formatting ---
    print_color(HEADER_COLOR, option->color);
    if (option->pipeline) printf("\nHexdump for <pipe>:\n\n");
    else printf("\nHexdump for <%s>:\n\n", option->filename);
    print_color(RESET, option->color);
    
    // Prints spaces to align column headers with the hex data area.
    printf("%*s", 11, " ");
    // Prints column headers (offsets within the line: 00, 01, 02...).
    for (int i = 0; i < option->buff_size; i++) {
        printf("%02X ", i);
    }
    
    // Calculates the total length of the separating line based on buff_size and ASCII flag.
    int n = 0;
    if (option->ascii == true) n = 18 + option->buff_size * 3 + option->buff_size; 
    else n = 10 + option->buff_size * 3;
    
    fputc('\n', out);
    
    // Prints the horizontal separator line. Includes logic to place a '+' if ASCII is enabled.
    for (int i = 0; i < n; i++) {
        if (option->ascii == true && i == 15 + option->buff_size * 3) {
            fputc('+', out);
            //n -= 1; // Adjusts loop count to maintain overall length.
        }
        fputc('-', out);
    }
    fputc('\n', out);

SKIP_HEADER:    

    // FILE handler
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
    
    char line[4096];         // Buffer for each line
    int line_pos = 0;        // Line position
    int bytes_read = 0;      // Bytes read in last read_file_to_buffer call.


    // --- Hex Output Loop ---
    while (1) {
        int bytes_read = 0;
        read_stream_to_buffer(&bytes_read, file, option->offset_read, option->limit_read, buffer);
        if (bytes_read == 0) break;

        int processed = 0;
        while (processed < bytes_read) {
            int line_len = option->buff_size;
            if (processed + line_len > bytes_read) {
                line_len = bytes_read - processed;
            }

            line_pos = 0;

            // Adressbar
            if (option->raw) continue;
            else if (option->color) {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                    "%s%08X%s | ", RESET, addr_display, RESET);
            } 
            else {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                    "%08X | ", addr_display);
            }

            // HEX output
            for (int i = 0; i < option->buff_size; i++) {
                if (i < line_len) {
                    unsigned char b = buffer[processed + i];

                    const char *col = RESET;

                    if (option->heatmap == 2) {
                        col = heatmap_colors[(int)(b / 16)];
                    }

                    else if (option->color) {
                        if (b == 0)        col = NULL_BYTE_COLOR;
                        else if (b >= 32 && b < 127) col = ASCII_COLOR;
                        else               col = NON_PRINT_COLOR;
                    }                    

                    line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                         "%s%02x ", col, b);
                } 
                else {
                    line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "   ");
                }
            }

            // ASCII
            if (option->ascii) {
                line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "\x1b[0m    |   ");

                unsigned char min, max;

                if (option->heatmap == 1) {
                    find_extrema(&max, &min, file);
                }

                for (int i = 0; i < line_len; i++) {
                    unsigned char c = buffer[processed + i];
                    const char *col = RESET;

                    if (option->heatmap == 2) {
                        col = heatmap_colors[(int)(c / 16)];
                    }

                    else if (option->color) {
                        if (c == 0)        col = NULL_BYTE_COLOR;
                        else if (c < 32 || c >= 127) col = NON_PRINT_COLOR;
                        else               col = ASCII_COLOR;
                    }

                    char disp = (c >= 32 && c < 127) ? c : '.';
                    line_pos += snprintf(line + line_pos, sizeof(line) - line_pos,
                                         "%s%c", col, disp);
                }
            }

            // Zeile abschließen
            line_pos += snprintf(line + line_pos, sizeof(line) - line_pos, "\x1b[0m\n");

            // EINZIGER fprintf pro Zeile
            fwrite(line, 1, line_pos, out);
            // oder: fputs(line, out);  (wenn du sicher bist, dass kein \0 drin ist)

            addr_display += line_len;
            processed += line_len;
        }
    }

    printf("\n");
    // Closes the file handle opened at the beginning of print_hex.
    if (!option->pipeline) fclose(file);
    #undef printf // Wichtig: Am Ende der Funktion das Makro wieder löschen
}


int main(int argc, char *argv[]) {
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