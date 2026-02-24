/*
 * hxd - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef UTILS_H
#define UTILS_H

// Basiseffekte
#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"
#define DIM             "\x1b[2m"  // Sehr nützlich für Unwichtiges!

// "Modern UI" Farben (Helle Varianten wirken oft sauberer)
#define NON_PRINT_COLOR "\x1b[38;5;179m"  // Grey / Bright Black (Für Punkte oder 00er Werte)
#define NULL_BYTE_COLOR "\x1b[38;5;8m"  // Standard Blau (meist recht dunkel)
#define ADDR_COLOR      "\x1b[38;5;214m"  // Bright Blue (Für die Adressspalte links)
#define HEX_VAL_COLOR   "\x1b[38;5;95m"  // Bright White (Die eigentlichen Daten)
#define ASCII_COLOR     "\x1b[38;5;11m"  // Cyan (Für die Text-Vorschau rechts)
#define HEADER_COLOR    "\x1b[38;5;214m"  // Bright Yellow (Für die Spaltenüberschriften)

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

#include <stdio.h>
#include <stdbool.h>


#ifdef _WIN32
    // Windows nutzt Unterstriche für diese POSIX-Funktionen
    #define popen _popen
    #define pclose _pclose
    
    // Verhindert die Warnung wegen fopen
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

static inline void print_color(const char* color_code, bool enable_color) {
    if (enable_color) {
        printf("%s", color_code);
    }
}

static inline FILE* open_pager(void) {
    FILE* pager = popen("less -R", "w");
    if (pager) return pager;

    pager = popen("more", "w");
    if (pager) return pager;

    return stdout;
}

#endif