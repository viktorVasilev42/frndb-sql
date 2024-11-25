
// returns a sibling node [step] steps left or right of the passed node
// return null if there isn't such node
BTreeNode* get_node_neighbor(const char *filename, BTreeNode *node, SiblingSearchDirection direction, int step) {
    if (node->is_root) return NULL;

    BTreeNode *parent_node = load_node_to_mem(filename, node->parent_pointer);

    int currNodeIndexInParent = -1;
    // iterate over all values to find the pointer to the current node
    // including the rightmost pointer, thus <=
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

    int resultIndex = (direction == SIBLING_LEFT) ?
        currNodeIndexInParent - step :
        currNodeIndexInParent + step;

    if (resultIndex < 0 || resultIndex > parent_node->num_keys) {
        free(parent_node->keys);
        free(parent_node->values);
        free(parent_node);
        return NULL;
    }

    BTreeNode *result = load_node_to_mem(filename, parent_node->values[resultIndex]);

    free(parent_node->keys);
    free(parent_node->values);
    free(parent_node);
    return result;
}
