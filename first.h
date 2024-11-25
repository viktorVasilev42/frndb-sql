#ifndef FIRST_H
#define FIRST_H

#include <stdint.h>

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

typedef struct {
    uint32_t *keys;
    uint32_t *values;
} NodeKeyValuePairs;

extern const uint32_t NODE_CAP;

extern const uint32_t NODE_TYPE_SIZE;
extern const uint32_t IS_ROOT_SIZE;
extern const uint32_t PARENT_POINTER_SIZE;
extern const uint32_t NUM_CELLS_SIZE;
extern const uint32_t KEY_SIZE;
extern const uint32_t VALUE_SIZE;
extern const uint32_t CHILD_POINTER_SIZE;

extern const uint32_t NODE_TYPE_OFFSET;
extern const uint32_t IS_ROOT_OFFSET;
extern const uint32_t PARENT_POINTER_OFFSET;
extern const uint32_t NUM_CELLS_OFFSET;
extern const uint32_t KEY_OFFSET;

NodeType get_node_type(void* node);

uint8_t* get_node_is_root(void* node);

uint32_t* get_node_parent_pointer(void* node);

uint32_t* get_node_num_cells(void* node);

uint32_t* get_node_first_key(void* node);

NodeKeyValuePairs get_node_all_key_value_pairs(uint8_t* node);

#endif
