/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "Args.h"
#include "version.h"

// issatty and fileno for Windows compatibility
#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

// Get *endptr as suffix from strtol (fist char after number)
size_t get_suffix(const char *endchar) {
    if (*endchar == '\0') return 1;
    
    switch((unsigned char) *endchar) {
        // K | k
        case 75:
        case 107:
            return 1e3;
            break;

        // M
        case 77:
            return 1e6;
            break;

        // G
        case 71:
            return 1e9;
            break;

        // Found suffix but not defined
        default:
            return 1;
            break;
    }
}

static void fail_search_parse(const char *value) {
    fprintf(stderr, "Error: invalid search value <%s>\n", value);
    exit(EXIT_FAILURE);
}

static unsigned char *alloc_search_buffer(size_t len) {
    unsigned char *buffer = malloc(len > 0 ? len : 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed for search string\n");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

static void parse_ascii_search(const char *value, const char *body, options *option) {
    size_t body_len = strlen(body);
    if (body_len == 0 || body_len > MAX_SEARCH_LEN) {
        fail_search_parse(value);
    }

    option->search = alloc_search_buffer(body_len);
    memcpy(option->search, body, body_len);
    option->search_len = body_len;
}

static void parse_numeric_search(const char *value, const char *body, int base, options *option) {
    size_t body_len = strlen(body);
    unsigned char *parsed = alloc_search_buffer(MAX_SEARCH_LEN);
    size_t count = 0;
    size_t i = 0;

    while (i < body_len) {
        while (i < body_len && (isspace((unsigned char)body[i]) || body[i] == ',')) i++;
        if (i >= body_len) break;

        size_t start = i;
        while (i < body_len && !isspace((unsigned char)body[i]) && body[i] != ',') i++;

        char token[32] = {0};
        size_t token_len = i - start;
        if (token_len == 0 || token_len >= sizeof(token)) {
            free(parsed);
            fail_search_parse(value);
        }

        memcpy(token, body + start, token_len);

        char *endptr = NULL;
        errno = 0;
        unsigned long parsed_value = strtoul(token, &endptr, base);
        if (errno == ERANGE || endptr == token || *endptr != '\0' || parsed_value > 255 || count >= MAX_SEARCH_LEN) {
            free(parsed);
            fail_search_parse(value);
        }

        parsed[count++] = (unsigned char)parsed_value;
    }

    if (count == 0) {
        free(parsed);
        fail_search_parse(value);
    }

    option->search = parsed;
    option->search_len = count;
}

static void parse_hex_search(const char *value, const char *body, options *option) {
    size_t body_len = strlen(body);
    bool has_separators = false;

    if (body_len == 0) {
        fail_search_parse(value);
    }

    for (size_t i = 0; i < body_len; i++) {
        if (isspace((unsigned char)body[i]) || body[i] == ',') {
            has_separators = true;
            break;
        }
    }

    if (has_separators) {
        parse_numeric_search(value, body, 16, option);
        return;
    }

    if ((body_len % 2) != 0 || (body_len / 2) > MAX_SEARCH_LEN) {
        fail_search_parse(value);
    }

    option->search = alloc_search_buffer(body_len / 2);
    option->search_len = body_len / 2;

    for (size_t i = 0; i < option->search_len; i++) {
        char token[3] = {body[i * 2], body[i * 2 + 1], '\0'};
        char *endptr = NULL;
        errno = 0;
        unsigned long parsed_value = strtoul(token, &endptr, 16);
        if (errno == ERANGE || endptr == token || *endptr != '\0') {
            free(option->search);
            option->search = NULL;
            option->search_len = 0;
            fail_search_parse(value);
        }
        option->search[i] = (unsigned char)parsed_value;
    }
}

static void parse_search_argument(const char *value, options *option) {
    if (strncmp(value, "a:", 2) == 0) {
        parse_ascii_search(value, value + 2, option);
        return;
    }

    if (strncmp(value, "b:", 2) == 0) {
        parse_numeric_search(value, value + 2, 2, option);
        return;
    }

    if (strncmp(value, "d:", 2) == 0) {
        parse_numeric_search(value, value + 2, 10, option);
        return;
    }

    if (strncmp(value, "x:", 2) == 0) {
        parse_hex_search(value, value + 2, option);
        return;
    }

    fail_search_parse(value);
}

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
    option->pipeline = false;
    option->output_mode = 0;
    option->heatmap = 0;
    option->buff_size = 16;
    option->grouping = 1;
    option->ascii = true;
    option->color = true;
    option->string = false;
    option->entropie = false;
    option->skip_header = false;
    option->skip_zero = false;
    option->offset_read = 0;
    option->limit_read = 0;
    option->read_size = 0;
    option->search = NULL;
    option->search_len = 0;
    option->pager = false;
    option->raw = false;

    // Help message string.
    static const char *help =
        "\n"
        "hxed - A modern hex viewer\n"
        "Copyright (c) 2026 Joshua Jallow   MIT License\n"
        "\n"
        "Usage:\n"
        "  hxed [options] [filename]\n"
        "  cat file.bin | hxed [options]\n"
        "\n"
        "Input:\n"
        "  -f,  --file            <filename>         Input file (optional if data comes from stdin)\n"
        "\n"
        "Display:\n"
        "  -m,  --mode            <num|'name'>       Output mode (0 = hex, 1 = bin, 2 = oct, 3 = dec) (default: 0|hex)\n"
        "  -hm, --heatmap         <adaptiv|fixed>    Show colors as heatmap with 16 colors (default: none)\n"
        "  -w,  --width           <num>              Bytes per line (default: 16) (0 -> no new line)\n"
        "  -g,  --grouping        <num>              Grouping size (default: 1, 0 = no spaces)\n"
        "  -a,  --ascii                              Toggle ASCII representation column (default: on)\n"
        "  -c,  --color                              Toggle syntax highlighting / colors (default: on)\n"
        "  -s,  --string                             Toggle string highlighting (default: off)\n"
        "  -e,  --entropy                            Toggle entropy graph per line (default: off)\n"
        "  -th, --toggle-header                      Toggle header, footer and magic byte detection (default: on)\n"
        "  -sz, --skip-zero                          Toggle skip-zero lines (default: off)\n"
        "\n"
        "Read Range:\n"
        "  -o,  --offset          <num>              Start reading at this byte offset (default: 0)\n"
        "  -l,  --limit           <num|hex>          Stop at specific byte (default: read to EOF)\n"
        "  -r,  --read-size       <num>              Stop after this many bytes (default: read to EOF)\n"
        "\n"
        "Search:\n"
        "  -se, --search          <pattern>          Search and print matching lines only\n"
        "                                              a:<text>      | d:<num[,..]>\n"
        "                                              b:<bits[,..]> | x:<hex>\n"
        "\n"
        "Output:\n"
        "  -p,  --pager                              Toggle pager output (default: off)\n"
        "  -ro, --raw                                Raw output to console | file (pipe)\n"
        "\n"
        "Info:\n"
        "  -h,  --help                               Show this help message and exit\n"
        "  -v,  --version                            Show version information and exit\n"
        "\n"
        "Examples:\n"
        "  hxed image.png                     # normal file\n"
        "  hxed -w 32 -a secret.bin           # 32 bytes/line, toggle ASCII off\n"
        "  hxed -c file | less -R             # toggle colors off\n"
        "  echo 'Hello World' | hxed          # pipeline to hxed\n"
        "  hxed -w 0 -ro test > o.txt         # raw output without newlines into a file\n"
        "  hxed -s data.bin                   # with string highlighting\n"
        "  hxed -se a:Hello file.bin          # ascii search\n"
        "  hxed -se x:48656c6c6f file.bin     # hex search\n"
        "  hxed -se b:01001000,01101001 file  # binary byte search\n"
        "  hxed -se d:72,101,108,108,111 file # decimal byte search\n"
        "\n"
        "Notes:\n"
        "  * Offsets and limits must be positive integers.\n"
        "  * Magic byte detection is inactive if offset is set\n"
        "  * --offset and --limit cannot be combined in a way that limit < offset.\n"
        "  * When reading from stdin (pipe), filename is not required.\n"
        "  * Toggle flags flip the current default state.\n"
        "\n";

    const char help_short[] = "See -h for more information\n";
    
    // Loop through command line arguments starting from index 1.
    for (int x = 1; x < argc; x++) {
        if (strcmp(argv[x], "-f") == 0 || (strcmp(argv[x], "--file") == 0)) {
            // Handles explicit filename flag.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: filename requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            option->filename = argv[x + 1];
            x++; // Skip the argument value.
        }
        
        else if(argv[x][0] != '-' && option->filename == NULL) {
            // Handles positional filename argument (argument not starting with '-').
            option->filename = argv[x];
        }

        else if (strcmp(argv[x], "-m") == 0 || (strcmp(argv[x], "--mode") == 0)){
            // Output mode argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Output mode requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for next arg
                if (strcmp("0", argv[x + 1]) == 0 || strcmp("hex", argv[x + 1]) == 0) {
                    option->output_mode = 0;
                    x++;
                }
                // Check for next arg
                else if (strcmp("1", argv[x + 1]) == 0 || strcmp("bin", argv[x + 1]) == 0) {
                    option->output_mode = 1;
                    x++;
                }
                // Check for next arg
                else if (strcmp("2", argv[x + 1]) == 0 || strcmp("oct", argv[x + 1]) == 0) {
                    option->output_mode = 2;
                    x++;
                }
                // Check for next arg
                else if (strcmp("3", argv[x + 1]) == 0 || strcmp("dec", argv[x + 1]) == 0) {
                    option->output_mode = 3;
                    x++;
                }
                // Unrecognized value provided.
                else {
                    printf("Unrecognized argument for Output mode <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
        }

        else if (strcmp(argv[x], "-hm") == 0 || (strcmp(argv[x], "--heatmap") == 0)){
            // Heatmap argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Heatmap requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else{
                // Check for next arg
                if (strcmp("adaptiv", argv[x + 1]) == 0) {
                    option->heatmap = 1;
                    x++;
                }
                // Check for next arg
                else if (strcmp("fixed", argv[x + 1]) == 0) {
                    option->heatmap = 2;
                    x++;
                }
                // Check for next arg
                else if (strcmp("none", argv[x + 1]) == 0) {
                    option->heatmap = 0;
                    x++;
                }
                // Unrecognized value provided.
                else {
                    printf("Unrecognized argument for Heatmap <%s>\n", argv[x + 1]);
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }
            }
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
                fprintf(stderr, "Error: width out of range\n");
                exit(EXIT_FAILURE);
            }
            if (val > 128) {
                fprintf(stderr, "Error: width out of range [max. 128]\n");
                exit(EXIT_FAILURE);
            }

            option->buff_size = (int)val;
            x++;
        }

        else if (strcmp(argv[x], "-g") == 0 || (strcmp(argv[x], "--grouping") == 0)) {
            // Grouping argument parsing.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: grouping requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            errno = 0;
            char *endptr;
            long val = strtol(argv[x + 1], &endptr, 10);

            if (endptr == argv[x + 1]) {
                fprintf(stderr, "Error: grouping requires a numeric value\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            if (errno == ERANGE || val < 0) {
                fprintf(stderr, "Error: grouping out of range\n");
                exit(EXIT_FAILURE);
            }
            if (val > 128) {
                fprintf(stderr, "Error: grouping out of range [max. 128]\n");
                exit(EXIT_FAILURE);
            }

            option->grouping = (int)val;
            x++;
        }

        else if (strcmp(argv[x], "-a") == 0 || (strcmp(argv[x], "--ascii") == 0)){
            // ASCII flag toggle.
            option->ascii = !option->ascii;
        }

        else if (strcmp(argv[x], "-c") == 0 || (strcmp(argv[x], "--color") == 0)){
            // Color flag toggle.
            option->color = !option->color;
        }

        else if (strcmp(argv[x], "-s") == 0 || (strcmp(argv[x], "--string") == 0)){
            // String flag toggle.
            option->string = !option->string;
        }

        else if (strcmp(argv[x], "-e") == 0 || (strcmp(argv[x], "--entropy") == 0)){
            // Entropy flag toggle.
            option->entropie = !option->entropie;
        }

        else if (strcmp(argv[x], "-th") == 0 || (strcmp(argv[x], "--toggle-header") == 0)) {
            // Header flag toggle.
            option->skip_header = !option->skip_header;
        }

        else if (strcmp(argv[x], "-sz") == 0 || (strcmp(argv[x], "--skip-zero") == 0)) {
            // Size flag toggle.
            option->skip_zero = !option->skip_zero;
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

                size_t suffix = get_suffix(endptr); 
                val = suffix * val;

                if (suffix == 0) {
                    fprintf(stderr, "Nonvalid Prefix\n");
                    exit(EXIT_FAILURE);
                }

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: offset requires a numeric value\n");
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

                size_t suffix = get_suffix(endptr); 
                val = suffix * val;

                if (suffix == 0) {
                    fprintf(stderr, "Nonvalid Prefix\n");
                    exit(EXIT_FAILURE);
                }

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: limit requires a numeric value\n");
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

                if (option->read_size != 0) {
                    printf("No valid operation: limit & read");
                    exit(EXIT_FAILURE);
                }

                // Set offset
                option->limit_read = (size_t)val;
                x++; // Skip the argument value.
            }
            
        }


        else if (strcmp(argv[x], "-r") == 0 || (strcmp(argv[x], "--read-size") == 0)) {
            // Readsize Flag
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: read requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }
            else {
                errno = 0;
                char *endptr;
                long val = strtol(argv[x + 1], &endptr, 10);

                size_t suffix = get_suffix(endptr); 
                val = suffix * val;

                if (suffix == 0) {
                    fprintf(stderr, "Nonvalid Prefix\n");
                    exit(EXIT_FAILURE);
                }

                // Check for existing argument
                if (endptr == argv[x + 1]) {
                    fprintf(stderr, "Error: read requires a numeric value\n");
                    printf("%s", help_short);
                    exit(EXIT_FAILURE);
                }

                // Check for overflow
                if (errno == ERANGE) {
                    fprintf(stderr, "Error: read is too large or too small\n");
                    exit(EXIT_FAILURE);
                }

                // Check for positiv value
                if (val <= 0) {
                    fprintf(stderr, "Error: read must be > 0\n");
                    exit(EXIT_FAILURE);
                }

                if (option->limit_read != 0) {
                    printf("No valid operation: limit & read");
                    exit(EXIT_FAILURE);
                }

                // Set offset
                option->read_size = (size_t)val;
                x++; // Skip the argument value.
            }
            
        }

        else if (strcmp(argv[x], "-se") == 0 || (strcmp(argv[x], "--search") == 0)){
            // Search quarry argument.
            if (x + 1 >= argc) {
                fprintf(stderr, "Error: Search requires an argument\n");
                printf("%s", help_short);
                exit(EXIT_FAILURE);
            }

            if (option->search != NULL) {
                free(option->search);
                option->search = NULL;
                option->search_len = 0;
            }

            parse_search_argument(argv[x + 1], option);
            x++;
        }

        else if (strcmp(argv[x], "-ro") == 0 || (strcmp(argv[x], "--raw") == 0)){
            // RAW flag toggle argument.
            option->raw = true;
        }

        else if (strcmp(argv[x], "-p") == 0 || (strcmp(argv[x], "--pager") == 0)) {
            // Pager flag toggle.
            option->pager = !option->pager;
        }

        else if (strcmp(argv[x], "-h") == 0 || (strcmp(argv[x], "--help") == 0)) {
            // Help flag.
            printf("%s", help);
            exit(EXIT_SUCCESS);
        }

        else if (strcmp(argv[x], "-v") == 0 || (strcmp(argv[x], "--version") == 0)) {
            // Help flag.
            printf("%s", HXED_VERSION);
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

    if (!option->color) {
        option->heatmap = false;
        option->string = false;
    }

    return option;
}
