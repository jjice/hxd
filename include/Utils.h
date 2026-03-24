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

// --- Old color set (fallback) ---
// #define NON_PRINT_COLOR "\x1b[38;5;187m"   // Grey / Bright Black
// #define NULL_BYTE_COLOR "\x1b[38;5;8m"     // Standard Blau
// #define ADDR_COLOR      "\x1b[38;5;214m"   // Bright Blue
// #define ASCII_COLOR     "\x1b[38;5;11m"    // Cyan
// #define HEADER_COLOR    "\x1b[38;5;222m"   // Bright Yellow
// #define MAGIC_COLOR     "\x1b[38;5;202m"   // Orange Magic Byte Highlight

// --- Modern neutral color set (dark + light terminal friendly) ---
#define NON_PRINT_COLOR "\x1b[38;5;153m"     // Neutral gray for control bytes
#define NULL_BYTE_COLOR "\x1b[38;5;240m"     // Soft gray for null bytes
#define ADDR_COLOR      "\x1b[38;5;67m"      // Muted steel blue for addresses
#define ASCII_COLOR     "\x1b[38;5;75m"      // Calm cyan-blue for printable chars
#define HEADER_COLOR    "\x1b[38;5;110m"     // Soft teal for header / footer
#define MAGIC_COLOR     "\x1b[38;5;179m"     // Warm amber for magic signatures
#define BORDER_COLOR    "\x1b[38;5;109m"     // Subtle border / separator color
#define ANALYSIS_TEXT_COLOR "\x1b[38;5;67m"  // Royal blue for general text
#define ERROR_COLOR     "\x1b[38;5;160m"     // Bright red for errors

#define HIGHLIGHT_COLOR "\x1b[1;97;104m"     // Bright white on bright blue background for search highlights]"

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
    float printable;
    float control;
    float high_byte;
    float whitespace;
} dump_analysis;

bool get_file_metadata(const char *filename, file_metadata *meta);
void format_time_local(time_t value, char *buffer, size_t buffer_size);

void analyse(dump_analysis *analysis, unsigned char *data, size_t len);

#endif
