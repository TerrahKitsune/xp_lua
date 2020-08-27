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

void Deletion(BinaryTreeNode** root, BinaryTreeNode* tree, long long key)
{
    if (!tree)
    {
        return;
    }
    
    BinaryTreeNode* left = tree->Left;
    BinaryTreeNode* right = tree->Right;

    if (*root == tree) {
        *root = NULL;
    }
    
    tree->Left = NULL;
    tree->Right = NULL;
    
    Deletion(root, left, key);
    Deletion(root, right, key);

    if (tree->key == key) {
        DestroyTree(tree);
    }
    else {
        Insert(tree, root);
    }    
}

int DeleteInTree(BinaryTreeNode** root, long long key) {

    BinaryTreeNode* node = Search(root, key);
    if (!node) {
        return false;
    }

    Deletion(root, *root, key);

    return true;
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