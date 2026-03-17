/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include <stdlib.h>

#include "Args.h"
#include "Display.h"
#include "File.h"
#include "Utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
    // Enable ANSI escape code processing on Windows 10
    // and later for colored output in the console. (optional on terminal-app)
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif

    // 1. Parse command line arguments.
    options *option = get_options(argc, argv);

    // 2. Perform initial file existence and emptiness checks.
    check_file(option);

    // 3. Execute the hex dump logic.
    print_hex(option);

    // 4. Clean up allocated memory for options structure.
    free(option->search);
    free(option);
    print_color(RESET, true);

    return EXIT_SUCCESS;
}
