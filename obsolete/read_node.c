#include "first.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    const char *filename = "output.txt";
    const size_t filesize = 256;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    uint8_t *my_node = mmap(NULL, filesize,
            PROT_READ,
            MAP_SHARED,
            fd,
            0
    );
    if (my_node == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    uint8_t node_type = *get_node_type(my_node);
    uint8_t is_root = *get_node_is_root(my_node);
    uint32_t parent_pointer = *get_node_parent_pointer(my_node);

    uint32_t num_cells_to_read = *get_node_num_cells(my_node);
    uint32_t *node_keys = malloc(num_cells_to_read * sizeof(uint32_t));
    uint32_t *node_values = malloc(num_cells_to_read * sizeof(uint32_t));
    for (uint32_t i = 0; i < num_cells_to_read; i++) {
        node_keys[i] = my_node[KEY_OFFSET + (i * 8)];
        node_values[i] = my_node[(KEY_OFFSET + 4) + (i * 8)];
    }

    printf("NODE TYPE: %u\n", node_type);
    printf("IS ROOT: %u\n", is_root);
    printf("PARENT POINTER: %u\n", parent_pointer);

    printf("CELLS:\n");
    for (uint32_t i = 0; i < num_cells_to_read; i++) {
        printf("%u -> %u\n", node_keys[i], node_values[i]);
    }

    if (munmap(my_node, filesize) == -1) {
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }

    free(node_keys);
    free(node_values);
    close(fd);
    return 0;
}
