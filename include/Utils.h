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
#define ADDR_COLOR      "\x1b[94m"  // Bright Blue (Für die Adressspalte links)
#define HEX_VAL_COLOR   "\x1b[97m"  // Bright White (Die eigentlichen Daten)
#define NON_PRINT_COLOR "\x1b[90m"  // Grey / Bright Black (Für Punkte oder 00er Werte)
#define ASCII_COLOR     "\x1b[36m"  // Cyan (Für die Text-Vorschau rechts)
#define HEADER_COLOR    "\x1b[93m"  // Bright Yellow (Für die Spaltenüberschriften)
#define PIPE_COLOR      "\x1b[91m"  // Bright Red (Für Trennstriche oder spezielle Offsets)
#define NULL_BYTE_COLOR "\x1b[34m"    // Standard Blau (meist recht dunkel)

#include <stdio.h>
#include <stdbool.h>

static inline void print_color(const char* color_code, bool enable_color) {
    if (enable_color) {
        printf("%s", color_code);
    }
}

static inline FILE* open_pager(void) {
    // 1. Versuch: less mit -R (für Farben)
    // 'where' (Windows) oder 'which' (Linux) prüfen ist komplex, 
    // einfacher ist es, den Befehl direkt zu probieren.
    FILE* pager = popen("less -R", "w");
    if (pager) return pager;

    // 2. Versuch: more (Standard auf Windows/Linux)
    pager = popen("more", "w");
    if (pager) return pager;

    // Fallback: Einfach STDOUT (kein Pager)
    return stdout;
}

#endif