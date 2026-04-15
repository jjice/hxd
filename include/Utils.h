/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"
#define DIM             "\x1b[2m"

// --- Modern neutral color set (dark + light terminal friendly) ---


extern char* CONTROL_COLOR;          // Neutral gray for control bytes
extern char* NULL_BYTE_COLOR;        // Soft gray for null bytes
extern char* ADDR_COLOR;              // Muted steel blue for addresses
extern char* ASCII_COLOR;             // Calm cyan-blue for printable chars
extern char* EXTENDED_ASCII_COLOR;   // Soft teal for extended ASCII
extern char* HEADER_COLOR;           // Soft teal for header / footer
extern char* MAGIC_COLOR;            // Warm amber for magic signatures
extern char* BORDER_COLOR;           // Subtle border / separator color
extern char* ANALYSIS_TEXT_COLOR;     // Royal blue for general text
extern char* ERROR_COLOR;              // Light red for errors

extern char* HIGHLIGHT_COLOR;        // Bright blue background with bright white text for highlights

extern const char *heatmap_colors[16];

#define INDEX_MAP(value, min, max) (int) (((float)(value - min) / (float)(max - min)) * 15.0 + 0.5)

#ifdef _WIN32
    // Windows-specific definitions for popen and fileno
    #define popen _popen
    #define pclose _pclose
    
    // Disable secure warnings for functions like fopen, popen, etc.
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

void print_color(const char *color_code, bool enable_color);
FILE *open_pager(void);
int is_space(size_t n, unsigned char *str);

// Format a local timestamp into a readable string.
typedef struct {
    bool exists;
    bool has_created;
    bool has_modified;
    size_t file_size;
    time_t created_at;
    time_t modified_at;
} file_metadata;

// Parses the analytic stuff for the footer, including magic byte summary and byte distribution statistics.
typedef struct {
    size_t total_bytes;
    size_t zero_bytes;
    size_t line_count;
    int magic_count;
    size_t printable;
    size_t control;
    size_t extended_ascii;
} dump_analysis;

bool get_file_metadata(const char *filename, file_metadata *meta);
void format_time_local(time_t value, char *buffer, size_t buffer_size);

void analyse(dump_analysis *analysis, unsigned char *data, size_t len);

#endif
