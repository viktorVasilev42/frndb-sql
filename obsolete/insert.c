#include "first.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void append_new_node() {
    FILE *file = fopen("output.txt", "a");
}

void insert(uint32_t *node) {
    uint32_t num_cells_in_node = *get_node_num_cells(node);
    if (num_cells_in_node > 30) {

    }
}

int main() {
    const char *filename = "output.txt";
    const size_t filesize = 256;

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    uint32_t *my_node = mmap(NULL, filesize, 
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
    );
    if (my_node == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    

    if (munmap(my_node, filesize) == -1) {
        perror("Error unmapping file");
    }

    close(fd);
    return 0;
}
