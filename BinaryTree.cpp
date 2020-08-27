#include "BinaryTree.h"
#include "mem.h"
#include <cstddef>

int Insert(BinaryTreeNode* addition, BinaryTreeNode** root)
{
    if (*root == NULL)
    {
        *root = addition;
        return 1;
    }
    else if (addition->key < (*root)->key)
    {
        return Insert(addition, &(*root)->Left);
    }
    else if (addition->key > (*root)->key)
    {
        return Insert(addition, &(*root)->Right);
    }
    else {
        return 0;
    }
}

BinaryTreeNode* Search(BinaryTreeNode** tree, long long key)
{
    if (!(*tree))
    {
        return NULL;
    }

    if (key < (*tree)->key)
    {
        return Search(&((*tree)->Left), key);
    }
    else if (key > (*tree)->key)
    {
        return Search(&((*tree)->Right), key);
    }
    else
    {
        return *tree;
    }
}

BinaryTreeNode* GetInTree(BinaryTreeNode* root, long long key) {

    return Search(&root, key);
}


BinaryTreeNode* AddToTree(BinaryTreeNode* root, long long key, void* data) {

	BinaryTreeNode *addition = (BinaryTreeNode*)gff_calloc(1, sizeof(BinaryTreeNode));
	if (!addition) {
		return NULL;
	}
    else {
        addition->key = key;
        addition->data = data;
    }

	if (!root) {
		return addition;
	}
    else if(!Insert(addition, &root)) {
        gff_free(addition);
        return NULL;
    }

	return addition;
}

void DestroyTree(BinaryTreeNode* root) {

    if (root) {
        DestroyTree(root->Left);
        DestroyTree(root->Right);
        if (root->data) {
            gff_free(root->data);
        }
        gff_free(root);
    }
}