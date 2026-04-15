/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include "Utils.h"

#include <stddef.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/stat.h>
#endif

const char *heatmap_colors[16] = {
    "\x1b[38;5;240m",  // 0  (low)
    "\x1b[38;5;241m",
    "\x1b[38;5;242m",
    "\x1b[38;5;66m",
    "\x1b[38;5;67m",
    "\x1b[38;5;68m",
    "\x1b[38;5;69m",
    "\x1b[38;5;74m",
    "\x1b[38;5;75m",
    "\x1b[38;5;81m",
    "\x1b[38;5;109m",
    "\x1b[38;5;110m",
    "\x1b[38;5;115m",
    "\x1b[38;5;151m",
    "\x1b[38;5;179m",
    "\x1b[38;5;221m"   // 15 (high)
};

void print_color(const char *color_code, bool enable_color) {
    if (enable_color) {
        printf("%s", color_code);
    }
}

FILE *open_pager(void) {
    FILE *pager = popen("less -R", "w");
    if (pager) return pager;

    pager = popen("more", "w");
    if (pager) return pager;

    return stdout;
}

int is_space(size_t n, unsigned char *str) {
    const unsigned char *end = str + n;
    while (str < end) {
        if (*str++ != 32) return 0;
    }
    return 1;
}

// Format a local timestamp into a readable string.
void format_time_local(time_t value, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;

    struct tm tm_value;
    memset(&tm_value, 0, sizeof(tm_value));

    #ifdef _WIN32
    if (localtime_s(&tm_value, &value) != 0) {
        snprintf(buffer, buffer_size, "n/a");
        return;
    }
    #else
    if (localtime_r(&value, &tm_value) == NULL) {
        snprintf(buffer, buffer_size, "n/a");
        return;
    }
    #endif

    if (strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &tm_value) == 0) {
        snprintf(buffer, buffer_size, "n/a");
    }
}

// Read file metadata in a cross-platform way.
bool get_file_metadata(const char *filename, file_metadata *meta) {
    if (!filename || !meta) return false;

    memset(meta, 0, sizeof(*meta));

    #ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    if (!GetFileAttributesExA(filename, GetFileExInfoStandard, &attr)) {
        return false;
    }

    ULARGE_INTEGER size;
    size.LowPart = attr.nFileSizeLow;
    size.HighPart = attr.nFileSizeHigh;
    meta->file_size = (size_t)size.QuadPart;

    ULARGE_INTEGER created;
    created.LowPart = attr.ftCreationTime.dwLowDateTime;
    created.HighPart = attr.ftCreationTime.dwHighDateTime;

    ULARGE_INTEGER modified;
    modified.LowPart = attr.ftLastWriteTime.dwLowDateTime;
    modified.HighPart = attr.ftLastWriteTime.dwHighDateTime;

    // Convert Windows FILETIME to Unix timestamp (seconds since 1970-01-01)
    meta->created_at = (time_t)((created.QuadPart - 116444736000000000ULL) / 10000000ULL); 
    meta->modified_at = (time_t)((modified.QuadPart - 116444736000000000ULL) / 10000000ULL);
    meta->has_created = true;
    meta->has_modified = true;
    meta->exists = true;
    return true;
    #else
    struct stat st = {0};
    if (stat(filename, &st) != 0) {
        return false;
    }

    meta->file_size = (size_t)st.st_size;
    meta->modified_at = st.st_mtime;
    meta->has_modified = true;

    #ifdef __APPLE__
    meta->created_at = st.st_birthtimespec.tv_sec;
    meta->has_created = true;
    #elif defined(__linux__)
    struct statx stx = {0};
    if (syscall(SYS_statx, AT_FDCWD, filename, 0, STATX_BTIME, &stx) == 0 &&
        (stx.stx_mask & STATX_BTIME)) {
        meta->created_at = stx.stx_btime.tv_sec;
        meta->has_created = true;
    }
    #endif

    meta->exists = true;
    return true;
    #endif
}

void analyse(dump_analysis *analysis, unsigned char *data, size_t len) {
    if (!analysis || !data) return;

    for (size_t i = 0; i < len; i++) {
        unsigned char byte = data[i];

        if (byte == 0x00) {
            analysis->zero_bytes++;
        }
        else if (byte >= 0x20 && byte <= 0x7E) {
            analysis->printable++;
        }
        else if ((byte <= 0x1F || byte == 0x7F) && byte != 0x00) {
            analysis->control++;
        }
        else if (byte > 0x7F) {
            analysis->extended_ascii++;
        }
    }

    analysis->total_bytes += len;
}