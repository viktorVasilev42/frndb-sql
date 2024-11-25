#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define M 4

struct BTreeNode {
    int num_keys;
    int keys[M - 1];
    struct BTreeNode *children[M];
    bool is_leaf;
};

static struct BTreeNode *createNode(bool is_leaf) {

    struct BTreeNode *newNode = (struct BTreeNode*) malloc(sizeof(struct BTreeNode));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->num_keys = 0;
    newNode->is_leaf = is_leaf;
    for (int i = 0; i < M; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

static void splitChild(struct BTreeNode *parent, int index) {
    struct BTreeNode *child = parent->children[index];
    struct BTreeNode *newNode = createNode(child->is_leaf);

    newNode->num_keys = M / 2 - 1;

    for (int i = 0; i < M / 2 - 1; i++) {
        newNode->keys[i] = child->keys[i + M / 2];
    }
    if (!child->is_leaf) {
        for (int i = 0; i < M / 2; i++) {
            newNode->children[i] = child->children[i + M / 2];
        }
    }

    child->num_keys = M / 2 - 1;
    
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }

    parent->children[index + 1] = newNode;

    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
    }

    parent->keys[index] = child->keys[M / 2 - 1];
    parent->num_keys++;
}

static void insertNonFull(struct BTreeNode *node, int key) {
    int i = node->num_keys - 1;
    
    if (node->is_leaf) {
        // Insert key into the sorted order
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->num_keys++;
    } else {
        // Find the child to insert the key
        while (i >= 0 && node->keys[i] > key) {
            i--;
        }
        i++;
        
        if (node->children[i]->num_keys == M - 1) {
            // Split child if it's full
            splitChild(node, i);
            
            // Determine which of the two children is the new one
            if (node->keys[i] < key) {
                i++;
            }
        }
        insertNonFull(node->children[i], key);
    }
}

void insert(struct BTreeNode **root, int key) {

    struct BTreeNode *node = *root;
    if (node == NULL) {
        *root = createNode(true);
        (*root)->keys[0] = key;
        (*root)->num_keys = 1;
        return;
    }

    if (node->num_keys == M - 1) {
        struct BTreeNode *new_root = createNode(false);
        new_root->children[0] = node;
        splitChild(new_root, 0);
        *root = new_root;
    }
    insertNonFull(*root, key);
}

void traverse(struct BTreeNode *root) {
    if (root == NULL) return;

    int i;
    for (i = 0; i < root->num_keys; i++) {
        traverse(root -> children[i]);
        printf("%d ", root->keys[i]);
    }
    traverse(root->children[i]);
}
