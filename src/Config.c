/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include "Config.h"
#include "Args.h"
#include "Utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

static void cleanup_colors(void) {
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

static inline bool parse_bool(const char *value) {
    return (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
}

static inline int parse_heatmap(const char *value) {
    if (strcmp(value, "none") == 0) return 0;
    if (strcmp(value, "adaptive") == 0) return 1;
    if (strcmp(value, "fixed") == 0) return 2;
    return atoi(value); // fallback
}

#define CONFIG_FILE_NAME "hxed.conf"

static char *config_default = 
    "# this is the default config for hxed\n"
    "# you can edit this file to change the default settings\n"
    "# for hxed. If the file doesn't exist, it will be created with these defaults.\n"
    "# see -h for more information on args.\n"
    "# Default color scheme\n"
    "default_color_scheme=blue\n"
    "output_mode=0\n"
    "heatmap=none\n"
    "width=16\n"
    "grouping=1\n"
    "show_ascii=true\n"
    "show_color=true\n"
    "string=false\n"
    "entropie=false\n"
    "toggle_header=false\n"
    "skip_zero=false\n"
    "raw=false\n"
    "\n\n"
    "# custom color scheme example\n"
    "#CONTROL_COLOR=R;G;B\n"
    "#NULL_BYTE_COLOR=R;G;B\n"
    "#ADDR_COLOR=R;G;B\n"
    "#ASCII_COLOR=R;G;B\n"
    "#EXTENDED_ASCII_COLOR=R;G;B\n"
    "#HEADER_COLOR=R;G;B\n"
    "#MAGIC_COLOR=R;G;B\n"
    "#BORDER_COLOR=R;G;B\n"
    "#ANALYSIS_TEXT_COLOR=R;G;B\n"
    "#ERROR_COLOR=R;G;B\n"
    "#HIGHLIGHT_COLOR=R;G;B"
;
char *expressions[] = {"output_mode", "heatmap", "width", "grouping",
    "show_ascii", "show_color", "string", "entropie", "toggle_header",
    "skip_zero", "raw", "CONTROL_COLOR", "NULL_BYTE_COLOR", "ADDR_COLOR",
    "ASCII_COLOR", "EXTENDED_ASCII_COLOR", "HEADER_COLOR", "MAGIC_COLOR",
    "BORDER_COLOR", "ANALYSIS_TEXT_COLOR", "ERROR_COLOR", "HIGHLIGHT_COLOR"}
;


static inline char *get_config(void) {
    FILE *fconfig = fopen(CONFIG_FILE_NAME, "r");

    // Write default file config if it doesn't exist
    if (fconfig == NULL) {
        fconfig = fopen(CONFIG_FILE_NAME, "w");
        if (fconfig == NULL) {
            fprintf(stderr, "Could not create hxed.conf\n");
            return NULL;
        }
        else {
            fwrite(config_default, strlen(config_default), 1, fconfig);
            fclose(fconfig);
            return NULL;
        }
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

static inline void rgb_to_ansi(const char *rgb_str, char *buffer, size_t buffer_size) {
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
        // Skip comments and empty lines
        if (line[0] != '#' && line[0] != '\0') {
            char *eq = strchr(line, '=');
            if (eq != NULL) {
                *eq = '\0';
                char *key = line;
                char *value = eq + 1;
                trim(key);
                trim(value);

                bool found = false;
                for (size_t i = 0; i < sizeof(expressions) / sizeof(expressions[0]); i++) {
                    if (strcmp(key, expressions[i]) == 0) {
                        found = true;

                        // Color setting
                        if (strstr(key, "_COLOR") != NULL) {
                            char ansi[32];
                            rgb_to_ansi(value, ansi, sizeof(ansi));
                            if (ansi[0] != '\0') {
                                if (strcmp(key, "CONTROL_COLOR") == 0) {
                                    free(CONTROL_COLOR);
                                    CONTROL_COLOR = strdup(ansi);
                                    if (!CONTROL_COLOR) fprintf(stderr, "Memory allocation failed for CONTROL_COLOR\n");
                                } else if (strcmp(key, "NULL_BYTE_COLOR") == 0) {
                                    free(NULL_BYTE_COLOR);
                                    NULL_BYTE_COLOR = strdup(ansi);
                                    if (!NULL_BYTE_COLOR) fprintf(stderr, "Memory allocation failed for NULL_BYTE_COLOR\n");
                                } else if (strcmp(key, "ADDR_COLOR") == 0) {
                                    free(ADDR_COLOR);
                                    ADDR_COLOR = strdup(ansi);
                                    if (!ADDR_COLOR) fprintf(stderr, "Memory allocation failed for ADDR_COLOR\n");
                                } else if (strcmp(key, "ASCII_COLOR") == 0) {
                                    free(ASCII_COLOR);
                                    ASCII_COLOR = strdup(ansi);
                                    if (!ASCII_COLOR) fprintf(stderr, "Memory allocation failed for ASCII_COLOR\n");
                                } else if (strcmp(key, "EXTENDED_ASCII_COLOR") == 0) {
                                    free(EXTENDED_ASCII_COLOR);
                                    EXTENDED_ASCII_COLOR = strdup(ansi);
                                    if (!EXTENDED_ASCII_COLOR) fprintf(stderr, "Memory allocation failed for EXTENDED_ASCII_COLOR\n");
                                } else if (strcmp(key, "HEADER_COLOR") == 0) {
                                    free(HEADER_COLOR);
                                    HEADER_COLOR = strdup(ansi);
                                    if (!HEADER_COLOR) fprintf(stderr, "Memory allocation failed for HEADER_COLOR\n");
                                } else if (strcmp(key, "MAGIC_COLOR") == 0) {
                                    free(MAGIC_COLOR);
                                    MAGIC_COLOR = strdup(ansi);
                                    if (!MAGIC_COLOR) fprintf(stderr, "Memory allocation failed for MAGIC_COLOR\n");
                                } else if (strcmp(key, "BORDER_COLOR") == 0) {
                                    free(BORDER_COLOR);
                                    BORDER_COLOR = strdup(ansi);
                                    if (!BORDER_COLOR) fprintf(stderr, "Memory allocation failed for BORDER_COLOR\n");
                                } else if (strcmp(key, "ANALYSIS_TEXT_COLOR") == 0) {
                                    free(ANALYSIS_TEXT_COLOR);
                                    ANALYSIS_TEXT_COLOR = strdup(ansi);
                                    if (!ANALYSIS_TEXT_COLOR) fprintf(stderr, "Memory allocation failed for ANALYSIS_TEXT_COLOR\n");
                                } else if (strcmp(key, "ERROR_COLOR") == 0) {
                                    free(ERROR_COLOR);
                                    ERROR_COLOR = strdup(ansi);
                                    if (!ERROR_COLOR) fprintf(stderr, "Memory allocation failed for ERROR_COLOR\n");
                                } else if (strcmp(key, "HIGHLIGHT_COLOR") == 0) {
                                    free(HIGHLIGHT_COLOR);
                                    HIGHLIGHT_COLOR = strdup(ansi);
                                    if (!HIGHLIGHT_COLOR) fprintf(stderr, "Memory allocation failed for HIGHLIGHT_COLOR\n");
                                }
                            }
                        }

                        else if (strcmp(key, "heatmap") == 0) {
                            opt->heatmap = parse_heatmap(value);
                        }

                        else if (strcmp(key, "output_mode") == 0) {
                            opt->output_mode = atoi(value);
                        } else if (strcmp(key, "width") == 0) {
                            opt->buff_size = atoi(value);
                        } else if (strcmp(key, "grouping") == 0) {
                            opt->grouping = atoi(value);
                        } else if (strcmp(key, "show_ascii") == 0) {
                            opt->ascii = parse_bool(value);
                        } else if (strcmp(key, "show_color") == 0) {
                            opt->color = parse_bool(value);
                        } else if (strcmp(key, "string") == 0) {
                            opt->string = parse_bool(value);
                        } else if (strcmp(key, "entropie") == 0) {
                            opt->entropie = parse_bool(value);
                        } else if (strcmp(key, "toggle_header") == 0) {
                            opt->skip_header = parse_bool(value);
                        } else if (strcmp(key, "skip_zero") == 0) {
                            opt->skip_zero = parse_bool(value);
                        } else if (strcmp(key, "raw") == 0) {
                            opt->raw = parse_bool(value);
                        }
                        break;
                    }
                }

                if (!found) {
                    fprintf(stderr, "%s unknown config key\n", key);
                }
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(config_data);
}