#include "AST_write.h"
#include "lang_funcs.h"

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTWriteNode(LangCtx_t* lang_ctx, const TreeNode_t* node, FILE* fp, int rank);
static void ASTWriteIdTable(LangCtx_t* lang_ctx, FILE* fp);

//==========================================================================================

LangErr_t ASTWriteData(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    char data_file_path[MAX_FILE_NAME_LEN] = "";

    WDPRINTF(L"ast filename = %s;", lang_ctx->ast_file_name);

    snprintf(data_file_path, sizeof(data_file_path), "AST/%s.txt", lang_ctx->ast_file_name);

    FILE* fp = fopen(data_file_path, "w");

    if (fp == NULL)
    {
        WPRINTERR("Error with opening file: %s", data_file_path);
        return LANG_FILE_ERROR;
    }

    ASTWriteNode(lang_ctx, lang_ctx->tree.dummy->right, fp, 0);

    fwprintf(fp, L"\n\n");

    fwprintf(fp, L"global variables: %zu\n", LangIdTableCountVars(&lang_ctx->main_id_table));

    ASTWriteIdTable(lang_ctx, fp);

    fclose(fp);

    WDPRINTF(L"База данных записана в файл: %s\n", data_file_path);

    return LANG_SUCCESS;
}

//==========================================================================================

static void ASTWriteIdData(LangCtx_t* lang_ctx, IdData_t* id_data, FILE* fp);

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTWriteIdTable(LangCtx_t* lang_ctx, FILE* fp)
{
    assert(fp);
    assert(lang_ctx);

    IdTable_t* id_table = &lang_ctx->main_id_table;

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].type == ID_TYPE_FUNCTION)
        {
            ASTWriteIdData(lang_ctx, &(id_table->data[i]), fp);
        }
    }
}

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTWriteIdData(LangCtx_t* lang_ctx, IdData_t* id_data, FILE* fp)
{
    assert(lang_ctx);
    assert(id_data);
    assert(fp);

    // name_index, name, memory_needed, n_params
    fwprintf(fp, L"[%zu, \"%ls\", %zu, %zu]\n",
                 id_data->name_index,
                 lang_ctx->names_pool.data[id_data->name_index],
                 id_data->memory_needed,
                 id_data->n_params);
}

//==========================================================================================

static void ASTWriteNodeData(LangCtx_t* lang_ctx, const TokenData_t* data, FILE* fp);

//——————————————————————————————————————————————————————————————————————————————————————————

static void ASTWriteNode(LangCtx_t* lang_ctx, const TreeNode_t* node, FILE* fp, int rank)
{
    assert(fp != NULL);

    for (int i = 0; i < rank; i++)
        fwprintf(fp, L"\t");

    if (node == NULL)
    {
        fwprintf(fp, L"nil\n");
        return;
    }

    fwprintf(fp, L"( ");
    ASTWriteNodeData(lang_ctx, &node->data, fp);

    if (node->left == NULL && node->right == NULL)
    {
        fwprintf(fp, L" nil nil )\n");
        return;
    }

    fwprintf(fp, L"\n");

    ASTWriteNode(lang_ctx, node->left , fp, rank + 1);
    ASTWriteNode(lang_ctx, node->right, fp, rank + 1);

    for (int i = 0; i < rank; i++)
        fwprintf(fp, L"\t");

    fwprintf(fp, L")\n");
}

//==========================================================================================

static void ASTWriteNodeData(LangCtx_t* lang_ctx, const TokenData_t* data, FILE* fp)
{
    assert(data);
    assert(fp);

    fwprintf(fp, L"\"%ls ", TYPE_CASES_TABLE[data->type].ast_format);

    switch (data->type)
    {
        case TYPE_OP:
            fwprintf(fp, L"%ls", OP_CASES_TABLE[data->value.opcode].ast_format);
            break;

        case TYPE_ID:
        case TYPE_VAR:
        case TYPE_VAR_DECL:
        case TYPE_FUNC_CALL:
        case TYPE_FUNC_DECL:
            fwprintf(fp, L"%ls", lang_ctx->names_pool.data[data->value.id]);
            break;

        case TYPE_NUM:
            fwprintf(fp, L"%lg", data->value.number);
            break;

        default:
            return;
    }

    fwprintf(fp, L"\"");
}

//==========================================================================================
