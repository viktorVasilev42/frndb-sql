#include "../disk_to_struct/load_node_to_mem.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

typedef struct {
    uint32_t num_neighbors;
    BTreeNode** neighbors;
    uint32_t separators[];
} NeighborsResponse;

// if the function does not return NULL, the node passed to this function
// should always be freed after the function call
// after usage of the return array -> the allocated BTreeNodes inside the array should be freed
// and the array itself should be freed
//
// ill also need to return the seperator keys from the parent node
NeighborsResponse *get_node_neighbors_for_balance(const char *filename, BTreeNode *node) {
    if (node->is_root) return NULL;
    
    BTreeNode *parent_node = load_node_to_mem(filename, node->parent_pointer);

    int currNodeIndexInParent = -1;
    for (uint32_t i = 0; i <= parent_node->num_keys; i++) {
        if (parent_node->values[i] == node->node_id) {
            currNodeIndexInParent = i;
            break;
        }
    }

    if (currNodeIndexInParent == -1) {
        free(parent_node->keys);
        free(parent_node->values);
        free(parent_node);
        perror("Pointer to node does not exist in parent. Index structure is corrupt");
        exit(EXIT_FAILURE);
    }

    int shouldBeLeftIndex = currNodeIndexInParent - 1;
    int shouldBeRightIndex = currNodeIndexInParent + 1;
    BTreeNode **result;
    uint32_t num_neighbors;
    uint32_t separators[3];
    if (shouldBeLeftIndex < 0) {
        if (shouldBeRightIndex > parent_node->num_keys) {
            // no neighbors
            return NULL;
        } else if (shouldBeRightIndex == parent_node->num_keys) {
            // only one right neighbor
            result = malloc(2 * sizeof(BTreeNode*));
            result[0] = malloc(sizeof(BTreeNode));
            memcpy_btreenode(result[0], node);
            result[1] = load_node_to_mem(filename, parent_node->values[shouldBeRightIndex]);
            num_neighbors = 2;
            separators[0] = parent_node->keys[shouldBeRightIndex - 1];
        } else {
            // two right neighbors
            int farRightIndex = shouldBeRightIndex + 1;
            result = malloc(3 * sizeof(BTreeNode*));
            result[0] = malloc(sizeof(BTreeNode));
            memcpy_btreenode(result[0], node);
            result[1] = load_node_to_mem(filename, parent_node->values[shouldBeRightIndex]);
            result[2] = load_node_to_mem(filename, parent_node->values[farRightIndex]);
            num_neighbors = 3;
            separators[0] = parent_node->keys[shouldBeRightIndex - 1];
            separators[1] = parent_node->keys[shouldBeRightIndex];
        }
    } else {
        // left neighbor is good
        if (shouldBeRightIndex > parent_node->num_keys) {
            // but right neighbor is not good
            if (shouldBeLeftIndex == 0) {
                // only one left neighbor
                result = malloc(2 * sizeof(BTreeNode*));
                result[0] = load_node_to_mem(filename, parent_node->values[shouldBeLeftIndex]);
                result[1] = malloc(sizeof(BTreeNode));
                memcpy_btreenode(result[1], node);
                num_neighbors = 2;
                separators[0] = parent_node->keys[shouldBeLeftIndex];
            } else {
                // two left neighbors
                int farLeftIndex = shouldBeLeftIndex - 1;
                result = malloc(3 * sizeof(BTreeNode*));
                result[0] = load_node_to_mem(filename, parent_node->values[farLeftIndex]);
                result[1] = load_node_to_mem(filename, parent_node->values[shouldBeLeftIndex]);
                result[2] = malloc(sizeof(BTreeNode));
                memcpy_btreenode(result[2], node);
                num_neighbors = 3;
                separators[0] = parent_node->keys[shouldBeLeftIndex];
                separators[1] = parent_node->keys[shouldBeLeftIndex - 1];
            }
        } else {
            // left and right are both good
            result = malloc(3 * sizeof(BTreeNode*));
            result[0] = load_node_to_mem(filename, parent_node->values[shouldBeLeftIndex]);
            result[1] = malloc(sizeof(BTreeNode));
            memcpy_btreenode(result[1], node);
            result[2] = load_node_to_mem(filename, parent_node->values[shouldBeRightIndex]);
            num_neighbors = 3;
            separators[0] = parent_node->keys[shouldBeLeftIndex];
            separators[1] = parent_node->keys[shouldBeLeftIndex + 1];
        }
    }

    NeighborsResponse *response = malloc(sizeof(NeighborsResponse) + ((num_neighbors - 1) * sizeof(uint32_t)));
    response->num_neighbors = num_neighbors;
    response->neighbors = result;
    for (uint32_t i = 0; i < num_neighbors - 1; i++) {
        response->separators[i] = separators[i];
    }

    free(parent_node->keys);
    free(parent_node->values);
    free(parent_node);
    return response;
}

BTreeNode* find_where_to_insert(uint32_t new_key, const char* filename, uint32_t node_id) {
    BTreeNode *node = load_node_to_mem(filename, node_id);

    if (node->node_type == NODE_LEAF) {
        return node;
    }

    for (uint32_t i = 0; i < node->num_keys; i++) {
        if (new_key < node->keys[i]) {
            const uint32_t next_hop = node->values[i];
            free(node->keys);
            free(node->values);
            free(node);
            return find_where_to_insert(new_key, filename, next_hop);
        }
    }

    const uint32_t next_hop = node->values[node->num_keys];
    free(node->keys);
    free(node->values);
    free(node);
    return find_where_to_insert(new_key, filename, next_hop);
}

void write_cell_at_position(const char* filename,
        uint32_t new_key, uint32_t new_value,
        uint32_t position,
        BTreeNode *pass_node
) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        free(pass_node->keys);
        free(pass_node->values);
        free(pass_node);
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    const uint32_t os_page_size = 4096;
    const uint32_t os_page_multiple = (pass_node->node_id * 256) / os_page_size;
    const uint32_t offset_to_read_from = os_page_multiple * os_page_size;
    const uint32_t raw_offset = pass_node->node_id * 256;
    const uint32_t KEY_VALUE_JOINT_SIZE = KEY_SIZE + VALUE_SIZE;
    const uint32_t FIRST_VALUE_OFFSET = KEY_OFFSET + KEY_SIZE;

    uint8_t *mapped_mem = mmap(NULL, os_page_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            offset_to_read_from
    );
    close(fd);
    if (mapped_mem == MAP_FAILED) {
        free(pass_node->keys);
        free(pass_node->values);
        free(pass_node);
        perror("Error mapping file");
        exit(EXIT_FAILURE);
    }

    uint8_t *my_node = mapped_mem + (raw_offset - offset_to_read_from);

    // shift all cells 8 bytes to the right to make room for new cell at position
    for (uint32_t i = pass_node->num_keys - 1; i >= position; i--) {
        uint32_t curr_key = *((uint32_t*)(my_node + KEY_OFFSET + (i * KEY_VALUE_JOINT_SIZE)));
        uint32_t curr_value = *((uint32_t*)(my_node + FIRST_VALUE_OFFSET + (i * KEY_VALUE_JOINT_SIZE)));

        *((uint32_t*)(my_node + KEY_OFFSET + ((i + 1) * KEY_VALUE_JOINT_SIZE))) = curr_key;
        *((uint32_t*)(my_node + FIRST_VALUE_OFFSET + ((i + 1) * KEY_VALUE_JOINT_SIZE))) = curr_value;
    }

    // add new cell add position
    *((uint32_t*)(my_node + KEY_OFFSET + (position * KEY_VALUE_JOINT_SIZE))) = new_key;
    *((uint32_t*)(my_node + FIRST_VALUE_OFFSET + (position * KEY_VALUE_JOINT_SIZE))) = new_value;

    // update page header values after insert
    *((uint32_t*)(my_node + NUM_CELLS_OFFSET)) = pass_node->num_keys + 1;

    if (munmap(mapped_mem, os_page_size) == -1) {
        free(pass_node->keys);
        free(pass_node->values);
        free(pass_node);
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }
}

void insert_new_cell(const char* filename, uint32_t new_key, uint32_t new_value) {
    BTreeNode *node = find_where_to_insert(new_key, filename, 0);

    if (node->num_keys >= 30) {
        perror("Node is full");
        free(node->keys);
        free(node->values);
        free(node);
        exit(EXIT_FAILURE);
    }

    // determine position in node where new cell should be entered
    int position_to_insert = 0;
    while (new_key > node->keys[position_to_insert]) {
        position_to_insert++;
    }

    // call write_cell_at_position()
    write_cell_at_position("table_for_search.frndb", new_key, new_value, position_to_insert, node);

    free(node->keys);
    free(node->values);
    free(node);
}

int main() {
    const char *filename = "table_for_search.frndb";
    const uint32_t nodesize = 256;
    const uint32_t num_nodes = 3;

    int fd = open(filename,
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR
    );
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, nodesize * num_nodes) == -1) {
        perror("Error setting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 1
    uint8_t *curr_node = mmap(NULL, nodesize * num_nodes,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0);
    close(fd);
    if (curr_node == MAP_FAILED) {
        perror("Error mapping file");
        exit(EXIT_FAILURE);
    }

    curr_node[NODE_TYPE_OFFSET] = 0;
    curr_node[IS_ROOT_OFFSET] = 1;
    *((uint32_t*)(curr_node + PARENT_POINTER_OFFSET)) = 0;
    *((uint32_t*)(curr_node + NUM_CELLS_OFFSET)) = 1;

    *((uint32_t*)(curr_node + KEY_OFFSET)) = 5;
    *((uint32_t*)(curr_node + KEY_OFFSET + KEY_SIZE)) = 1;
    *((uint32_t*)(curr_node + KEY_OFFSET + KEY_SIZE + VALUE_SIZE)) = 2;

    // 2
    uint8_t *tmp_node = curr_node + nodesize;

    tmp_node[NODE_TYPE_OFFSET] = 1;
    tmp_node[IS_ROOT_OFFSET] = 0;
    *((uint32_t*)(tmp_node + PARENT_POINTER_OFFSET)) = 0;
    *((uint32_t*)(tmp_node + NUM_CELLS_OFFSET)) = 2;

    *((uint32_t*)(tmp_node + KEY_OFFSET)) = 2;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE)) = 100;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE + VALUE_SIZE)) = 4;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE + VALUE_SIZE + KEY_SIZE)) = 100;

    // 3
    tmp_node = tmp_node + nodesize;

    tmp_node[NODE_TYPE_OFFSET] = 1;
    tmp_node[IS_ROOT_OFFSET] = 0;
    *((uint32_t*)(tmp_node + PARENT_POINTER_OFFSET)) = 0;
    *((uint32_t*)(tmp_node + NUM_CELLS_OFFSET)) = 2;

    *((uint32_t*)(tmp_node + KEY_OFFSET)) = 6;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE)) = 100;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE + VALUE_SIZE)) = 9;
    *((uint32_t*)(tmp_node + KEY_OFFSET + KEY_SIZE + VALUE_SIZE + KEY_SIZE)) = 100;

    if (munmap(curr_node, nodesize) == -1) {
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }

    insert_new_cell("table_for_search.frndb", 3, 101);
    printf("All nodes:\n");
    for (uint32_t i = 0; i < 3; i++) {
        BTreeNode *node_read = load_node_to_mem("table_for_search.frndb", i);
        print_node_from_mem(node_read);
        printf("\n");

        free(node_read->keys);
        free(node_read->values);
        free(node_read);
    }

    printf("\n");
    printf("TEST NEIGHBORS:\n");
    BTreeNode *node_one = load_node_to_mem("table_for_search.frndb", 1);
    NeighborsResponse *neighbors_response = get_node_neighbors_for_balance(filename, node_one);
    free(node_one->keys);
    free(node_one->values);
    free(node_one);

    BTreeNode **neighbors = neighbors_response->neighbors;
    BTreeNode *one = neighbors[0];
    BTreeNode *two = neighbors[1];
    print_node_from_mem(one);
    print_node_from_mem(two);

    printf("Separators:\n");
    uint32_t num_separators = neighbors_response->num_neighbors - 1;
    for (uint32_t i = 0; i < num_separators; i++) {
        printf("%u\n", neighbors_response->separators[i]);
    }

    free(one->keys);
    free(one->values);
    free(one);
    free(two->keys);
    free(two->values);
    free(two);
    free(neighbors);
    free(neighbors_response);
    return 0;
}
