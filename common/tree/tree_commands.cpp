#include "tree_commands.h"

//==========================================================================================

#ifdef TREE_DEBUG

TreeErr_t TreeCheck(LangCtx_t*  lang_ctx,
                    const char* func,
                    const char* file,
                    int         line,
                    const char* fmt, ...)
{
    assert(func != NULL);
    assert(file != NULL);
    assert(fmt  != NULL);

    TreeErr_t verify_status = TREE_SUCCESS;

    if ((verify_status = TreeVerify(&lang_ctx->tree)))
    {
        PRINTERR("%s (TreeVerify not passed! Check \"tree.html\")", TREE_STR_ERRORS[verify_status]);

        TreeDumpInfo_t dump_info = {verify_status, func, file, line};

        va_list args = {};
        va_start(args, fmt);

        if (vTreeDump(lang_ctx, &dump_info, DUMP_FULL, fmt, args))
        {
            return TREE_DUMP_ERROR;
        }

        va_end(args);
    }

    return verify_status;
}

#endif /* TREE_DEBUG */

//==========================================================================================

TreeNode_t* LangOperatorNodeCtor(LangCtx_t* lang_ctx, Operator_t opcode, TreeNode_t* left, TreeNode_t* right)
{
    assert(lang_ctx != NULL);

    TokenData_t token_data = {};

    token_data.type         = TYPE_OP;
    token_data.value.opcode = opcode;

    return TreeNodeCtor(&lang_ctx->tree, token_data, left, right, NULL);
}

//==========================================================================================

TreeNode_t* LangIdentifierNodeCtor(LangCtx_t* lang_ctx, Identifier_t id)
{
    assert(lang_ctx != NULL);

    TokenData_t token_data = {};

    token_data.type     = TYPE_ID;
    token_data.value.id = id;

    return TreeNodeCtor(&lang_ctx->tree, token_data, NULL, NULL, NULL);
}

//==========================================================================================

TreeNode_t* LangNumberNodeCtor(LangCtx_t* lang_ctx, double number)
{
    assert(lang_ctx != NULL);

    TokenData_t token_data = {};

    token_data.type         = TYPE_NUM;
    token_data.value.number = number;

    return TreeNodeCtor(&lang_ctx->tree, token_data, NULL, NULL, NULL);
}

//==========================================================================================

TreeErr_t TreeCtor(Tree_t* tree)
{
    if (tree == NULL)
    {
        PRINTERR(L"Tree is a nullptr");
        return TREE_NULL;
    }

    tree->dummy = TreeNodeCtor(tree, {TYPE_NUM, { .number = 0.0 }}, NULL, NULL, NULL);

    if (tree->dummy == NULL)
    {
        PRINTERR(L"Dummy construction failed");
        return TREE_NULL;
    }

    tree->buffer = NULL;
    tree->size   = 1;

    // DEBUG_TREE_CHECK(tree, "ERROR DUMP AFTER CTOR");
    DPRINTF(L"> TreeCtor   END\n");

    return TREE_SUCCESS;
}

//==========================================================================================

TreeNode_t* TreeNodeCtor(Tree_t*        tree,
                         TreeElem_t     data,
                         TreeNode_t*    left,
                         TreeNode_t*    right,
                         TreeNode_t*    parent)
{
    if (tree == NULL)
    {
        PRINTERR("Tree is a nullptr");
        return NULL;
    }

    TreeNode_t* node = (TreeNode_t*) calloc(1, sizeof(TreeNode_t));

    if (node == NULL)
    {
        PRINTERR("Memory allocation for a new node failed");
        return NULL;
    }
    // DPRINTF(L"Allocated %p\n", node);

    node->parent = parent;

    node->data   = data;
    node->left   = left;
    node->right  = right;

    tree->size++;

    return node;
}

//==========================================================================================

TreeNode_t* TreeCopySubtree(Tree_t* dest_tree, TreeNode_t* node, TreeNode_t* parent)
{
    DPRINTF(L"node = %p\n", node);

    assert(dest_tree != NULL);

    if (node == NULL)
    {
        return NULL;
    }

    return TreeNodeCtor(dest_tree, node->data,
                        TreeCopySubtree(dest_tree, node->left, node),
                        TreeCopySubtree(dest_tree, node->right, node),
                        parent);
}

//==========================================================================================

TreeErr_t TreeDtor(Tree_t* tree)
{
    assert(tree != NULL);

    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeNodeDtor(tree->dummy, tree)))
    {
        return error;
    }

    if (tree->buffer)
    {
        free(tree->buffer);
        tree->buffer = NULL;
    }

    if (tree->size != 0)
    {
        PRINTERR("Tree size after dtor != 0, size = %zu", tree->size);
        return TREE_DTOR_ERROR;
    }

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeSubtreesDtor(TreeNode_t* node, Tree_t* tree)
{
    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeLeftSubtreeDtor(node, tree)))
        return error;

    if ((error = TreeRightSubtreeDtor(node, tree)))
        return error;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeLeftSubtreeDtor(TreeNode_t* node, Tree_t* tree)
{
    assert(node != NULL);

    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeSubtreeDtor(&node->left, tree)))
    {
        return error;
    }

    node->left = NULL;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeRightSubtreeDtor(TreeNode_t* node, Tree_t* tree)
{
    assert(node != NULL);

    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeSubtreeDtor(&node->right, tree)))
    {
        return error;
    }

    node->right = NULL;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeSubtreeDtor(TreeNode_t** node_ptr, Tree_t* tree)
{
    assert(node_ptr != NULL);

    if (*node_ptr == NULL)
    {
        return TREE_SUCCESS;
    }

    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeNodeDtor(*node_ptr, tree)))
    {
        return error;
    }

    *node_ptr = NULL;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeNodeDtor(TreeNode_t* node, Tree_t* tree)
{
    if (node == NULL)
        return TREE_NULL;

    TreeErr_t error = TREE_SUCCESS;

    if (node->left != NULL)
    {
        if ((error = TreeNodeDtor(node->left, tree)))
            return error;
    }

    if (node->right != NULL)
    {
        if ((error = TreeNodeDtor(node->right, tree)))
            return error;
    }

    if ((error = TreeSingleNodeDtor(node, tree)))
        return error;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeSingleNodeDtor(TreeNode_t* node, Tree_t* tree)
{
    if (node == NULL)
        return TREE_NULL;

    node->data.type         = TYPE_NUM;
    node->data.value.number = 0;

    node->left   = NULL;
    node->right  = NULL;
    node->parent = NULL;

    // cprintf(BLUE, "\t\tfreed ptr %p\n", node);

    free(node);

    tree->size--;

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeNodeVerify(const Tree_t* tree,  TreeNode_t* node,
                         size_t* calls_count, TreeNode_t* parent)
{
    assert(calls_count != NULL);
    assert(tree        != NULL);

    if (*calls_count > tree->size)
    {
        return TREE_LOOP;
    }
    if (node == NULL)
    {
        return TREE_NULL;
    }
    if (node->parent != parent)
    {
        return TREE_LOST_CONNECTION;
    }

    (*calls_count)++;

    TreeErr_t error = TREE_SUCCESS;

    if (node->left != NULL)
    {
        if ((error = TreeNodeVerify(tree, node->left, calls_count, node)))
            return error;
    }

    if (node->right != NULL)
    {
        if ((error = TreeNodeVerify(tree, node->right, calls_count, node)))
            return error;
    }

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeVerify(const Tree_t* tree)
{
    if (tree == NULL)
    {
        return TREE_NULL;
    }
    if (tree->dummy == NULL)
    {
        return TREE_NULL;
    }
    if (tree->size > TREE_MAX_SIZE)
    {
        return TREE_SIZE_EXCEEDS_MAX;
    }

    size_t calls_count = 0;
    TreeErr_t error = TREE_SUCCESS;

    if ((error = TreeNodeVerify(tree, tree->dummy, &calls_count, NULL)))
        return error;

    return TREE_SUCCESS;
}

//==========================================================================================

int CompareDoubles(double val1, double val2)
{
    if (val1 + DOUBLE_EPS < val2 - DOUBLE_EPS)
    {
        return -1;
    }
    else if (val2 + DOUBLE_EPS < val1 - DOUBLE_EPS)
    {
        return 1;
    }

    return 0;
}

//==========================================================================================
