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

    int fd = open(filename,
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR
    );
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, filesize) == -1) {
        perror("Error setting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    uint8_t *my_node = mmap(NULL, filesize,
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

    my_node[NODE_TYPE_OFFSET] = 0;
    my_node[IS_ROOT_OFFSET] = 1;
    *((uint32_t*)(my_node + PARENT_POINTER_OFFSET)) = UINT32_MAX;

    const uint32_t KEY_VALUE_JOINT_SIZE = KEY_SIZE + VALUE_SIZE;
    const uint32_t FIRST_VALUE_OFFSET = KEY_OFFSET + KEY_SIZE;
    const uint32_t num_cells_to_write = 5;
    *((uint32_t*)(my_node + NUM_CELLS_OFFSET)) = num_cells_to_write;

    for (uint32_t i = 0; i < num_cells_to_write; i++) {
        my_node[KEY_OFFSET + (i * KEY_VALUE_JOINT_SIZE)] = i + 1;
        my_node[FIRST_VALUE_OFFSET + (i * KEY_VALUE_JOINT_SIZE)] = i + 21;
    }
    const uint32_t right_most_offset = 
        FIRST_VALUE_OFFSET + (num_cells_to_write * KEY_VALUE_JOINT_SIZE);
    my_node[right_most_offset] = num_cells_to_write + 21;

    if (munmap(my_node, filesize) == -1) {
        perror("Error unmapping file");
    }

    close(fd);
    return 0;
}
