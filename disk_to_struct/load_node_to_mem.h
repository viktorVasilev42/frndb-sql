#ifndef LOAD_NODE_TO_MEM
#define LOAD_NODE_TO_MEM

#include "../first.h"
#include <stdint.h>
#include <stdbool.h>

extern const uint32_t PAGE_OVERFLOW_THRESHOLD;
extern const uint32_t PAGE_UNDERFLOW_THRESHOLD;

typedef struct {
    uint32_t node_id;
    uint32_t num_keys;
    uint32_t *keys;
    uint32_t *values;
    NodeType node_type;
    bool is_root;
    uint32_t parent_pointer;
} BTreeNode;

typedef enum {
    PAGE_OVERFLOW,
    PAGE_UNDERFLOW,
    PAGE_NOFLOW
} PageFlowType;

typedef enum {
    SIBLING_LEFT,
    SIBLING_RIGHT
} SiblingSearchDirection;

BTreeNode *node_mmap_to_struct(uint8_t *node);

BTreeNode *load_node_to_mem(const char *filename, uint32_t node_id);

PageFlowType get_page_flow_status(BTreeNode *mem_node);

void print_node_from_mem(BTreeNode *mem_node);

void memcpy_btreenode(BTreeNode *dest, BTreeNode *source);

#endif
