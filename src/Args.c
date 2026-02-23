/*
 * hxd - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "Args.h"


/**
 * @brief Parses command line arguments and initializes the options structure.
 *
 * @note Implements manual argument parsing instead of using standard library functions (getopt).
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return options* Pointer to the dynamically allocated options structure.
 */
options *get_options(int argc, char *argv[]) {
    // Allocate memory for options structure.
    options *option = (options *) malloc(sizeof(options));
    if (!option) {
        perror("Cant malloc options");
        exit(EXIT_FAILURE);
    }

    // Set default values.
    option->filename = NULL;
    option->buff_size = 16;
    option->ascii = true;
    option->offset_read = 0;
    option->limit_read = 0;
    option->pipeline = false;
    option->color = true;
    option->pager = false;

    // Help message string.
    char help[] = "\nUsage:\n"
                 "\n  (-f) : <filename>                        (-f is optional)"
                 "\n    -b : Linewidth <bytes>                 (default = 16)"
                 "\n    -a : Show ASCII-Table <on/off>         (default = on)"
                 "\n    -o : Offset to start reading <bytes>   (default = 0)"
                 "\n    -l : Limit to stop reading <bytes>     (default = EOF)"
                 "\n    -c : Enable colored output <on/off>    (default = on)"
                 "\n    -p : Enable pager output <on/off>      (default = off)"
                 "\n              more <win> less <linux>\n"
                 "\n    -h : Show this info\n\n";

    char help_short[] = "See -h for more information\n";
    
    // Loop through command line arguments starting from index 1.
    for (int x = 1; x < argc; x++) {
        if (strcmp(argv[x], "-f") == 0) {
            // Handles explicit filename flag.
            option->filename = argv[x + 1];
            x++; // Skip the argument value.
        }
        
        else if(argv[x][0] != '-' && option->filename == NULL) {
            // Handles positional filename argument (argument not starting with '-').
            option->filename = argv[x];
        }
        
        else if (strcmp(argv[x], "-b") == 0) {
            // Buffer size argument parsing.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -b requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            // Robust conversion using strtol for numeric input validation.
            errno = 0;
            char *endptr;
            long val = strtol(argv[x + 1], &endptr, 10);

            if (endptr == argv[x + 1]) {
                fprintf(stderr, "Error: -b requires a numeric value\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            if (errno == ERANGE || val <= 0) {
                fprintf(stderr, "Error: buff_size out of range\n"); // Includes check for negative/zero.
                exit(EXIT_FAILURE);
            }

            option->buff_size = (int)val;
            x++; // Skip the argument value.
        }

        else if(strcmp(argv[x], "-a") == 0){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -a requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("on", argv[x + 1]) == 0) {
                    option->ascii = true;
                    x++;
                }
                // Check for "off"
                else if (strcmp("off", argv[x + 1]) == 0) {
                    option->ascii = false;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument -a <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if(strcmp(argv[x], "-o") == 0){
            // Offset Flag
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else {
                errno = 0;
                char *endptr;
                long val = strtol(argv[x + 1], &endptr, 10);

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: -o requires a numeric value\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for bad characters
                if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
                    fprintf(stderr, "Error: -o contains invalid characters\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for overflow
                if (errno == ERANGE) {
                    fprintf(stderr, "Error: offset is too large or too small\n");
                    exit(EXIT_FAILURE);
                }

                // Check for positiv value
                if (val <= 0) {
                    fprintf(stderr, "Error: offset must be positive\n");
                    exit(EXIT_FAILURE);
                }
                // Set offset
                option->offset_read = (size_t)val;
                x++; // Skip the argument value.
            }
            
        }

        else if(strcmp(argv[x], "-l") == 0){
            // Limit Flag
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -l requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else {
                errno = 0;
                char *endptr;
                long val = strtol(argv[x + 1], &endptr, 10);

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: -l requires a numeric value\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for bad characters
                if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
                    fprintf(stderr, "Error: -l contains invalid characters\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for overflow
                if (errno == ERANGE) {
                    fprintf(stderr, "Error: limit is too large or too small\n");
                    exit(EXIT_FAILURE);
                }

                // Check for positiv value
                if (val <= 0) {
                    fprintf(stderr, "Error: limit must be positive\n");
                    exit(EXIT_FAILURE);
                }
                // Check for higher value than offset
                if (val < (long) option->offset_read) {
                    fprintf(stderr, "Error: limit must be greater than offset\n");
                    exit(EXIT_FAILURE);
                }

                // Set offset
                option->limit_read = (size_t)val;
                x++; // Skip the argument value.
            }
            
        }

        else if(strcmp(argv[x], "-c") == 0){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -c requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("on", argv[x + 1]) == 0) {
                    option->color = true;
                    x++;
                }
                // Check for "off"
                else if (strcmp("off", argv[x + 1]) == 0) {
                    option->color = false;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument -c <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if(strcmp(argv[x], "-p") == 0){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: -p requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("on", argv[x + 1]) == 0) {
                    option->pager = true;
                    x++;
                }
                // Check for "off"
                else if (strcmp("off", argv[x + 1]) == 0) {
                    option->pager = false;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument -p <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if(strcmp(argv[x], "-h") == 0){
            // Help flag.
            printf("%s", help);
            exit(EXIT_FAILURE);
        }

        else {
            // Unrecognized argument leads to help display and exit.
            printf("%s", help_short);
            exit(EXIT_SUCCESS);
        }
    }

        // Final validation: Ensure a filename was provided or a pipe is waiting.
        // Am Ende von get_options, nach der Argument-Schleife:

    if (option->filename == NULL) {
        if (isatty(fileno(stdin))) {
            // interaktiv / kein Input → Fehler
            fprintf(stderr, "No filename given and no data in pipe\n");
            free(option);
            exit(EXIT_FAILURE);
        }
       option->pipeline = true;
    }

    return option;
}