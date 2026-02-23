#ifndef ARGS_H
#define ARGS_H

#include <stdlib.h>
#include <stdbool.h>

// options (no getopt)
/**
 * @struct options
 * @brief Structure to hold the configuration for the hex dump utility.
 * This manually replaces the functionality provided by standard libraries like getopt.
 */
typedef struct {
    char *filename;     // Path to the file to be read.
    int buff_size;      // The size of the buffer/line to display (e.g., 16 bytes).
    bool ascii;         // Flag to determine if the ASCII representation column should be shown.
    size_t offset_read; // Bytes to skip until print
    size_t limit_read;  // Count of bytes to be read/displayed
    bool pipeline;      // Flag to determine if a pipeline is active (bypass check_file)
} options;

options *get_options(int argc, char *argv[]);

#endif