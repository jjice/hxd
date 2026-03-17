/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "Args.h"

void read_stream_to_buffer(int *out_read, FILE *file, size_t read_start, size_t read_limit, unsigned char *_buffer, bool _no_seek);
void check_file(options *option);
void find_extrema(unsigned char *_max, unsigned char *_min, FILE *file);

#endif
