#include "tree_dump.h"
#include <wchar.h>

//——————————————————————————————————————————————————————————————————————————————————————————

static TreeErr_t TreeDumpSetDebugFilePaths  (LangCtx_t* lang_ctx);
static TreeErr_t TreeDumpSetDirs            (LangCtx_t* lang_ctx);
static TreeErr_t TreeDumpSetLogFilePath     (LangCtx_t* lang_ctx);
static TreeErr_t TreeDumpMakeDirs           (LangCtx_t* lang_ctx);
static void      TreeDumpSetTime            (LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

static TreeErr_t TreeDumpSetDebugFilePaths(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeDumpSetTime(lang_ctx);

    if (TreeDumpSetDirs(lang_ctx))
        return TREE_FILE_ERROR;

    WDPRINTF(L"> creating TREE LOGS directories:\n"
             L"\tlog_dir: %ls\n"
             L"\timg_dir: %ls\n"
             L"\tdot_dir: %ls\n",
             lang_ctx->debug.log_dir,
             lang_ctx->debug.img_dir,
             lang_ctx->debug.dot_dir);

    if (TreeDumpMakeDirs(lang_ctx))
        return TREE_FILE_ERROR;

    WDPRINTF(L"> TREE LOGS directories successfully created!\n");

    if (TreeDumpSetLogFilePath(lang_ctx))
        return TREE_FILE_ERROR;

    WDPRINTF(L"> TREE LOG file path: \"%ls\"\n", lang_ctx->debug.log_file_path);

    return TREE_SUCCESS;
}

//==========================================================================================

static void TreeDumpSetTime(LangCtx_t* lang_ctx)
{
    time_t rawtime = time(NULL);

    struct tm* info = localtime(&rawtime);

    wcsftime(lang_ctx->debug.str_time, sizeof(lang_ctx->debug.str_time), L"[%Y-%m-%d_%H%M%S]", info);

    WDPRINTF(L"time = %ls\n", lang_ctx->debug.str_time);
}

//==========================================================================================

static TreeErr_t TreeDumpMakeDirs(LangCtx_t* lang_ctx)
{
    char log_dir[MAX_DIR_PATH_LEN] = "";
    char img_dir[MAX_DIR_PATH_LEN] = "";
    char dot_dir[MAX_DIR_PATH_LEN] = "";

    wcstombs(log_dir, lang_ctx->debug.log_dir, wcslen(lang_ctx->debug.log_dir));
    wcstombs(img_dir, lang_ctx->debug.img_dir, wcslen(lang_ctx->debug.img_dir));
    wcstombs(dot_dir, lang_ctx->debug.dot_dir, wcslen(lang_ctx->debug.dot_dir));

    WDPRINTF(L"log_dir = %s\n"
             L"img_dir = %s\n",
             log_dir, img_dir);

    mkdir(log_dir, 0777);
    mkdir(img_dir, 0777);
    mkdir(dot_dir, 0777);

    return TREE_SUCCESS;
}

//==========================================================================================

static TreeErr_t TreeDumpSetDirs(LangCtx_t* lang_ctx)
{
    // if (swprintf(lang_ctx->debug.log_dir, sizeof(lang_ctx->debug.log_dir), L"log/%ls",
    //              lang_ctx->debug.str_time) < 0)
    if (swprintf(lang_ctx->debug.log_dir, sizeof(lang_ctx->debug.log_dir), L"log") < 0)
    {
        WPRINTERR(L"Error with setting \"log_dir\"");
        return TREE_FILE_ERROR;
    }
    if (swprintf(lang_ctx->debug.img_dir, sizeof(lang_ctx->debug.img_dir), L"%ls/%ls",
                 lang_ctx->debug.log_dir, IMAGE_FILE_TYPE) < 0)
    {
        WPRINTERR(L"Error with setting \"img_dir\"");
        return TREE_FILE_ERROR;
    }
    if (swprintf(lang_ctx->debug.dot_dir, sizeof(lang_ctx->debug.dot_dir), L"%ls/dot",
                 lang_ctx->debug.log_dir) < 0)
    {
        WPRINTERR(L"Error with setting \"dot_dir\"");
        return TREE_FILE_ERROR;
    }

    return TREE_SUCCESS;
}

//==========================================================================================

static TreeErr_t TreeDumpSetLogFilePath(LangCtx_t* lang_ctx)
{
    if (swprintf(lang_ctx->debug.log_file_path, sizeof(lang_ctx->debug.log_file_path),
                 L"%ls/tree.html", lang_ctx->debug.log_dir) < 0)
    {
        WPRINTERR(L"Error with setting log_file_path");
        return TREE_FILE_ERROR;
    }

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeDump(LangCtx_t*            lang_ctx,
                   const TreeDumpInfo_t* dump_info,
                   NodeDumpType_t dump_type,
                   const char* fmt, ...)
{
    va_list args = {};
    va_start(args, fmt);

    TreeErr_t result = vTreeDump(lang_ctx, dump_info, dump_type, fmt, args);

    va_end(args);

    return result;
}

//==========================================================================================

LangErr_t LangIdTableDump(LangCtx_t* lang_ctx, IdTable_t* id_table, const char* fmt, ...)
{
    va_list args = {};
    va_start(args, fmt);

    FILE* fp = lang_ctx->debug.fp;

    fwprintf(fp, L"<pre><h4><font color=blue>");

    // vfprintf(fp, fmt, args);

    fwprintf(fp, L"</h4></font>");

    fwprintf(fp, L"vars_table [%p]:\n\n"
                L"size     = %zu\n"
                L"capacity = %zu\n\n",
                id_table->data,
                id_table->size,
                id_table->capacity);

    fwprintf(fp, L"index: ");

    for (size_t i = 0; i <id_table->capacity; i++)
    {
        fwprintf(fp, L"%12zu |", i);
    }

    fwprintf(fp, L"\nnames: ");

    for (size_t i = 0; i < id_table->capacity; i++)
    {
        fwprintf(fp, L"%12ls |", id_table->data[i]);
    }

    va_end(args);

    fflush(fp);

    return LANG_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeReadBufferDump(LangCtx_t* lang_ctx, const char* fmt, ...)
{
    assert(fmt != NULL);

    int      pos    = (int) (lang_ctx->cur_symbol_ptr - lang_ctx->buffer);
    wchar_t* buffer = lang_ctx->buffer;

    va_list args = {};
    va_start(args, fmt);

    FILE* fp = lang_ctx->debug.fp;

    fwprintf(fp, L"<pre><h4><font color=green>");

    // vfprintf(fp, fmt, args);

    fwprintf(fp, L"</h4></font>\n"
                L"<font color=gray>");

    fwprintf(fp, L"\"");

    for (int i = 0; i < pos; i++)
    {
        fwprintf(fp, L"%lc", buffer[i]);
    }

    fwprintf(fp, L"</font><font color=red>%c</font>", buffer[pos]);

    if (*(buffer + pos) != '\0')
    {
        fwprintf(fp, L"<font color=blue>%ls</font>\n\n", buffer + pos + 1);
    }

    fwprintf(fp, L"\"");

    va_end(args);

    fflush(fp);

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t vTreeDump(LangCtx_t* lang_ctx,
                    const TreeDumpInfo_t* dump_info,
                    NodeDumpType_t dump_type,
                    const char* fmt, va_list args)
{
    assert(dump_info != NULL);
    assert(lang_ctx  != NULL);

    Tree_t* tree = &lang_ctx->tree;
    FILE*   fp   = lang_ctx->debug.fp;

    fwprintf(fp, L"<pre>\n<h3><font color=blue>");

    //FIXME -
    // vfprintf(fp, fmt, args);

    fwprintf(fp, L"</font></h3>");

    fwprintf(fp, dump_info->error == TREE_SUCCESS ?
                L"<font color=green><b>" :
                L"<font color=red><b>ERROR: ");

    fwprintf(fp, L"%s (code %d)</b></font>\n"
                 L"TREE DUMP called from %s at %s:%d\n\n",
                 TREE_STR_ERRORS[dump_info->error],
                 dump_info->error,
                 dump_info->func,
                 dump_info->file,
                 dump_info->line);

    fwprintf(fp, L"tree [%p]:\n\n"
                 L"size  = %zu;\n"
                 L"dummy = %p;\n",
                 tree, tree->size, tree->dummy);

    fwprintf(fp, L"code = \"%ls\"\n\n", lang_ctx->buffer);

    TreeErr_t graph_error = TREE_SUCCESS;

    if ((graph_error = TreeGraphDump(lang_ctx, dump_type)))
    {
        fflush(fp);
        return graph_error;
    }

    int image_width = tree->size <= 5 ? 25 : 50;

    fwprintf(fp, L"\n<img src = svg/%ls.svg width = %d%%>\n\n"
                 L"============================================================="
                 L"=============================================================\n\n",
                 lang_ctx->debug.graph_file_name, image_width);

    fflush(fp);

    return TREE_SUCCESS;
}

//==========================================================================================

TreeErr_t TreeOpenLogFile(LangCtx_t* lang_ctx)
{
    if (TreeDumpSetDebugFilePaths(lang_ctx))
        return TREE_FILE_ERROR;

    char log_fp[MAX_FILE_NAME_LEN] = "";

    wcstombs(log_fp, lang_ctx->debug.log_file_path, wcslen(lang_ctx->debug.log_file_path));

    lang_ctx->debug.fp = fopen(log_fp, "w");

    if (lang_ctx->debug.fp == NULL)
    {
        PRINTERR(L"Opening logfile %ls failed", lang_ctx->debug.log_file_path);
        return TREE_FILE_ERROR;
    }

    return TREE_SUCCESS;
}

//==========================================================================================

void TreeCloseLogFile(LangCtx_t* lang_ctx)
{
    fclose(lang_ctx->debug.fp);
    lang_ctx->debug.fp = NULL;
}

//==========================================================================================

static void ASTNodeDump(TreeNode_t* node,     FILE*          fp,
                        LangCtx_t*  lang_ctx, NodeDumpType_t dump_type);

//——————————————————————————————————————————————————————————————————————————————————————————

TreeErr_t GraphDump(LangCtx_t*     lang_ctx,  TreeNode_t*    node, const TreeDumpInfo_t* dump_info,
                    NodeDumpType_t dump_type, const wchar_t* fmt, ...)
{
    assert(lang_ctx != NULL);

    Tree_t*   tree  = &lang_ctx->tree;
    TreeErr_t error = TREE_SUCCESS;

    FILE* fp = lang_ctx->debug.fp;

    if (fp == NULL)
    {
        WPRINTERR("fp is NULL");
        return TREE_SUCCESS;
    }

    fwprintf(fp, L"<pre>\n<h3><font color=blue>");

    va_list args = {};

    va_start(args, fmt);

    vfwprintf(fp, fmt, args);

    va_end(args);

    fwprintf(fp, L"</font></h3>");

    fwprintf(fp, dump_info->error == TREE_SUCCESS ?
                L"<font color=green><b>" :
                L"<font color=red><b>ERROR: ");

    fwprintf(fp, L"%s (code %d)</b></font>\n"
                 L"TREE DUMP called from %s at %s:%d\n\n",
                 TREE_STR_ERRORS[dump_info->error],
                 dump_info->error,
                 dump_info->func,
                 dump_info->file,
                 dump_info->line);

    fwprintf(fp, L"tree [%p]:\n\n"
                 L"size  = %zu;\n"
                 L"dummy = %p;\n",
                 tree, tree->size, tree->dummy);

    if (tree == NULL)
    {
        WPRINTERR(L"TREE_NULL");
        return    TREE_NULL;
    }

    SetGraphFilepaths(lang_ctx);

    char dot_fp[MAX_FILE_NAME_LEN] = "";
    wcstombs(dot_fp, lang_ctx->debug.dot_file_path, MAX_FILE_NAME_LEN);

    FILE* dot_file = fopen(dot_fp, "w");

    lang_ctx->debug.graphs_count++;

    if (dot_file == NULL)
    {
        WPRINTERR(L"Failed opening dotfile %s", dot_fp);
        return TREE_DUMP_ERROR;
    }

    DumpGraphTitle(dot_file);

    if (node != NULL)
        ASTNodeDump(node, dot_file, lang_ctx, dump_type);
    else
        fwprintf(fp, L"<font color=red><b> NODE IS A NULL POINTER </b></font>\n");

    fwprintf(dot_file, L"}\n");

    fclose(dot_file);

    if ((error = TreeConvertGraphFile(lang_ctx)))
        return error;

    int image_width = tree->size <= 5 ? 25 : 50;

    fwprintf(lang_ctx->debug.fp, L"\n<img src = svg/%ls.svg width = %d%%>\n\n"
                                 L"============================================================="
                                 L"=============================================================\n\n",
                                  lang_ctx->debug.graph_file_name, image_width);

    return TREE_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

static void DumpDefaultTreeNode(NodeDumpParams_t* params, FILE* fp);

//——————————————————————————————————————————————————————————————————————————————————————————

TreeErr_t TreeGraphDump(LangCtx_t* lang_ctx, NodeDumpType_t dump_type)
{
    assert(lang_ctx != NULL);

    Tree_t*   tree  = &lang_ctx->tree;
    TreeErr_t error = TREE_SUCCESS;

    if (tree == NULL)
    {
        PRINTERR("TREE_NULL");
        return    TREE_NULL;
    }

    SetGraphFilepaths(lang_ctx);

    char dot_fp[MAX_FILE_NAME_LEN] = "";
    wcstombs(dot_fp, lang_ctx->debug.dot_file_path, wcslen(lang_ctx->debug.dot_file_path));

    FILE* dot_file = fopen(dot_fp, "w");

    lang_ctx->debug.graphs_count++;

    if (dot_file == NULL)
    {
        PRINTERR("Failed opening logfile");
        return TREE_DUMP_ERROR;
    }

    DumpGraphTitle(dot_file);

//     NodeDumpParams_t dummy_params = DUMMY_NODE_PARAMS;
//
//     dummy_params.dump_type = dump_type;
//
//     swprintf(dummy_params.name,     sizeof(dummy_params.name),     L"dummy: node%p", tree->dummy);
//     swprintf(dummy_params.str_data, sizeof(dummy_params.str_data), L"type = PZN | value = PZN");
//
//     DumpDefaultTreeNode(&dummy_params, dot_file);

    if (tree->dummy != NULL)
    {
        ASTNodeDump(tree->dummy, dot_file, lang_ctx, dump_type);
    }

    fwprintf(dot_file, L"}\n");

    fclose(dot_file);

    if ((error = TreeConvertGraphFile(lang_ctx)))
        return error;

    return TREE_SUCCESS;
}

//==========================================================================================

void SetGraphFilepaths(LangCtx_t* lang_ctx)
{
    swprintf(lang_ctx->debug.graph_file_name,
             sizeof(lang_ctx->debug.graph_file_name),
             L"graph_%04d",
             lang_ctx->debug.graphs_count);

    swprintf(lang_ctx->debug.dot_file_path,
             sizeof(lang_ctx->debug.dot_file_path),
             L"%ls/%ls.dot",
             lang_ctx->debug.dot_dir,
             lang_ctx->debug.graph_file_name);

    swprintf(lang_ctx->debug.img_file_path,
             sizeof(lang_ctx->debug.img_file_path),
             L"%ls/%ls.%ls",
             lang_ctx->debug.img_dir,
             lang_ctx->debug.graph_file_name,
             IMAGE_FILE_TYPE);
}

//==========================================================================================

void DumpGraphTitle(FILE* dot_file)
{
    assert(dot_file != NULL);

    fwprintf(dot_file,
    L"digraph Tree\n{\n\t"
    LR"(ranksep=0.75;
    nodesep=0.5;
    node [
        fontname  = "Arial",
        shape     = "Mrecord",
        style     = "filled",
        color     = "#3E3A22",
        fillcolor = "#E3DFC9",
        fontcolor = "#3E3A22"
    ];)"
    L"\n");
}

//==========================================================================================

TreeErr_t TreeConvertGraphFile(LangCtx_t* lang_ctx)
{
    wchar_t command[MAX_COMMAND_LEN] = {};

    swprintf(command, sizeof(command), L"dot %ls -T %ls -o %ls",
                                       lang_ctx->debug.dot_file_path,
                                       IMAGE_FILE_TYPE,
                                       lang_ctx->debug.img_file_path);

    char command_ch[MAX_COMMAND_LEN] = {};

    wcstombs(command_ch, command, wcslen(command));

    int result = system(command_ch);

    if (result != 0)
    {
        PRINTERR("TREE_SYSTEM_FUNC_ERROR");
        return    TREE_SYSTEM_FUNC_ERROR;
    }

    return TREE_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTDumpNodeWithEdges(TreeNode_t* node,     FILE*          fp,
                                 LangCtx_t*  lang_ctx, NodeDumpType_t dump_type);

static void DumpNode         (NodeDumpParams_t* params, FILE* fp);
static void DumpEdge         (EdgeDumpParams_t* params, FILE* fp, int side);
static void ASTDumpSingleNode(NodeDumpParams_t* params, FILE* fp, LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTNodeDump(TreeNode_t* node,     FILE*          fp,
                        LangCtx_t*  lang_ctx, NodeDumpType_t dump_type)
{
    assert(lang_ctx != NULL);
    assert(node     != NULL);
    assert(fp       != NULL);

    if (node->left)
        ASTNodeDump(node->left, fp, lang_ctx, dump_type);

    ASTDumpNodeWithEdges(node, fp, lang_ctx, dump_type);

    if (node->right)
        ASTNodeDump(node->right, fp, lang_ctx, dump_type);
}

//==========================================================================================

static void ASTDumpNodeWithEdges(TreeNode_t* node,     FILE*          fp,
                                 LangCtx_t*  lang_ctx, NodeDumpType_t dump_type)
{
    NodeDumpParams_t node_params = {};

    node_params.node      = node;
    node_params.dump_type = dump_type;

    swprintf(node_params.name, sizeof(node_params.name), L"node%p", node);

    ASTDumpSingleNode(&node_params, fp, lang_ctx);

    //----------------------------------------------------------------------//

    EdgeDumpParams_t edge_params = { .color = DEFAULT_EDGE_COLOR };

    swprintf(edge_params.node1, sizeof(edge_params.node1), L"node%p", node);

    if (node->left != NULL)
    {
        swprintf(edge_params.node2, sizeof(edge_params.node2), L"node%p", node->left);
        DumpEdge(&edge_params, fp, 0);
    }
    if (node->right != NULL)
    {
        swprintf(edge_params.node2, sizeof(edge_params.node2), L"node%p", node->right);
        DumpEdge(&edge_params, fp, 1);
    }
}

//——————————————————————————————————————————————————————————————————————————————————————————

static void DumpNodeDataOperator    (NodeDumpParams_t* params, LangCtx_t* lang_ctx);
static void DumpNodeDataIdentifier  (NodeDumpParams_t* params, LangCtx_t* lang_ctx);
static void DumpNodeDataNumber      (NodeDumpParams_t* params, LangCtx_t* lang_ctx);

//==========================================================================================

void (* const DUMP_NODE_DATA_TABLE[]) (NodeDumpParams_t* params, LangCtx_t* lang_ctx) =
{
    [TYPE_OP       ] = DumpNodeDataOperator,
    [TYPE_ID       ] = DumpNodeDataIdentifier,
    [TYPE_NUM      ] = DumpNodeDataNumber,
    [TYPE_VAR      ] = DumpNodeDataIdentifier,
    [TYPE_VAR_DECL ] = DumpNodeDataIdentifier,
    [TYPE_FUNC_CALL] = DumpNodeDataIdentifier,
    [TYPE_FUNC_DECL] = DumpNodeDataIdentifier
};

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTDumpSingleNode(NodeDumpParams_t* params, FILE* fp, LangCtx_t* lang_ctx)
{
    assert(lang_ctx != NULL);
    assert(params   != NULL);
    assert(fp       != NULL);

    DUMP_NODE_DATA_TABLE[params->node->data.type](params, lang_ctx);

    params->color     = TYPE_CASES_TABLE[params->node->data.type].color;
    params->fillcolor = TYPE_CASES_TABLE[params->node->data.type].fillcolor;
    params->fontcolor = TYPE_CASES_TABLE[params->node->data.type].fontcolor;
    params->shape     = TYPE_CASES_TABLE[params->node->data.type].shape;

    DumpDefaultTreeNode(params, fp);
}

//==========================================================================================

static void DumpNodeDataOperator(NodeDumpParams_t* params, LangCtx_t* lang_ctx)
{
    assert(lang_ctx != NULL);
    assert(params   != NULL);

    swprintf(params->str_data, sizeof(params->str_data),
             L"type = %s | code = %s | value = %ls",
             TYPE_CASES_TABLE[params->node->data.type].name,
             OP_CASES_TABLE[params->node->data.value.opcode].code_str,
             OP_CASES_TABLE[params->node->data.value.opcode].name);
}

//==========================================================================================

static void DumpNodeDataIdentifier(NodeDumpParams_t* params, LangCtx_t* lang_ctx)
{
    assert(lang_ctx != NULL);
    assert(params   != NULL);

    swprintf(params->str_data, sizeof(params->str_data),
             L"type = %s | value = %ls (%zu)",
             TYPE_CASES_TABLE[params->node->data.type].name,
             lang_ctx->names_pool.data[params->node->data.value.id],
             params->node->data.value.id);
}

//==========================================================================================

static void DumpNodeDataNumber(NodeDumpParams_t* params, LangCtx_t* lang_ctx)
{
    assert(lang_ctx != NULL);
    assert(params   != NULL);

    swprintf(params->str_data, sizeof(params->str_data),
             L"type = %s | value = %lg",
             TYPE_CASES_TABLE[params->node->data.type].name,
             params->node->data.value.number);
}

//——————————————————————————————————————————————————————————————————————————————————————————

static inline int DumpAllowsRecordLabel(NodeDumpParams_t* params);

//——————————————————————————————————————————————————————————————————————————————————————————

static void DumpDefaultTreeNode(NodeDumpParams_t* params, FILE* fp)
{
    assert(params != NULL);
    assert(fp     != NULL);

    if (wcscmp(params->name, L"") == 0)
        swprintf(params->name, sizeof(params->name), L"node%p", params->node);

    TreeNode_t* node = params->node;

    if (params->dump_type == DUMP_SHORT)
    {
        swprintf(params->label, sizeof(params->label), L"{ %ls | { <left> LEFT | <right> RIGHT }}", params->str_data);
    }
    else if (DumpAllowsRecordLabel(params))
    {
        swprintf(params->label, sizeof(params->label),
                 L"{ %p | %ls | { <left> left = %p | <right> right = %p }}",
                 node, params->str_data, node->left, node->right);
    }
    else
    {
        swprintf(params->label, sizeof(params->label),
                 L"%p \\n %ls \\n left = %p \\n right = %p",
                 node, params->str_data, node->left, node->right);
    }

    DumpNode(params, fp);
}

//==========================================================================================

static inline int DumpAllowsRecordLabel(NodeDumpParams_t* params)
{
    return ((params->shape != NULL) && (wcscmp(params->shape, L"record" ) == 0
                                    ||  wcscmp(params->shape, L"Mrecord") == 0));
}

//——————————————————————————————————————————————————————————————————————————————————————————

static void PrintArg(const char*    arg_name,
                     const wchar_t* arg_value,
                     bool*          is_first_arg,
                     FILE*          fp);

//——————————————————————————————————————————————————————————————————————————————————————————

static void DumpNode(NodeDumpParams_t* params, FILE* fp)
{
    assert(params != NULL);
    assert(fp     != NULL);

    fwprintf(fp, L"\t%ls", params->name);

    bool is_first_arg = true;

    PrintArg("label",     params->label,     &is_first_arg, fp);
    PrintArg("color",     params->color,     &is_first_arg, fp);
    PrintArg("fillcolor", params->fillcolor, &is_first_arg, fp);
    PrintArg("fontcolor", params->fontcolor, &is_first_arg, fp);
    PrintArg("shape",     params->shape,     &is_first_arg, fp);

    if (!is_first_arg)
    {
        fwprintf(fp, L"]");
    }

    fwprintf(fp, L";\n");
}

//==========================================================================================

// left side = 0; right side = 1;
static void DumpEdge(EdgeDumpParams_t* params, FILE* fp, int side)
{
    assert(params != NULL);
    assert(fp     != NULL);

    if (side == 0) // left
        fwprintf(fp, L"\t%ls:left->%ls", params->node1, params->node2);
    else
        fwprintf(fp, L"\t%ls:right->%ls", params->node1, params->node2);

    bool is_first_arg = true;

    PrintArg("color",      params->color,      &is_first_arg, fp);
    PrintArg("constraint", params->constraint, &is_first_arg, fp);
    PrintArg("dir",        params->dir,        &is_first_arg, fp);
    PrintArg("style",      params->style,      &is_first_arg, fp);
    PrintArg("arrowhead",  params->arrowhead,  &is_first_arg, fp);
    PrintArg("arrowtail",  params->arrowtail,  &is_first_arg, fp);
    PrintArg("label",      params->label,      &is_first_arg, fp);

    if (!is_first_arg)
    {
        fwprintf(fp, L"]");
    }

    fwprintf(fp, L";\n");
}

//==========================================================================================

static void PrintArg(const char*    arg_name,
                     const wchar_t* arg_value,
                     bool*          is_first_arg,
                     FILE*          fp)
{
    assert(is_first_arg != NULL);
    assert(arg_name     != NULL);
    assert(fp           != NULL);

    if (arg_value == NULL)
        return;

    if (*is_first_arg)
    {
        fwprintf(fp, L" [");
        *is_first_arg = false;
    }
    else
    {
        fwprintf(fp, L", ");
    }

    fwprintf(fp, L"%s = \"%ls\"", arg_name, arg_value);
}

//==========================================================================================
