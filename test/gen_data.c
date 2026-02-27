#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define _BUFSIZ 4096

static unsigned char random_data[_BUFSIZ] = {0};

void init_random_data(void) {
    srand(time(NULL));
    for (size_t i = 0; i < _BUFSIZ; i++) {
        ((unsigned char *)random_data)[i] = rand() % 256;
    }
}

void make_file(size_t size, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Failed to open file");
        return;
    }

    size_t bytes_written = 0;
    while (bytes_written < size) {
        size_t to_write = (size - bytes_written > _BUFSIZ) ? _BUFSIZ : (size - bytes_written);
        fwrite(random_data, 1, to_write, fp);
        bytes_written += to_write;
    }

    fclose(fp);
}

int main(void) {
    init_random_data();
    make_file(1024, "1KB.bin");
    
    init_random_data();
    make_file(1024 * 1024, "1MB.bin");
    
    init_random_data();
    make_file(10 * 1024 * 1024, "10MB.bin");
    
    init_random_data();    
    make_file(100 * 1024 * 1024, "100MB.bin");
    
    init_random_data();
    make_file(1000 * 1024 * 1024, "1GB.bin");

    return 0;
}