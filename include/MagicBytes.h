/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef MAGIC_SIGNATURES_H
#define MAGIC_SIGNATURES_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint8_t *bytes;
    size_t         len;
    int            offset;
    const char    *extension;   // may be NULL
    const char    *description; // may be NULL
} MagicSignature;

#define MAGIC_MAX_LEN 16

extern const MagicSignature magic_signatures[];
extern const size_t magic_signatures_count;

#endif
