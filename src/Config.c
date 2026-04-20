/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include "Config.h"
#include "Args.h"
#include "Utils.h"
#include "hxed_config.h.in"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

char* CONTROL_COLOR = NULL;
char* NULL_BYTE_COLOR = NULL;
char* ADDR_COLOR = NULL;
char* ASCII_COLOR = NULL;
char* EXTENDED_ASCII_COLOR = NULL;
char* HEADER_COLOR = NULL;
char* MAGIC_COLOR = NULL;
char* BORDER_COLOR = NULL;
char* ANALYSIS_TEXT_COLOR = NULL;
char* ERROR_COLOR = NULL;
char* HIGHLIGHT_COLOR = NULL;

#if defined(_MSC_VER)
    #define strdup _strdup
#endif

static void init_default_colors(void) {
    CONTROL_COLOR = strdup("\x1b[38;5;153m");
    NULL_BYTE_COLOR = strdup("\x1b[38;5;240m");
    ADDR_COLOR = strdup("\x1b[38;5;67m");
    ASCII_COLOR = strdup("\x1b[38;5;75m");
    EXTENDED_ASCII_COLOR = strdup("\x1b[38;5;110m");
    HEADER_COLOR = strdup("\x1b[38;5;110m");
    MAGIC_COLOR = strdup("\x1b[38;5;179m");
    BORDER_COLOR = strdup("\x1b[38;5;109m");
    ANALYSIS_TEXT_COLOR = strdup("\x1b[38;5;67m");
    ERROR_COLOR = strdup("\x1b[38;5;9m");
    HIGHLIGHT_COLOR = strdup("\x1b[1;97;104m");
}


void cleanup_colors(void) {
    free(CONTROL_COLOR); CONTROL_COLOR = NULL;
    free(NULL_BYTE_COLOR); NULL_BYTE_COLOR = NULL;
    free(ADDR_COLOR); ADDR_COLOR = NULL;
    free(ASCII_COLOR); ASCII_COLOR = NULL;
    free(EXTENDED_ASCII_COLOR); EXTENDED_ASCII_COLOR = NULL;
    free(HEADER_COLOR); HEADER_COLOR = NULL;
    free(MAGIC_COLOR); MAGIC_COLOR = NULL;
    free(BORDER_COLOR); BORDER_COLOR = NULL;
    free(ANALYSIS_TEXT_COLOR); ANALYSIS_TEXT_COLOR = NULL;
    free(ERROR_COLOR); ERROR_COLOR = NULL;
    free(HIGHLIGHT_COLOR); HIGHLIGHT_COLOR = NULL;
}

static bool parse_bool(const char *value, bool *out) {
    if (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0) {
        *out = true;
        return true;
    }

    if (strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "0") == 0) {
        *out = false;
        return true;
    }

    return false;
}

static bool parse_int(const char *value, int *out) {
    char *end;
    long result;

    if (value == NULL || *value == '\0') {
        return false;
    }

    errno = 0;
    result = strtol(value, &end, 10);

    if (errno != 0 || end == value || *end != '\0') {
        return false;
    }

    if (result < INT_MIN || result > INT_MAX) {
        return false;
    }

    *out = (int)result;
    return true;
}

static bool parse_heatmap(const char *value, int *out) {
    if (strcmp(value, "none") == 0) {
        *out = 0;
        return true;
    }

    if (strcmp(value, "adaptive") == 0) {
        *out = 1;
        return true;
    }

    if (strcmp(value, "fixed") == 0) {
        *out = 2;
        return true;
    }

    if (parse_int(value, out)) {
        if (*out >= 0 && *out <= 2) {
            return true;
        }
    }

    return false;
}

static bool parse_output_mode(const char *value, int *out) {
    if (strcmp(value, "0") == 0 || strcmp(value, "hex") == 0) {
        *out = 0;
        return true;
    }

    if (strcmp(value, "1") == 0 || strcmp(value, "bin") == 0) {
        *out = 1;
        return true;
    }

    if (strcmp(value, "2") == 0 || strcmp(value, "oct") == 0) {
        *out = 2;
        return true;
    }

    if (strcmp(value, "3") == 0 || strcmp(value, "dec") == 0) {
        *out = 3;
        return true;
    }

    return false;
}

static char *expressions[] = {
    "output_mode", "heatmap", "width", "grouping",
    "show_ascii", "show_color", "string", "entropie", "toggle_header",
    "skip_zero", "raw", "reverse", "CONTROL_COLOR", "NULL_BYTE_COLOR", "ADDR_COLOR",
    "ASCII_COLOR", "EXTENDED_ASCII_COLOR", "HEADER_COLOR", "MAGIC_COLOR",
    "BORDER_COLOR", "ANALYSIS_TEXT_COLOR", "ERROR_COLOR", "HIGHLIGHT_COLOR"
};

static char *get_config(void) {
    FILE *fconfig = fopen(CONFIG_FILE_PATH, "r");

    if (fconfig == NULL) {
        fprintf(stderr, "No config file found, using defaults\n");
        return NULL;
    }

    // Read the config file into a buffer
    fseek(fconfig, 0, SEEK_END);
    size_t fconfig_size = ftell(fconfig);
    fseek(fconfig, 0, SEEK_SET);

    if (fconfig_size == 0) {
        fclose(fconfig);
        return NULL;
    }

    char *out = (char *)malloc(fconfig_size + 1);
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed for config content\n");
        fclose(fconfig);
        return NULL;
    }

    size_t read = fread(out, 1, fconfig_size, fconfig);
    fclose(fconfig);

    if (read != fconfig_size) {
        fprintf(stderr, "Error: Could not read config file\n");
        free(out);
        return NULL;
    }

    out[fconfig_size] = '\0';
    return out;
}

static void rgb_to_ansi(const char *rgb_str, char *buffer, size_t buffer_size) {
    static const char *format = "\x1b[38;5;%dm";
    int r, g, b;

    if (sscanf(rgb_str, "%d;%d;%d", &r, &g, &b) != 3) {
        buffer[0] = '\0';
        return;
    }

    r = r < 0 ? 0 : (r > 255 ? 255 : r);
    g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b);

    // Convert to 256-color code (6x6x6 cube)
    int code = 16 + (36 * (r * 6 / 256)) + (6 * (g * 6 / 256)) + (b * 6 / 256);
    snprintf(buffer, buffer_size, format, code);
}

static char *trim(char *str) {
    char *end;

    if (str == NULL) {
        return NULL;
    }

    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

static bool set_color_value(char **target, const char *value, const char *key) {
    char ansi[32];
    char *new_color;

    rgb_to_ansi(value, ansi, sizeof(ansi));
    if (ansi[0] == '\0') {
        fprintf(stderr, "Invalid color value for %s: %s\n", key, value);
        return false;
    }

    new_color = strdup(ansi);
    if (!new_color) {
        fprintf(stderr, "Memory allocation failed for %s\n", key);
        return false;
    }

    free(*target);
    *target = new_color;
    return true;
}

#if defined(_WIN32)
#define strtok_r strtok_s
#endif

void print_config(void) {
    FILE *fconfig = fopen(CONFIG_FILE_PATH, "r");

    if (fconfig == NULL) {
        fprintf(stderr, "No config file found under %s\n", CONFIG_FILE_PATH);
        return;
    }

    printf("Current config from %s:\n\n", CONFIG_FILE_PATH);

    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), fconfig)) {
        printf("%s", buffer);
    }

    fclose(fconfig);
}

void set_config(options *opt) {
    static bool colors_initialized = false;

    if (!colors_initialized) {
        init_default_colors();
        colors_initialized = true;
    }

    char *config_data = get_config();
    if (config_data == NULL) {
        fprintf(stderr, "Using default config\n");
        return;
    }

    char *saveptr;
    char *line = strtok_r(config_data, "\n", &saveptr);
    while (line != NULL) {
        char *eq;
        char *key;
        char *value;
        bool found = false;
        bool bool_value;
        int int_value;

        line = trim(line);

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        eq = strchr(line, '=');
        if (eq == NULL) {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        *eq = '\0';
        key = trim(line);
        value = trim(eq + 1);

        for (size_t i = 0; i < sizeof(expressions) / sizeof(expressions[0]); i++) {
            if (strcmp(key, expressions[i]) == 0) {
                found = true;

                // Color setting
                if (strcmp(key, "CONTROL_COLOR") == 0) {
                    set_color_value(&CONTROL_COLOR, value, key);
                } else if (strcmp(key, "NULL_BYTE_COLOR") == 0) {
                    set_color_value(&NULL_BYTE_COLOR, value, key);
                } else if (strcmp(key, "ADDR_COLOR") == 0) {
                    set_color_value(&ADDR_COLOR, value, key);
                } else if (strcmp(key, "ASCII_COLOR") == 0) {
                    set_color_value(&ASCII_COLOR, value, key);
                } else if (strcmp(key, "EXTENDED_ASCII_COLOR") == 0) {
                    set_color_value(&EXTENDED_ASCII_COLOR, value, key);
                } else if (strcmp(key, "HEADER_COLOR") == 0) {
                    set_color_value(&HEADER_COLOR, value, key);
                } else if (strcmp(key, "MAGIC_COLOR") == 0) {
                    set_color_value(&MAGIC_COLOR, value, key);
                } else if (strcmp(key, "BORDER_COLOR") == 0) {
                    set_color_value(&BORDER_COLOR, value, key);
                } else if (strcmp(key, "ANALYSIS_TEXT_COLOR") == 0) {
                    set_color_value(&ANALYSIS_TEXT_COLOR, value, key);
                } else if (strcmp(key, "ERROR_COLOR") == 0) {
                    set_color_value(&ERROR_COLOR, value, key);
                } else if (strcmp(key, "HIGHLIGHT_COLOR") == 0) {
                    set_color_value(&HIGHLIGHT_COLOR, value, key);
                }

                else if (strcmp(key, "heatmap") == 0) {
                    if (parse_heatmap(value, &int_value)) {
                        opt->heatmap = int_value;
                    } else {
                        fprintf(stderr, "Invalid value for heatmap: %s\n", value);
                    }
                }

                else if (strcmp(key, "output_mode") == 0) {
                    if (parse_output_mode(value, &int_value)) {
                        opt->output_mode = int_value;
                    } else {
                        fprintf(stderr, "Invalid value for output_mode: %s\n", value);
                    }
                }

                else if (strcmp(key, "width") == 0) {
                    if (parse_int(value, &int_value)) {
                        opt->buff_size = int_value;
                    } else {
                        fprintf(stderr, "Invalid value for width: %s\n", value);
                    }
                }

                else if (strcmp(key, "grouping") == 0) {
                    if (parse_int(value, &int_value)) {
                        opt->grouping = int_value;
                    } else {
                        fprintf(stderr, "Invalid value for grouping: %s\n", value);
                    }
                }

                else if (strcmp(key, "show_ascii") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->ascii = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for show_ascii: %s\n", value);
                    }
                }

                else if (strcmp(key, "show_color") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->color = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for show_color: %s\n", value);
                    }
                }

                else if (strcmp(key, "string") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->string = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for string: %s\n", value);
                    }
                }

                else if (strcmp(key, "entropie") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->entropie = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for entropie: %s\n", value);
                    }
                }

                else if (strcmp(key, "toggle_header") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->skip_header = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for toggle_header: %s\n", value);
                    }
                }

                else if (strcmp(key, "skip_zero") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->skip_zero = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for skip_zero: %s\n", value);
                    }
                }

                else if (strcmp(key, "raw") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->raw = bool_value;
                    } else {
                        fprintf(stderr, "Invalid value for raw: %s\n", value);
                    }
                }

                else if (strcmp(key, "reverse") == 0) {
                    if (parse_bool(value, &bool_value)) {
                        opt->reverse_mode = bool_value;
                        opt->skip_header = true; // Reverse mode implies skipping header analysis
                    } else {
                        fprintf(stderr, "Invalid value for reverse: %s\n", value);
                    }
                }

                break;
            }
        }

        if (!found) {
            fprintf(stderr, "%s unknown config key\n", key);
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(config_data);
}