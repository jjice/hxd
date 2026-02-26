/*
 * hxd - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "Args.h"

// issatty and fileno for Windows compatibility
#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

#define MAX_SEARCH_LEN 1024

static char search[MAX_SEARCH_LEN] = {0}; 

// Function to read a chunk of the file into the buffer, 
// starting from read_start and respecting read_limit
options *get_options(int argc, char *argv[]) {
    // Allocate memory for options structure.
    options *option = (options *) malloc(sizeof(options));
    if (!option) {
        perror("Cant malloc options");
        exit(EXIT_FAILURE);
    }

    // Set default values.
    option->filename = NULL;
    option->heatmap = 0;
    option->buff_size = 16;
    option->ascii = true;
    option->offset_read = 0;
    option->limit_read = 0;
    option->pipeline = false;
    option->color = true;
    option->color = true;
    option->pager = false;
    option->entropie = false;
    option->string = false;
    option->raw = false;

    // Help message string.
    static const char *help =
        "\n"
        "hxd - A modern hex dumper\n"
        "Copyright (c) 2026 Joshua Jallow   MIT License\n"
        "\n"
        "Usage:\n"
        "  hxd [options] [filename]\n"
        "  cat file.bin | hxd [options]\n"
        "\n"
        "Options:\n"
        "  -f,  --file    <filename>         Input file (optional if data comes from stdin)\n"
        "  -hm, --heatmap <adaptiv|fixed>    Show Colors as Heatmap with 16 colors (default: none)\n"
        "  -w,  --width   <num>              Bytes per line (default: 16) (0 -> no new line)\n"
        "  -a,  --ascii   <on|off>           Show ASCII representation column (default: on)\n"
        "  -o,  --offset  <num>              Start reading at this byte offset (default: 0)\n"
        "  -l,  --limit   <num>              Stop after this many bytes (default: read to EOF)\n"
        "  -c,  --color   <on|off>           Enable syntax highlighting / colors (default: on)\n"
        "  -s,  --string  <on|off>           Enable string highlighting / strings (default: off)\n"
        "  -p,  --pager   <on|off>           Pipe output through pager (more/less) (default: off)\n"
        "  -e,  --entropy <on|off>           Show entropy graph per line (default: off)\n"
        "  -sa, --search-ascii  '<ascii chars>'  Search for all lines with input str as ascii\n"
        "  -sh, --search-hex    '<hex chars>'    Search for all line with input str as hex\n"
        "  -ro, --raw                        Raw output to console | file (pipe)\n"
        "  -h,  --help                       Show this help message and exit\n"
        "\n"
        "Examples:\n"
        "  hxd image.png                     # normal file\n"
        "  hxd -w 32 -a off secret.bin       # 32 bytes/line, kein ASCII\n"
        "  hxd file | less -R                # output inside Pager with colors\n"
        "  echo 'Hello World' | hxd          # pipeline to hxd\n"
        "  hxd -w 0 -ro test > o.txt         # raw output without newlines into a file\n"
        "\n"
        "Notes:\n"
        "  * Offsets and limits must be positive integers.\n"
        "  * --offset and --limit cannot be combined in a way that limit < offset.\n"
        "  * When reading from stdin (pipe), filename is not required.\n"
        "\n";

    char help_short[] = "See -h for more information\n";
    
    // Loop through command line arguments starting from index 1.
    for (int x = 1; x < argc; x++) {
        if (strcmp(argv[x], "-f") == 0 || (strcmp(argv[x], "--file") == 0)) {
            // Handles explicit filename flag.
            option->filename = argv[x + 1];
            x++; // Skip the argument value.
        }
        
        else if(argv[x][0] != '-' && option->filename == NULL) {
            // Handles positional filename argument (argument not starting with '-').
            option->filename = argv[x];
        }
        
        else if (strcmp(argv[x], "-w") == 0 || (strcmp(argv[x], "--width") == 0)) {
            // Buffer size argument parsing.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: width requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            // Robust conversion using strtol for numeric input validation.
            errno = 0;
            char *endptr;
            long val = strtol(argv[x + 1], &endptr, 10);

            if (endptr == argv[x + 1]) {
                fprintf(stderr, "Error: width requires a numeric value\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            if (errno == ERANGE || val < 0) {
                fprintf(stderr, "Error: width out of range\n"); // Includes check for negative/zero.
                exit(EXIT_FAILURE);
            }
            if (val > 512) {
                fprintf(stderr, "Error: width out of range [max. 512]\n"); // Includes check for negative/zero.
                exit(EXIT_FAILURE);
            }

            option->buff_size = (int)val;
            x++; // Skip the argument value.
        }

        else if (strcmp(argv[x], "-hm") == 0 || (strcmp(argv[x], "--heatmap") == 0)){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Heatmap requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("adaptiv", argv[x + 1]) == 0) {
                    option->heatmap = 1;
                    x++;
                }
                // Check for "off"
                else if (strcmp("fixed", argv[x + 1]) == 0) {
                    option->heatmap = 2;
                    x++;
                }
                // Check for "off"
                else if (strcmp("none", argv[x + 1]) == 0) {
                    option->heatmap = 0;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument for Heatmap <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-a") == 0 || (strcmp(argv[x], "--ascii") == 0)){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: ASCII requires an argument\n");
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
                    printf("Invalid argument for ASCII <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-o") == 0 || (strcmp(argv[x], "--offset") == 0)) {
            // Offset Flag
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: offset requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else {
                errno = 0;
                char *endptr;
                long val = strtol(argv[x + 1], &endptr, 10);

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: offset requires a numeric value\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for bad characters
                if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
                    fprintf(stderr, "Error: offset contains invalid characters\n");
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

        else if (strcmp(argv[x], "-l") == 0 || (strcmp(argv[x], "--limit") == 0)) {
            // Limit Flag
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: limit requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else {
                errno = 0;
                char *endptr;
                long val = strtol(argv[x + 1], &endptr, 10);

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: limit requires a numeric value\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for bad characters
                if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
                    fprintf(stderr, "Error: limit contains invalid characters\n");
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

        else if (strcmp(argv[x], "-c") == 0 || (strcmp(argv[x], "--color") == 0)){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: color requires an argument\n");
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
                    printf("Invalid argument for color <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-s") == 0 || (strcmp(argv[x], "--string") == 0)){
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: string requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("on", argv[x + 1]) == 0) {
                    option->string = true;
                    x++;
                }
                // Check for "off"
                else if (strcmp("off", argv[x + 1]) == 0) {
                    option->string = false;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument for string <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-p") == 0 || (strcmp(argv[x], "--pager") == 0)) {
            // ASCII flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: pager requires an argument\n");
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
                    printf("Invalid argument for pager <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-e") == 0 || (strcmp(argv[x], "--entropy") == 0)){
            // Entropy flag toggle argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: entropy requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for "on"
                if (strcmp("on", argv[x + 1]) == 0) {
                    option->entropie = true;
                    x++;
                }
                // Check for "off"
                else if (strcmp("off", argv[x + 1]) == 0) {
                    option->entropie = false;
                    x++;
                }
                // Invalid value provided.
                else {
                    printf("Invalid argument for entropy <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-sa") == 0 || (strcmp(argv[x], "--search-ascii") == 0)){
            // Search quarry argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Search requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            if (strlen(argv[x + 1]) > MAX_SEARCH_LEN) {
                printf("Search string is too long ( .... > %d )\n", (int) MAX_SEARCH_LEN);
                exit(EXIT_FAILURE);
            }
            
            strcpy(search, argv[x + 1]);

            x++;
        }

        else if (strcmp(argv[x], "-sh") == 0 || (strcmp(argv[x], "--search-hex") == 0)){
            // Search quarry argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Search requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            if (strlen(argv[x + 1]) > MAX_SEARCH_LEN) {
                printf("Search string is too long ( .... > %d )\n", (int) MAX_SEARCH_LEN);
                exit(EXIT_FAILURE);
            }
            
            strcpy(search, argv[x + 1]);

            x++;
        }

        else if (strcmp(argv[x], "-ro") == 0 || (strcmp(argv[x], "--raw") == 0)){
            // RAW flag toggle argument.
            option->raw = true;
        }

        else if (strcmp(argv[x], "-h") == 0 || (strcmp(argv[x], "--help") == 0)) {
            // Help flag.
            printf("%s", help);
            exit(EXIT_SUCCESS);
        }

        else {
            // Unrecognized argument leads to help display and exit.
            printf("Unrecognized argument: %s\n%s", argv[x], help_short);
            exit(EXIT_FAILURE);
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

    // Deactivate future flags for true raw 
    if (option->buff_size == 0 && !option->raw) {
        fprintf(stderr, "Error: Width can only be 0 if <-ro> has been set\n");
        printf("%s", help_short);
        exit(EXIT_FAILURE);
    }
    
    if (option->raw) {
        option->color = false;
        option->ascii = false;
        option->heatmap = false;
    }

    return option;
}

