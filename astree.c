#include "astree.h"
#include <assert.h>
#include <stdlib.h>

void ASTreeAttachBinaryBranch(ASTreeNode* root, ASTreeNode* leftNode, ASTreeNode* rightNode)
{
    assert(root != NULL);
    root->left = leftNode;
    root->right = rightNode;
}

void ASTreeNodeSetType(ASTreeNode* node, NodeType nodetype)
{
    assert(node != NULL);
    node->type = nodetype;
}

void ASTreeNodeSetData(ASTreeNode* node, char* data)
{
    assert(node != NULL);
    if(data != NULL) {
        node->szData = data;
        node->type |= NODE_DATA;
    }
}

void ASTreeNodeDelete(ASTreeNode* node)
{
    if (node == NULL)
        return;

    if (node->type & NODE_DATA)
        free(node->szData);


    ASTreeNodeDelete(node->left);
    ASTreeNodeDelete(node->right);
    free(node);
}
