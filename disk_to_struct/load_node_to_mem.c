#include "../first.h"
#include "load_node_to_mem.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

const uint32_t PAGE_OVERFLOW_THRESHOLD = 25;
const uint32_t PAGE_UNDERFLOW_THRESHOLD = 13;

BTreeNode *node_mmap_to_struct(uint8_t *node) {
    BTreeNode *mem_node = malloc(sizeof(BTreeNode));
  
    mem_node->num_keys = *get_node_num_cells(node);
    if (mem_node->num_keys > 30) {
        free(mem_node);
        printf(">30");
        return NULL;
    }

    mem_node->is_root = (*get_node_is_root(node) == 1);
    mem_node->node_type = get_node_type(node);
    NodeKeyValuePairs key_value_pairs = get_node_all_key_value_pairs(node);
    mem_node->keys = key_value_pairs.keys;
    mem_node->values = key_value_pairs.values;
    mem_node->parent_pointer = *get_node_parent_pointer(node);

    return mem_node;
}

BTreeNode *load_node_to_mem(const char *filename, uint32_t node_id) {
    uint32_t os_page_size = 4096;
    uint32_t os_page_multiple = (node_id * 256) / os_page_size;
    uint32_t offset_to_read_from = os_page_multiple * os_page_size;
    uint32_t raw_offset = node_id * 256;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return NULL;
    }

    uint8_t *mapped_mem = mmap(NULL, os_page_size,
            PROT_READ,
            MAP_PRIVATE,
            fd,
            offset_to_read_from
    );
    close(fd);
    if (mapped_mem == MAP_FAILED) {
        perror("Error mapping file");
        return NULL;
    }

    uint8_t *my_node = mapped_mem + (raw_offset - offset_to_read_from);

    BTreeNode *mem_node = node_mmap_to_struct(my_node);
    mem_node->node_id = node_id;

    if (munmap(mapped_mem, os_page_size) == -1) {
        perror("Error unmapping file");
        return NULL;
    }

    return mem_node;
}

PageFlowType get_page_flow_status(BTreeNode *mem_node) {
    if (mem_node->num_keys > PAGE_OVERFLOW_THRESHOLD)
        return PAGE_OVERFLOW;
    if (mem_node->num_keys < PAGE_UNDERFLOW_THRESHOLD)
        return PAGE_UNDERFLOW;

    return PAGE_NOFLOW;
}

void print_node_from_mem(BTreeNode *mem_node) {
     printf("FROM MEMORY\n");
     printf("NODE TYPE: %u\n", mem_node->node_type);
     printf("IS ROOT: %d\n", mem_node->is_root);
     printf("PARENT POINTER: %u\n", mem_node->parent_pointer);
     printf("NUM KEYS: %u\n", mem_node->num_keys);
     printf("CELLS:\n");
     for (uint32_t i = 0; i < mem_node->num_keys; i++) {
         printf("%u -> %u\n", mem_node->keys[i], mem_node->values[i]);
     }
     printf("RIGHT MOST VALUE: %u\n", mem_node->values[mem_node->num_keys]);
}

void memcpy_btreenode(BTreeNode *dest, BTreeNode *source) {
    memcpy(dest, source, sizeof(BTreeNode));
    dest->keys = malloc(source->num_keys * sizeof(uint32_t));
    dest->values = malloc((source->num_keys + 1) * sizeof(uint32_t));
    memcpy(dest->keys, source->keys, source->num_keys * sizeof(uint32_t));
    memcpy(dest->values, source->values, (source->num_keys + 1) * sizeof(uint32_t));
}

/*
 *
 * free(mem_node->keys);
 * free(mem_node->values);
 * free(mem_node);
 * close(fd);
 *
*/
