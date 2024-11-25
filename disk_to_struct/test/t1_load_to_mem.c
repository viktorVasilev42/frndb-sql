#include "../load_node_to_mem.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/*
 * load an empty node with no cells
 * load a full node with 30 cells + 1 rigtmost pointer
 * load a node with 15 cells
 */ 

typedef enum {
    TEST_NODE_EMPTY,
    TEST_NODE_FULL,
    TEST_NODE_DEFAULT
} TEST_NODE_TYPE;

void write_empty_node(uint8_t *node) {
    *((uint32_t*)(node + NUM_CELLS_OFFSET)) = 0;
}

void write_node_with_cells(uint8_t *node, uint32_t num_cells_to_write) {
    if (num_cells_to_write > 30) {
        perror("Number of cells cannot be that large");
        exit(EXIT_FAILURE);
    }

    const uint32_t KEY_VALUE_JOINT_SIZE = KEY_SIZE + VALUE_SIZE;
    const uint32_t FIRST_VALUE_OFFSET = KEY_OFFSET + KEY_SIZE;
    *((uint32_t*)(node + NUM_CELLS_OFFSET)) = num_cells_to_write;
    for (uint32_t i = 0; i < num_cells_to_write; i++) {
        *((uint32_t*)(node + KEY_OFFSET + (i * KEY_VALUE_JOINT_SIZE))) = i + 1;
        *((uint32_t*)(node + FIRST_VALUE_OFFSET + (i * KEY_VALUE_JOINT_SIZE))) = i + 21;
    }
    const uint32_t right_most_offset = KEY_OFFSET + (num_cells_to_write * KEY_VALUE_JOINT_SIZE);
    *((uint32_t*)(node + right_most_offset)) = num_cells_to_write + 21;
}

void write_test_node(TEST_NODE_TYPE test_node_type) {
    const char *filename;
    switch (test_node_type) {
        case TEST_NODE_EMPTY:
            filename = "empty_node.frndb";
            break;
        case TEST_NODE_FULL:
            filename = "full_node.frndb";
            break;
        default:
            filename = "default_node.frndb";

    }
    const size_t filesize = 256;

    int fd = open(filename,
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR
    );
    if (fd == -1) {
        perror("Error opnening file");
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
    if (test_node_type == TEST_NODE_EMPTY) {
        write_empty_node(my_node);
    } else if (test_node_type == TEST_NODE_FULL) {
        write_node_with_cells(my_node, 30);
    } else {
        write_node_with_cells(my_node, 15);
    }

    if (munmap(my_node, filesize) == -1) {
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }
    close(fd);
}

void test_load_empty() {
    printf("test_load_empty:\n");
    write_test_node(TEST_NODE_EMPTY);
    const char *filename = "empty_node.frndb";
    const size_t filesize = 256;
    BTreeNode *my_node = load_node_to_mem(filename, 0);
    
    print_node_from_mem(my_node);
    printf("\n");

    free(my_node->keys);
    free(my_node->values);
    free(my_node);
}

void test_load_full() {
    printf("test_load_full:\n");
    write_test_node(TEST_NODE_FULL);
    const char *filename = "full_node.frndb";
    const size_t filesize = 256;
    BTreeNode *my_node = load_node_to_mem(filename, 0);

    print_node_from_mem(my_node);
    printf("\n");

    free(my_node->keys);
    free(my_node->values);
    free(my_node);
}

void test_load_default() {
    printf("test_load_default:\n");
    write_test_node(TEST_NODE_DEFAULT);
    const char *filename = "default_node.frndb";
    const size_t filesize = 256;
    BTreeNode *my_node = load_node_to_mem(filename, 0);

    print_node_from_mem(my_node);
    printf("\n");

    free(my_node->keys);
    free(my_node->values);
    free(my_node);
}

int main() {
    test_load_empty();
    test_load_full();
    test_load_default();

    return 0;
}
