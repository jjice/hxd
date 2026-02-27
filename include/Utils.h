/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"
#define DIM             "\x1b[2m"

#define NON_PRINT_COLOR "\x1b[38;5;187m"     // Grey / Bright Black 
#define NULL_BYTE_COLOR "\x1b[38;5;8m"      // Standard Blau 
#define ADDR_COLOR      "\x1b[38;5;214m"    // Bright Blue 
//#define HEX_VAL_COLOR   "\x1b[38;5;95m"     // Bright White 
#define ASCII_COLOR     "\x1b[38;5;11m"     // Cyan 
#define HEADER_COLOR    "\x1b[38;5;222m"    // Bright Yellow 
#define MAGIC_COLOR     "\x1b[38;5;202m"    // Orange Magic Byte Highlight

static const char *heatmap_colors[16] = {
    "\x1b[38;5;232m",  // 0
    "\x1b[38;5;235m",
    "\x1b[38;5;17m",
    "\x1b[38;5;19m",
    "\x1b[38;5;20m",
    "\x1b[38;5;26m",
    "\x1b[38;5;32m",
    "\x1b[38;5;44m",
    "\x1b[38;5;76m",
    "\x1b[38;5;112m",
    "\x1b[38;5;190m",
    "\x1b[38;5;226m",
    "\x1b[38;5;220m",
    "\x1b[38;5;214m",
    "\x1b[38;5;202m",
    "\x1b[38;5;196m"   // 15
};

#define INDEX_MAP(value, min, max) (int) (((float)(value - min) / (float)(max - min)) * 15.0 + 0.5)

#include <stdio.h>
#include <stdbool.h>


#ifdef _WIN32
    // Windows-specific definitions for popen and fileno
    #define popen _popen
    #define pclose _pclose
    
    // Disable secure warnings for functions like fopen, popen, etc.
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

// Function prototypes
static inline void print_color(const char* color_code, bool enable_color) {
    
    if (enable_color) {
        printf("%s", color_code);
    }
}

// Function to open a pager (like less or more) for output. 
// Falls back to stdout if no pager is available.
static inline FILE* open_pager(void) {
    
    FILE* pager = popen("less -R", "w");
    if (pager) return pager;

    pager = popen("more", "w");
    if (pager) return pager;

    return stdout;
}

// Return 0 if a single char from a given string is not a space char ' ' 
static inline int is_space(size_t n, unsigned char *str) {
    unsigned char *end = str + n;
    while (str < end) {
        if (*str++ != 32) return 0;
    }
    return 1;
}

#endif