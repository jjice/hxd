#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

/** 
 * This documentation has been generated with ai. Use with caution!
 * @author jjice <joshuajallow1@gmail.com>
 */

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
} options;

// bad characters
/**
 * @brief Array of control characters (0x00 through 0x1F) and the DEL character (0x7F).
 * These characters are considered non-printable or potentially harmful to terminal display 
 * and are replaced by a dot ('.') in the ASCII output column.
 */
const unsigned char replace_with_dot[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x7F
};

/**
 * @brief Reads a chunk of data from an open file handle into a dynamically allocated buffer.
 *
 *
 * @param filename File path (ignored, as the FILE* handle is already passed).
 * @param out_read Pointer to store the actual number of bytes read.
 * @param buff_size The maximum buffer size to read.
 * @param file The already opened FILE* handle.
 * @return const char* Pointer to the dynamically allocated buffer (must be freed by the caller).
 */
const char *read_file_to_buffer(const char *filename, size_t *out_read, int buff_size, FILE *file) {
    char *buffer = malloc(buff_size);
    // STATIC OFFSET: Tracks the cumulative read position across all function calls.
    static size_t offset = 0;

    if (!buffer) {
        perror("Cant allocate memory");
        exit(EXIT_FAILURE);
    }

    // Repositions the file pointer based on the static offset.
    fseek(file, offset, SEEK_SET);

    size_t read_buffer = fread(buffer, 1, buff_size, file);
    
    // Updates the static offset for the next sequential read.
    offset += read_buffer;

    *out_read = read_buffer;
    return buffer;
}

/**
 * @brief Checks if a file exists and verifies that its size is non-zero.
 *
 * @note Uses fseek/ftell to determine file size, which is efficient for checking emptiness.
 * @note Error message "File is empty" is printed to stdout, not stderr, which is non-standard for errors.
 *
 * @param filename The path to the file to be checked.
 */
void check_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");

    if(fp == NULL) {
        // File not found or permission denied.
        perror("File not found");
        exit(EXIT_FAILURE);
    }
    else {
        // Check for zero size
        fseek(fp, 0, SEEK_END);
        if(ftell(fp) == 0) {
            printf("File is empty"); // Prints error message
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        // Close the file handle after successful check.
        fclose(fp);
    }
}

/**
 * @brief Executes the core hex dump loop, handling output formatting and file reading.
 *
 * @param option Pointer to the configured options structure.
 */
void print_hex(options *option){
    size_t bytes_read = 0;       // Bytes read in last read_file_to_buffer call.
    size_t addr_display = 0;     // Starting memory address to display for the current row.

    // --- Header Formatting ---
    printf("\nHexdump for <%s>:\n\n", option->filename);
    
    // Prints spaces to align column headers with the hex data area.
    printf("%*s", 11, " ");
    // Prints column headers (offsets within the line: 00, 01, 02...).
    for (int i = 0; i < option->buff_size; i++) {
        printf("%02X ", i);
    }
    
    // Calculates the total length of the separating line based on buff_size and ASCII flag.
    int n = 0;
    if (option->ascii == true) n = 16 + option->buff_size * 3 + option->buff_size; 
    else n = 10 + option->buff_size * 3;
    
    printf("\n");
    
    // Prints the horizontal separator line. Includes logic to place a '+' if ASCII is enabled.
    for (int i = 0; i < n; i++) {
        if (option->ascii == true && i == 13 + option->buff_size * 3) {
            printf("+");
            n -= 1; // Adjusts loop count to maintain overall length.
        }
        printf("-");
    }
    printf("\n");

    // FILE handler
    // Opens the file for reading in binary mode.
    FILE *file = fopen(option->filename, "rb");
    if (!file) {
        perror("Cant open file");
        exit(EXIT_FAILURE);
    }

    // --- Hex Output Loop ---
    do {
        // Reads data block by block using the helper function.
        const char *buffer = read_file_to_buffer(option->filename, &bytes_read, option->buff_size, file);
        if (!buffer) break; // Should only happen if malloc fails and exit is avoided.
        if (bytes_read == 0) {
            break; // End of file reached.
        }

        // Prints the starting address of the current line (8-digit hex, padded).
        printf("%08X | ", addr_display);
        
        
        // Output hex values (two digits, padded with 0, followed by a space).
        for (size_t i = 0; i < bytes_read; i++) {
            printf("%02x ", (unsigned char)buffer[i]);
        }

        // Fills the line with spaces if the last read block was smaller than buff_size.
        if (bytes_read < option->buff_size) {
            int n_spaces = (option->buff_size - bytes_read) * 3; // 3 characters per byte (XX ).
            printf("%*s", n_spaces, "");
        }

        // ASCII column layout
        bool b_break = false; // Flag used inside the inner loop.
        
        if (option->ascii == true) {
            printf("    |   "); // Separator for the ASCII view.
            
            // Iterates over the read bytes for ASCII output.
            for (size_t i = 0; i < bytes_read; i++) {
                b_break = false;
                
                // Linear search: Checks if the current byte is in the 'replace_with_dot' list.
                // NOTE: This O(N*M) approach is inefficient compared to a simple range check (0x00 < c < 0x20 || c == 0x7F).
                for (size_t x = 0; x < sizeof(replace_with_dot); x++) {
                    if (replace_with_dot[x] == buffer[i]) {
                        printf(".");
                        b_break = true;
                    }
                }
                
                // If not a bad character, print the ASCII representation.
                if (!b_break) printf("%c", (char)buffer[i]);
            }
        }

        // Updates the display address for the next row. 
        // NOTE: Using option->buff_size here is correct for continuous display address tracking,
        // even if the last read was shorter.
        addr_display += option->buff_size; 
        printf("\n");
        
        // Release the memory allocated in read_file_to_buffer.
        free((void*)buffer);

    } while (bytes_read > 0);
    
    printf("\n");
    // Closes the file handle opened at the beginning of print_hex.
    fclose(file);
}

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
    options *option = malloc(sizeof(options));
    if (!option) {
        perror("Cant malloc options");
        exit(EXIT_FAILURE);
    }

    // Set default values.
    option->filename = NULL;
    option->buff_size = 16;
    option->ascii = true;

    // Help message string.
    char *help = "Usage:\n       (-f) : <filename>"
                 "\n    -b : <buffer> (default = 16)"
                 "\n    -a : <on/off> (default = on)"
                 "\n    -h : Show this info\n";
    
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
                printf("%s", help);
                exit(EXIT_FAILURE);
            }

            // Robust conversion using strtol for numeric input validation.
            errno = 0;
            char *endptr;
            long val = strtol(argv[x + 1], &endptr, 10);

            if (endptr == argv[x + 1]) {
                fprintf(stderr, "Error: -b requires a numeric value\n");
                printf("%s", help);
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
                printf("%s", help);
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
                    printf("%s", help);
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
            printf("%s", help);
            exit(EXIT_SUCCESS);
        }
    }

    // Final validation: Ensure a filename was provided.
    if(option->filename == NULL) {
        printf("No valid filename");
        exit(EXIT_FAILURE);
    }

    return option;
}

/**
 * @brief Main entry point of the program.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {

    // 1. Parse command line arguments.
    options *option = get_options(argc, argv);

    // 2. Perform initial file existence and emptiness checks.
    check_file(option->filename);

    // 3. Execute the hex dump logic.
    print_hex(option);

    // 4. Clean up allocated memory for options structure.
    free(option);
    return EXIT_SUCCESS;
}