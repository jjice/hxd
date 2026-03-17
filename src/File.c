/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stdlib.h>
#include <stdio.h>
#include "File.h"

#define MAX_BUFF_SIZE 16384

// Read a chunk of the file into the buffer, starting from read_start and respecting read_limit
void read_stream_to_buffer(int *out_read, FILE *file, size_t read_start, size_t read_limit, unsigned char *_buffer, bool _no_seek) {
    // Return if file is stdin
    if (file == stdin) {
        *out_read = fread(_buffer, 1, MAX_BUFF_SIZE, file);
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
}

// Check if file exists and is not empty, also checks if offset, read and limit are in range of filesize
void check_file(options *option) {

    if (option->pipeline == true) return;

    FILE *fp = fopen(option->filename, "rb");

    if (fp == NULL) {
        // File not found or permission denied.
        perror("File not found / No permission to read");
        exit(EXIT_FAILURE);
    } else {
        // Check for zero size
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        if (file_size == 0) {
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
        if (file_size < option->offset_read + option->read_size) {
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
void find_extrema(unsigned char *_max, unsigned char *_min, FILE *file) {
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

        for (size_t index = 0; index < (size_t)read_bytes; index++) {
            if (*_min > _buffer[index]) *_min = _buffer[index];
            if (*_max < _buffer[index]) *_max = _buffer[index];

            if (*_min == 0 && *_max == 255) {
                // Reset file position before early exit so the dump can read from the beginning again.
                fseek(file, 0, SEEK_SET);
                return;
            }
        }
    }

    // Reset file position to beginning
    fseek(file, 0, SEEK_SET);
}
