#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "Utils.h"
#include "Args.h"

/** 
 * This documentation has been generated with ai. Use with caution!
 * @author jjice <joshuajallow1@gmail.com>
 */


/**
 * @brief Reads a chunk of data from an open file handle into a dynamically allocated buffer.
 *
 *
 * @param filename File path (ignored, as the FILE* handle is already passed).
 * @param out_read Pointer to store the actual number of bytes read.
 * @param buff_size The maximum buffer size to read.
 * @param file The already opened FILE* handle.
 * @param read_start Start Byte to read
 * @return const char* Pointer to the dynamically allocated buffer (must be freed by the caller).
 */
const char *read_stream_to_buffer(size_t *out_read, int buff_size, FILE *file, size_t read_start, size_t read_limit) {
    char *buffer = (char *)malloc(buff_size);
    
    if (file == stdin) {
        // Bei stdin: Offset ignorieren, einfach lesen 
        *out_read = fread(buffer, 1,  buff_size, file);
        return buffer;
    }
    
    // Comulative offset variable for file cursor
    static bool offset_apply = false;   // If offset was firt time set -> true
    
    // Read bytes from file
    size_t read_buffer = 0;

    // Apply start_offset
    if (!offset_apply) {
        offset_apply = true;
        fseek(file, read_start, SEEK_SET);
    }

    if (!buffer) {
        perror("Cant allocate memory");
        exit(EXIT_FAILURE);
    }

    // Current File cursor position
    size_t current_pos = ftell(file);

    // Absolut limit to read
    size_t end_limit = read_start + read_limit;

    // Remaining bytes to read
    size_t remaining = 0;
    if (current_pos < end_limit) {
        remaining = end_limit - current_pos;
    }

    // If remaining is smaller than buff_size: read remaining else read buffsize
    size_t to_read = remaining < (size_t)buff_size ? remaining : (size_t)buff_size;

    // Read filecontent to buffer, output read bytes
    read_buffer = fread(buffer, 1, to_read, file);

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
void check_file(options *option) {
    FILE *fp = fopen(option->filename, "rb");

    if(option->pipeline == true) return;

    if(fp == NULL) {
        // File not found or permission denied.
        perror("File not found / No permission to read");
        exit(EXIT_FAILURE);
    }
    else {
        // Check for zero size
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        if(file_size == 0) {
            printf("File is empty"); // Prints error message
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        // Ceck if filesize is in range of offset till limit
        if (file_size < option->offset_read + option->limit_read) {
            printf("Filesize is out of range, check offset|limit");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        // Check if file limit has been placed, else EOF
        if (option->limit_read == 0) {
            fseek(fp, 0, SEEK_END);
            option->limit_read = ftell(fp);
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
    
    // Apply starting addr
    addr_display += option->offset_read;

    // --- Header Formatting ---
    if (option->pipeline) printf("\nHexdump for <pipe>:\n\n");
    else printf("\nHexdump for <%s>:\n\n", option->filename);
    
    // Prints spaces to align column headers with the hex data area.
    printf("%*s", 11, " ");
    // Prints column headers (offsets within the line: 00, 01, 02...).
    for (int i = 0; i < option->buff_size; i++) {
        printf("%02X ", i);
    }
    
    // Calculates the total length of the separating line based on buff_size and ASCII flag.
    int n = 0;
    if (option->ascii == true) n = 18 + option->buff_size * 3 + option->buff_size; 
    else n = 10 + option->buff_size * 3;
    
    printf("\n");
    
    // Prints the horizontal separator line. Includes logic to place a '+' if ASCII is enabled.
    for (int i = 0; i < n; i++) {
        if (option->ascii == true && i == 15 + option->buff_size * 3) {
            printf("+");
            //n -= 1; // Adjusts loop count to maintain overall length.
        }
        printf("-");
    }
    printf("\n");
    
    // FILE handler
    FILE *file = NULL;
    
    // checks if filename is empty, then use stdin as file
    if (!option->filename) {
        file = stdin;
    }
    
    // Opens the file for reading in binary mode.
    else {
        file = fopen(option->filename, "rb");
        if (!file) {
            perror("Cant open file");
            exit(EXIT_FAILURE);
        }
    }
    

    // --- Hex Output Loop ---
    do {
        // Reads data block by block using the helper function.
        const char *buffer = read_stream_to_buffer(&bytes_read, option->buff_size, file, option->offset_read, option->limit_read);
        if (!buffer) break; // Should only happen if malloc fails and exit is avoided.
        if (bytes_read == 0) {
            free((void *) buffer);
            break; // End of file reached.
        }

        // Prints the starting address of the current line (8-digit hex, padded).
        printf("%08zX | ", addr_display);
        
        
        // Output hex values (two digits, padded with 0, followed by a space).
        for (size_t i = 0; i < bytes_read; i++) {
            printf("%02x ", (unsigned char)buffer[i]);
        }

        // Fills the line with spaces if the last read block was smaller than buff_size.
        if (bytes_read < (size_t)option->buff_size) {
            int n_spaces = (option->buff_size - bytes_read) * 3; // 3 characters per byte (XX ).
            printf("%*s", n_spaces, "");
        }

        // ASCII column layout
        if (option->ascii == true) {
            printf("    |   "); // Separator for the ASCII view.
            
            // Iterates over the read bytes for ASCII output.
            for (size_t i = 0; i < bytes_read; i++) {
                if (buffer[i] < 32 || buffer[i] == 127) printf(".");
                else printf("%c", buffer[i]);
            }
        }

        // Updates the display address for the next row. 
        // even if the last read was shorter.
        addr_display += option->buff_size; 
        printf("\n");
        
        // Release the memory allocated in read_file_to_buffer.
        free((void*)buffer);

    } while (1);

    printf("\n");
    // Closes the file handle opened at the beginning of print_hex.
    if (!option->pipeline) fclose(file);
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
    check_file(option);

    // 3. Execute the hex dump logic.
    print_hex(option);

    // 4. Clean up allocated memory for options structure.
    free(option);
    return EXIT_SUCCESS;
}