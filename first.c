#include "first.h"
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

const uint32_t NODE_CAP = 30;

const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t KEY_SIZE = sizeof(uint32_t);
const uint32_t VALUE_SIZE = sizeof(uint32_t);

const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t NUM_CELLS_OFFSET = PARENT_POINTER_OFFSET + PARENT_POINTER_SIZE;
const uint32_t KEY_OFFSET = NUM_CELLS_OFFSET + NUM_CELLS_SIZE;

NodeType get_node_type(void* node) {
    uint8_t *res = node + NODE_TYPE_OFFSET;
    
    return (*res == 0) ? NODE_INTERNAL : NODE_LEAF;
}

uint8_t* get_node_is_root(void* node) {
    return node + IS_ROOT_OFFSET;
}

uint32_t* get_node_parent_pointer(void* node) {
    return node + PARENT_POINTER_OFFSET;
}

uint32_t* get_node_num_cells(void* node) {
    return node + NUM_CELLS_OFFSET;
}

uint32_t* get_node_first_key(void* node) {
    return node + KEY_OFFSET;
}

NodeKeyValuePairs get_node_all_key_value_pairs(uint8_t* node) {
    uint32_t num_cells = *get_node_num_cells(node);
    const uint32_t KEY_VALUE_JOINT_SIZE = KEY_SIZE + VALUE_SIZE;
    const uint32_t FIRST_VALUE_OFFSET = KEY_OFFSET + KEY_SIZE;

    NodeKeyValuePairs result;
    result.keys = malloc(num_cells * sizeof(uint32_t));
    result.values = malloc(num_cells * sizeof(uint32_t) + 1);
    for (uint32_t i = 0; i < num_cells; i++) {
        result.keys[i] = *((uint32_t*)(node + KEY_OFFSET + (i * KEY_VALUE_JOINT_SIZE)));
        result.values[i] = *((uint32_t*)(node + FIRST_VALUE_OFFSET + (i * KEY_VALUE_JOINT_SIZE)));
    }
    result.values[num_cells] = 
        *((uint32_t*)(node + KEY_OFFSET + (num_cells * KEY_VALUE_JOINT_SIZE)));

    return result;
}
