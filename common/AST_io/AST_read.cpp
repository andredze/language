#include "AST_read.h"

//==========================================================================================

static void SkipSpaces(wchar_t* buffer, ssize_t* pos);
static int  SkipLetter(wchar_t* buffer, ssize_t* pos, wchar_t letter);
static void ReadIdTable(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos);
static LangErr_t ReadGlobalsCount(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t ASTReadData(LangCtx_t* lang_ctx, char* ast_file_path)
{
    assert(lang_ctx);

    Tree_t* tree = &lang_ctx->tree;

    FILE* fp = fopen(ast_file_path, "r");

    if (fp == NULL)
    {
        WPRINTERR(L"Error with opening file: %s", ast_file_path);
        return LANG_FILE_ERROR;
    }

    wchar_t* buffer = NULL;

    if (ReadFile(fp, &buffer, ast_file_path, &lang_ctx->buffer_size))
    {
        WPRINTERR("Error with reading file");
        free(buffer);
        return LANG_FILE_ERROR;
    }

    WDPRINTF(L"READ BUFFER: %ls\n\n"
             L"---------------------------------------------------",
             buffer);

    strcpy(lang_ctx->ast_file_name, GetFileName(ast_file_path));

    ssize_t i = 0;

    if (ReadNode(lang_ctx, &tree->dummy->right, buffer, &i))
    {
        free(buffer);
        return LANG_TREE_ERROR;
    }
    if (ReadGlobalsCount(lang_ctx, buffer, &i))
    {
        free(buffer);
        return LANG_TREE_ERROR;
    }

    ReadIdTable(lang_ctx, buffer, &i);

    free(buffer);

    TREE_CALL_DUMP(lang_ctx, "DUMP AFTER TREE READ DATA %s", ast_file_path);

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t ReadGlobalsCount(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos)
{
    assert(lang_ctx);
    assert(buffer);
    assert(pos);

    int symbols_count = 0;

    if (swscanf(&buffer[*pos], L" global variables: %zu\n%n",
                &lang_ctx->global_vars_count, &symbols_count) != 1)
    {
        WPRINTERR(L"Error: no global vars count in AST\n");
        return LANG_WRONG_AST_FORMAT;
    }

    *pos = *pos + symbols_count;

    return LANG_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

static bool ReadIdData(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos);

//——————————————————————————————————————————————————————————————————————————————————————————

static void ReadIdTable(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos)
{
    assert(lang_ctx);
    assert(buffer);
    assert(pos);

    WDPRINTF(L"Reading Id Table\n");

    bool got_data = false;

    do
    {
        got_data = ReadIdData(lang_ctx, buffer, pos);
    }
    while (got_data);
}

//==========================================================================================

static bool ReadIdData(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos)
{
    assert(lang_ctx);
    assert(buffer);
    assert(pos);

    IdData_t id_data = {};
    int symbols_read = 0;

    wchar_t id_buff[MAX_IDENTIFIER_LEN] = {};
    int ret_value = 0;

    if ((ret_value = swscanf(&buffer[*pos], L" [%zu, \"%[^\"]\", %zu, %zu]\n%n",
                             &id_data.name_index,
                             id_buff,
                             &id_data.memory_needed,
                             &id_data.n_params,
                             &symbols_read)) != 4)
    {
        wprintf(L"ret_value = %d\n", ret_value);
        // return false;
    }

    wprintf(L"id_buff = %ls\n", id_buff);

    id_data.name = lang_ctx->names_pool.data[id_data.name_index];

    LangIdTablePush(&lang_ctx->main_id_table, &id_data);

    *pos = *pos + symbols_read;

    if (buffer[*pos] == '\0')
    {
        return false;
    }

    return true;
}

//==========================================================================================

LangErr_t ReadNode(LangCtx_t* lang_ctx, TreeNode_t** node, wchar_t* buffer, ssize_t* pos)
{
    assert(lang_ctx != NULL);
    assert(buffer   != NULL);
    assert(node     != NULL);

    LangErr_t error = LANG_SUCCESS;
    wchar_t first_char = buffer[*pos];

    if (first_char == L'(')
    {
        (*pos)++;
        SkipSpaces (buffer, pos);
        // BUFFER_DUMP(buffer, *pos, "BUFFER DUMP SKIPPING OPENING BRACKET");

        TokenData_t data = {};

        if ((error = ReadNodeData(lang_ctx, buffer, pos, &data)))
            return error;

        *node = TreeNodeCtor(&lang_ctx->tree, data, NULL, NULL, NULL);

        TREE_CALL_DUMP(lang_ctx, "DUMP AFTER NODE CTOR in READ AST");

        if ((error = ReadNode(lang_ctx, &(*node)->left,  buffer, pos)))
            return error;

        if ((error = ReadNode(lang_ctx, &(*node)->right, buffer, pos)))
            return error;

        if (SkipLetter(buffer, pos, L')'))
            return LANG_INVALID_AST_INPUT;

        SkipSpaces(buffer, pos);
    }
    else if (wcsncmp(&buffer[*pos], L"nil", 3) == 0)
    {
        (*pos) += 3;
        SkipSpaces (buffer, pos);
    }
    else
    {
        WPRINTERR(L"Syntax error in tree data (unknown symbol = \"%lc\" )\n", first_char);
        return LANG_INVALID_AST_INPUT;
    }

    return LANG_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t GetNodeData(LangCtx_t* lang_ctx, TokenData_t* node_data, wchar_t* word);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t ReadNodeData(LangCtx_t* lang_ctx, wchar_t* buffer, ssize_t* pos, TokenData_t* node_data)
{
    assert(node_data != NULL);
    assert(buffer    != NULL);
    assert(pos       != NULL);

    int data_len = 0;

    if (swscanf(&buffer[*pos], L"\"%*[^\"]\"%n", &data_len) != 0)
    {
        PRINTERR("Error with reading data");
        return LANG_INVALID_AST_INPUT;
    }

    /* node_data points to the start of the word in buffer */
    wchar_t* word = buffer + *pos + 1;

    /* moving pos to the next word */
    (*pos) += data_len;

    /* setting null-terminator for node_data (instead of closing quote) */
    buffer[*pos - 1] = '\0';

    GetNodeData(lang_ctx, node_data, word);

    SkipSpaces(buffer, pos);

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t GetNodeDataOp (TokenData_t* node_data, wchar_t* string_data);
static LangErr_t GetNodeDataNum(TokenData_t* node_data, wchar_t* string_data);
static LangErr_t GetNodeDataId (LangCtx_t* lang_ctx, TokenData_t* node_data, wchar_t* string_data);

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t GetNodeData(LangCtx_t* lang_ctx, TokenData_t* node_data, wchar_t* word)
{
    assert(node_data);
    assert(lang_ctx);
    assert(word);

    wchar_t type_name  [MAX_BUFFER_LEN] = {};
    wchar_t string_data[MAX_BUFFER_LEN] = {};

    if (swscanf(word, L"%ls %ls", type_name, string_data) != 2)
    {
        WPRINTERR("swscanf in AST read failed");
        return LANG_INVALID_AST_INPUT;
    }

    bool got_type = false;

    for (size_t i = 0; i < TYPES_COUNT; i++)
    {
        if (wcscmp(type_name, TYPE_CASES_TABLE[i].ast_format) == 0)
        {
            node_data->type = TYPE_CASES_TABLE[i].type;
            got_type = true;
            break;
        }
    }
    if (!got_type)
    {
        WPRINTERR(L"unknown token type in AST read");
        return LANG_INVALID_AST_INPUT;
    }

    switch (node_data->type)
    {
        case TYPE_OP:
            return GetNodeDataOp(node_data, string_data);

        case TYPE_NUM:
            return GetNodeDataNum(node_data, string_data);

        case TYPE_ID:
            WPRINTERR("Type identifier should not be in back end");
            return LANG_INVALID_AST_INPUT;

        case TYPE_VAR:
        case TYPE_VAR_DECL:
        case TYPE_FUNC_DECL:
        case TYPE_FUNC_CALL:
            return GetNodeDataId(lang_ctx, node_data, string_data);

        default:
            return LANG_INVALID_AST_INPUT;
    }

    return LANG_INVALID_AST_INPUT;
}

//==========================================================================================

static LangErr_t GetNodeDataOp(TokenData_t* node_data, wchar_t* string_data)
{
    assert(string_data != NULL);
    assert(node_data   != NULL);

    for (size_t opcode = 0; opcode < OPERATORS_COUNT; opcode++)
    {
        const wchar_t* opname = OP_CASES_TABLE[opcode].ast_format;

        if (wcscmp(string_data, opname) == 0)
        {
            node_data->type         = TYPE_OP;
            node_data->value.opcode = (Operator_t) opcode;

            return LANG_SUCCESS;
        }
    }

    WPRINTERR("Unknown operator in AST");
    return LANG_INVALID_AST_INPUT;
}

//==========================================================================================

static LangErr_t GetNodeDataNum(TokenData_t* node_data, wchar_t* string_data)
{
    assert(string_data != NULL);
    assert(node_data   != NULL);

    if (string_data[0] != L'-' && !iswdigit(string_data[0]))
    {
        WPRINTERR("Not a number in AST with type number");
        return LANG_INVALID_AST_INPUT;
    }

    double value = wcstod(string_data, NULL);

    node_data->type         = TYPE_NUM;
    node_data->value.number = value;

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t GetNodeDataId(LangCtx_t* lang_ctx, TokenData_t* node_data, wchar_t* string_data)
{
    assert(string_data != NULL);
    assert(node_data   != NULL);

    size_t name_index = 0;

    LangErr_t error = LANG_SUCCESS;

    if ((error = LangNamesPoolPush(&lang_ctx->names_pool, string_data, &name_index)))
        return error;

    node_data->value.id = name_index;

    return LANG_SUCCESS;
}

//==========================================================================================

static void SkipSpaces(wchar_t* buffer, ssize_t* pos)
{
    assert(buffer != NULL);
    assert(pos    != NULL);

    wchar_t ch = '\0';

    while ((ch = buffer[*pos]) != '\0' && iswspace(ch))
    {
        (*pos)++;
    }
}

//==========================================================================================

static int SkipLetter(wchar_t* buffer, ssize_t* pos, wchar_t letter)
{
    assert(buffer != NULL);
    assert(pos    != NULL);

    wchar_t current_char = buffer[*pos];

    if (current_char != letter)
    {
        WPRINTERR(L"Syntax error: expected %lc, got %lc (%d)", letter, current_char, current_char);
        return 1;
    }
    (*pos)++;

    return 0;
}

//==========================================================================================
