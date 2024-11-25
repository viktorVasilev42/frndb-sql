#include "../load_node_to_mem.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

void write_multipage_file() {
    const char *filename = "multipage_file.frndb";
    const uint32_t os_page_size = 4096;
    const uint32_t filesize = os_page_size * 3;

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

    uint8_t *mapped_mem = mmap(NULL, filesize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
    );
    close(fd);
    if (mapped_mem == MAP_FAILED) {
        perror("Error mapping file");
        exit(EXIT_FAILURE);
    }

    // first node
    for (int i = 0; i < 4096; i++) {
        mapped_mem[i] = 66;
    }

    // second node
    uint8_t* my_node = mapped_mem + 4096;
    my_node[NODE_TYPE_OFFSET] = 0;
    my_node[IS_ROOT_OFFSET] = 1;
    *((uint32_t*)(my_node + PARENT_POINTER_OFFSET)) = UINT32_MAX;
    const uint32_t KEY_VALUE_JOINT_SIZE = KEY_SIZE + VALUE_SIZE;
    const uint32_t FIRST_VALUE_OFFSET = KEY_OFFSET + KEY_SIZE;
    *((uint32_t*)(my_node + NUM_CELLS_OFFSET)) = 15;
    for (int i = 0; i < 15; i++) {
        *((uint32_t*)(my_node + KEY_OFFSET + (i * KEY_VALUE_JOINT_SIZE))) = i + 1;
        *((uint32_t*)(my_node + FIRST_VALUE_OFFSET + (i * KEY_VALUE_JOINT_SIZE))) = i + 21;
    }
    const uint32_t right_most_offset = KEY_OFFSET + (15 * KEY_VALUE_JOINT_SIZE);
    *((uint32_t*)(my_node + right_most_offset)) = 15 + 21;

    // third node
    uint8_t* last_node = my_node + 4096;
    for (int i = 0; i < 4096; i++) {
        last_node[i] = 66;
    }

    if (munmap(mapped_mem, filesize) == -1) {
        perror("Error umapping file");
        exit(EXIT_FAILURE);
    }
}

void test_load_multipage() {
    printf("test_load_multipage\n");
    write_multipage_file();
    printf("Done writing\n");
    const char *filename = "multipage_file.frndb";
    const uint32_t os_page_size = 4096;
    const size_t filesize = os_page_size * 3;

    BTreeNode *my_node = load_node_to_mem(filename, 16);
    print_node_from_mem(my_node);
    printf("\n");

    free(my_node->keys);
    free(my_node->values);
    free(my_node);
}

int main() {
    test_load_multipage();

    return 0;
}
