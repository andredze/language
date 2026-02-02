#include "lang_funcs.h"
#include "op_cases.h"

//==========================================================================================

//FIXME - stack realloc дропается дамп, скорее всего реаллок не работает

//==========================================================================================

void LangSetError(LangCtx_t*       lang_ctx,
                  LangErrorInfo_t* error_info,
                  const wchar_t*   message,
                  ...)
{
    assert(lang_ctx);

    va_list message_args = {};

    va_start(message_args, message);

    wchar_t buffer[MAX_BUFFER_SIZE] = {};

    vswprintf(buffer, sizeof(buffer), message, message_args);

    lang_ctx->error_info = *error_info;

    wcsncpy(lang_ctx->error_info.message, buffer,
            sizeof(lang_ctx->error_info.message));

    va_end(message_args);
}

//==========================================================================================

void LangPrintError(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    wprintf(L"%s", RED);

    LangErrorInfo_t error_info = lang_ctx->error_info;

    LangErr_t error = error_info.error;

    wprintf(L"---------------------------------------------"
            L"---------------------------------------------\n\n"
            L"ERROR: %d in function %s at %s:%d\n\n",
            error, error_info.func, error_info.file, error_info.line);

    if (error == LANG_PARSER_SYNTAX_ERROR ||
        error == LANG_LEXER_SYNTAX_ERROR)
    {
        LangPrintSyntaxError(lang_ctx);
    }

    wprintf(L"---------------------------------------------"
            L"---------------------------------------------\n");

    wprintf(L"%s", RESET_COLOR);
}

//==========================================================================================

static void LangPrintErrorCodePart(LangCtx_t* lang_ctx, TreeNode_t* node);

//------------------------------------------------------------------------------------------

void LangPrintSyntaxError(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    LangErrorInfo_t error_info = lang_ctx->error_info;

    if (error_info.error == LANG_PARSER_SYNTAX_ERROR)
        wprintf(L"PARSER ERROR: \n\n");

    if (error_info.error == LANG_LEXER_SYNTAX_ERROR)
        wprintf(L"LEXER ERROR: \n\n");

    wprintf(L"expected:\n"
            L"\t%ls\n", error_info.message);

    wprintf(L"got:\n");

    LangPrintNode(lang_ctx, error_info.node);

    wprintf(L"\n");
    wprintf(L"syntax error\n\n");

    LangPrintErrorCodePart(lang_ctx, error_info.node);
}

//==========================================================================================

static wchar_t* FindLineStart(wchar_t* buffer_start, wchar_t* buffer_end, size_t line);

//------------------------------------------------------------------------------------------

static void LangPrintErrorCodePart(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    if (lang_ctx->buffer == NULL)
    {
        wprintf(L"\terror: can not print: no buffer given\n");
        return;
    }

    wchar_t* buffer_start = lang_ctx->buffer;
    wchar_t* buffer_end   = buffer_start + lang_ctx->buffer_size;

    if (node->buf_pos == NULL || node->buf_pos > buffer_end)
    {
        wprintf(L"error: node->cur_pos was not set\n");
        return;
    }

    wchar_t* cur_pos = FindLineStart(buffer_start, buffer_end, node->line);

    if (cur_pos == NULL)
        return;

    wprintf(L"%s", RESET_COLOR);

    wprintf(L"%-5d | ");

    while (cur_pos < node->buf_pos)
    {
        putwc(*cur_pos, stdout);
        cur_pos++;
    }

    wchar_t* line_end = wcschr(node->buf_pos, L'\n');

    if (line_end != NULL)
        *line_end = L'\0';

    wprintf(L"%s", RED);

    wprintf(L"%-5d | %ls"
            L"      |\n"
            L"      |\n",
            node->line,
            node->buf_pos);
}

//==========================================================================================

static wchar_t* FindLineStart(wchar_t* buffer_start, wchar_t* buffer_end, size_t line)
{
    assert(buffer_start);
    assert(buffer_end);

    wchar_t* cur_pos = buffer_start;

    size_t enters_count = 0;

    while (enters_count + 1 < line)
    {
        cur_pos = wcschr(cur_pos, L'\n');

        if (cur_pos == NULL || cur_pos >= buffer_end)
        {
            wprintf(L"\terror: cur_line is outside of buffer\n");
            return NULL;
        }

        enters_count++;
    }

    return cur_pos;
}

//==========================================================================================

void LangPrintNode(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);

    if (node == NULL)
    {
        wprintf(L"node = NULL\n");
    }

    wprintf(
LR"(    node [%p]:
        left  [%p]
        right [%p]
        type = %ls;
        value = )",
        node,
        node->left,
        node->right,
        TYPE_CASES_TABLE[node->data.type].name);

    switch (node->data.type)
    {
        case TYPE_NUM:
            wprintf(LANG_NUM_SPEC, node->data.value.number);
            break;

        case TYPE_OP:
            wprintf(L"%ls", OP_CASES_TABLE[node->data.value.opcode].name);
            break;

        case TYPE_ID:
        case TYPE_VAR:
        case TYPE_VAR_DECL:
        case TYPE_FUNC_DECL:
        case TYPE_FUNC_CALL:
            wprintf(L"%ls", lang_ctx->names_pool.data[node->data.value.id]);
            break;

        default:
            wprintf(L"UNKNOWN");
    }

    wprintf(L";\n");
}

//==========================================================================================

const wchar_t* GetOpName(Operator_t opcode)
{
    if (!(0 <= opcode && opcode < OPERATORS_COUNT))
        return NULL;

    return OP_CASES_TABLE[opcode].name;
}

//==========================================================================================

LangErr_t LangCtxCtor(LangCtx_t* lang_ctx)
{
    assert(lang_ctx != NULL);

    LangErr_t error = LANG_SUCCESS;

#ifdef FRONTEND
    if (StackCtor(&lang_ctx->tokens, 256))
    {
        WPRINTERR(L"Tokens stack construct failed");
        return LANG_STACK_ERROR;
    }

    lang_ctx->cur_symbol_ptr = NULL;
    lang_ctx->current_line   = 1;

#endif /* FRONTEND */

    if ((error = LangIdTableCtor(&lang_ctx->main_id_table)))
        return error;

    if ((error = LangNamesPoolCtor(&lang_ctx->names_pool)))
        return error;

    if (TreeCtor(&lang_ctx->tree))
    {
        WPRINTERR(L"Language tree construct failed");
        return LANG_TREE_ERROR;
    }

    WDPRINTF(L"tree logfile opening\n");

    if (TreeOpenLogFile(lang_ctx))
        return LANG_TREE_ERROR;

    WDPRINTF(L"tree_ptr = %p\n", lang_ctx->tree);

    return LANG_SUCCESS;
}

//==========================================================================================

void LangCtxDtor(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

#ifdef FRONTEND

    TreeSingleNodeDtor(lang_ctx->tree.dummy, &lang_ctx->tree);

    for (size_t i = 0; i < lang_ctx->tokens.size; i++)
    {
        WDPRINTF(L"freed token = %p\n", lang_ctx->tokens.data[i]);
        free(lang_ctx->tokens.data[i]);
    }
    StackDtor(&lang_ctx->tokens);

#endif /* FRONTEND */

    LangIdTableDtor(&lang_ctx->main_id_table);

#ifdef BACKEND
    TreeDtor(&lang_ctx->tree);
#endif /* BACKEND */

#ifdef REVERSE
    TreeDtor(&lang_ctx->tree);
#endif /* REVERSE */

    if (lang_ctx->output_file != NULL)
        fclose(lang_ctx->output_file);

    LangNamesPoolDtor(&lang_ctx->names_pool);

    free(lang_ctx->buffer);

    lang_ctx->cur_symbol_ptr = NULL;
    lang_ctx->buffer         = NULL;

    TreeCloseLogFile(lang_ctx);
}

//==========================================================================================

LangErr_t LangOpenAsmFile(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    char asm_file_path[MAX_FILE_NAME_LEN];

    snprintf(asm_file_path, sizeof(asm_file_path), "asm/%s.asm", lang_ctx->ast_file_name);

    WDPRINTF(L"Asm file name: %s\n", lang_ctx->ast_file_name);
    WDPRINTF(L"Opening file %s\n\n",   asm_file_path);

    FILE* asm_fp = fopen(asm_file_path, "w");

    if (asm_fp == NULL)
    {
        WPRINTERR(L"Failed opening file %s", asm_file_path);
        return LANG_FILE_ERROR;
    }

#ifdef BACKEND
    lang_ctx->output_file = asm_fp;
#endif /* BACKEND */

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t LangOpenReverseFile(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    char src_file_path[MAX_FILE_NAME_LEN];

    snprintf(src_file_path, sizeof(src_file_path), "reversed/%s.psy", lang_ctx->ast_file_name);

    WDPRINTF(L"Reversed code file name: %s\n", lang_ctx->ast_file_name);
    WDPRINTF(L"Opening file %s\n\n", src_file_path);

    FILE* src_fp = fopen(src_file_path, "w");

    if (src_fp == NULL)
    {
        WPRINTERR(L"Failed opening file %s", src_file_path);
        return LANG_FILE_ERROR;
    }

    lang_ctx->output_file = src_fp;

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t LangNamesPoolCtor(NamesPool_t* names_pool)
{
    assert(names_pool);

    size_t capacity = DEFAULT_NAMES_POOL_CAPACITY;

    names_pool->data = (wchar_t**) calloc(capacity, sizeof(wchar_t*));

    if (names_pool->data == NULL)
    {
        WPRINTERR(L"Memory allocation failed");
        return LANG_MEMALLOC_ERROR;
    }

    names_pool->capacity = capacity;
    names_pool->size     = 0;

    return LANG_SUCCESS;
}

//==========================================================================================

void LangNamesPoolDtor(NamesPool_t* names_pool)
{
    assert(names_pool);

    for (size_t i = 0; i < names_pool->size; i++)
    {
        free(names_pool->data[i]);
        names_pool->data[i] = NULL;
    }

    names_pool->size     = 0;
    names_pool->capacity = 0;

    free(names_pool->data);
    names_pool->data = NULL;

    WDPRINTF(L"------- NamesPool destroyed -------\n");
}

//==========================================================================================

static LangErr_t LangNamesPoolRealloc(NamesPool_t* names_pool);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t LangNamesPoolPush(NamesPool_t* names_pool, const wchar_t* name_buf, size_t* name_index)
{
    assert(name_index != NULL);
    assert(names_pool != NULL);
    assert(name_buf   != NULL);

    for (size_t i = 0; i < names_pool->size; i++)
    {
        if (wcscmp(names_pool->data[i], name_buf) == 0)
        {
            *name_index = i;
            return LANG_SUCCESS;
        }
    }

    wchar_t* name = wcsdup(name_buf);

    if (name == NULL)
    {
        PRINTERR("Memory allocation failed");
        return LANG_MEMALLOC_ERROR;
    }

    LangErr_t error = LANG_SUCCESS;

    if (names_pool->size >= names_pool->capacity)
    {
        if ((error = LangNamesPoolRealloc(names_pool)))
            return error;
    }

    *name_index = names_pool->size;
    names_pool->data[names_pool->size++] = name;

    return LANG_SUCCESS;
}

//==========================================================================================

wchar_t* LangGetIdName(NamesPool_t* names_pool, Identifier_t index)
{
    assert(names_pool);

    if (!(index <= names_pool->size))
    {
        return NULL;
    }

    return names_pool->data[index];
}

//==========================================================================================

static LangErr_t LangNamesPoolRealloc(NamesPool_t* names_pool)
{
    assert(names_pool);

    size_t new_capacity = names_pool->capacity * 2 + 1;

    wchar_t** new_data = (wchar_t**) realloc(names_pool->data, new_capacity * sizeof(*names_pool->data));

    if (new_data == NULL)
    {
        WPRINTERR(L"Memory reallocation failed");
        return LANG_MEMALLOC_ERROR;
    }

    names_pool->data     = new_data;
    names_pool->capacity = new_capacity;

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t LangIdTableCtor(IdTable_t* id_table)
{
    assert(id_table);

    size_t capacity = DEFAULT_ID_TABLE_CAPACITY;

    id_table->data = (IdData_t*) calloc(capacity, sizeof(IdData_t));

    if (id_table->data == NULL)
    {
        WPRINTERR(L"Memory allocation failed");
        return LANG_MEMALLOC_ERROR;
    }

    id_table->capacity = capacity;
    id_table->size     = 0;

    return LANG_SUCCESS;
}

//==========================================================================================

void LangIdTableDump(IdTable_t* id_table)
{
    wprintf(L"\n---------dumping id_table %p-----------\n", id_table);
    wprintf(L".size = %zu\n",  id_table->size);
    wprintf(L".cap  = %zu\n",  id_table->capacity);
    wprintf(L".data = %p\n", id_table->data);

    IdData_t* id_data = NULL;

    for (size_t i = 0; i < id_table->size; i++)
    {
        id_data = &id_table->data[i];

        if (id_data->name == NULL)
        {
            continue;
        }

        wprintf(L"{%zu, %ls, %d, %zu, %zu, addr = %d}\n",
                id_data->name_index,
                id_data->name,
                id_data->type,
                id_data->memory_needed,
                id_data->n_params,
                id_data->addr);
    }

    wprintf(L"---------------------------------------\n");
}

//==========================================================================================

void LangIdTableDtor(IdTable_t* id_table)
{
    assert(id_table);

    for (size_t i = 0; i < id_table->size; i++)
    {
        id_table->data[i].name_index = (size_t)-1;
        id_table->data[i].type       = ID_TYPE_UNKNOWN;
    }

    id_table->size     = 0;
    id_table->capacity = 0;

    free(id_table->data);
    id_table->data = NULL;

    WDPRINTF(L"----- IdTable destroyed -----\n");
}

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t LangIdTableRealloc(IdTable_t* id_table);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t LangIdTablePush(IdTable_t* id_table, IdData_t* id_data)
{
    assert(id_table != NULL);
    assert(id_data  != NULL);

    LangErr_t error = LANG_SUCCESS;

    if (id_table->size >= id_table->capacity)
    {
        if ((error = LangIdTableRealloc(id_table)))
            return error;
    }

    id_table->data[id_table->size] = *id_data;
    id_table->size++;

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t LangSafePushIdTable(LangCtx_t* lang_ctx, IdTable_t* id_table, IdData_t* id_data)
{
    if (LangIdInTable(id_table, id_data->name_index))
    {
        WPRINTERR(L"Variable %ls was already declared\n",
                    lang_ctx->names_pool.data[id_data->name_index]);
        return LANG_VAR_REDECLARATION;
    }

    LangErr_t error = LANG_SUCCESS;

    if ((error = LangIdTablePush(id_table, id_data)))
    {
        return error;
    }

    return LANG_SUCCESS;
}

//==========================================================================================

bool LangGetIdInTable(IdTable_t* id_table, Identifier_t id, size_t* id_index)
{
    assert(id_table);

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].name_index == id)
        {
            *id_index = i;
            return true;
        }
    }

    return false;
}

//==========================================================================================

LangErr_t LangGetFuncIndex(LangCtx_t* lang_ctx, Identifier_t id, size_t* func_id_index)
{
    assert(lang_ctx);

    size_t id_index = 0;

    if (LangGetIdInTable(&lang_ctx->main_id_table, id, &id_index) == true)
    {
        if (lang_ctx->main_id_table.data[id_index].type == ID_TYPE_FUNCTION)
        {
            *func_id_index = id_index;
            return LANG_SUCCESS;
        }
    }

    return LANG_FUNC_NOT_DECLARED;
}

//==========================================================================================

LangErr_t LangGetIdData(IdTable_t* id_table, size_t index, IdData_t* id_data)
{
    assert(id_table != NULL);

    if (id_table->size <= index)
    {
        return LAND_ID_TABLE_WRONG_INDEX;
    }

    *id_data = id_table->data[index];

    return LANG_SUCCESS;
}

//==========================================================================================

size_t LangIdTableCountVars(IdTable_t* id_table)
{
    assert(id_table);

    size_t global_vars_count = 0;

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].type == ID_TYPE_VARIABLE)
        {
            global_vars_count++;
        }
    }

    return global_vars_count;
}

//==========================================================================================

LangErr_t LangFuncCallRightArgs(LangCtx_t* lang_ctx, size_t func_id_index, int args_count)
{
    assert(lang_ctx);

    IdData_t id_data = {};
    LangErr_t error = LANG_SUCCESS;

    if ((error = LangGetIdData(&lang_ctx->main_id_table, func_id_index, &id_data)))
    {
        return error;
    }

    if (args_count != id_data.n_params)
    {
        WPRINTERR(L"WRONG AMOUNT OF ARGS for function %ls\n",
                  lang_ctx->names_pool.data[id_data.name_index]);
        return LANG_WRONG_ARGS_COUNT;
    }

    return LANG_SUCCESS;
}

//==========================================================================================

bool LangFuncWasDeclared(LangCtx_t* lang_ctx, Identifier_t id)
{
    assert(lang_ctx);

    size_t id_index = 0;

    if (LangGetIdInTable(&lang_ctx->main_id_table, id, &id_index) == true)
    {
        if (lang_ctx->main_id_table.data[id_index].type == ID_TYPE_FUNCTION)
        {
            return true;
        }
    }

    return false;
}

//==========================================================================================

bool LangIdInTable(IdTable_t* id_table, Identifier_t id)
{
    assert(id_table);

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].name_index == id)
            return true;
    }

    return false;
}

//==========================================================================================

LangErr_t LangCheckVariableIsNotFunction(IdTable_t* id_table, Identifier_t id)
{
    assert(id_table);

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].name_index == id && id_table->data[i].type != ID_TYPE_VARIABLE)
        {
            return LANG_FUNC_USED_AS_VAR;
        }
    }

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t LangIdTableGetAddress(IdTable_t* id_table, Identifier_t id, int* addr)
{
    assert(id_table);
    assert(addr);

    for (size_t i = 0; i < id_table->size; i++)
    {
        if (id_table->data[i].name_index == id)
        {
            *addr = id_table->data[i].addr;
            return LANG_SUCCESS;
        }
    }

    return LANG_VAR_NOT_DECLARED;
}

//==========================================================================================

static LangErr_t LangIdTableRealloc(IdTable_t* id_table)
{
    assert(id_table);

    size_t new_capacity = id_table->capacity * 2 + 1;

    IdData_t* new_data = (IdData_t*) realloc(id_table->data, new_capacity * sizeof(*id_table->data));

    if (new_data == NULL)
    {
        WPRINTERR(L"Memory reallocation failed");
        return LANG_MEMALLOC_ERROR;
    }

    id_table->data     = new_data;
    id_table->capacity = new_capacity;

    return LANG_SUCCESS;
}

//==========================================================================================

TreeNode_t* LangGetCurrentToken(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* token = NULL;

    if (StackGetElement(&lang_ctx->tokens, lang_ctx->cur_token_index, &token))
    {
        PRINTERR("Error with getting token from stack");
        return NULL;
    }

    return token;
}

//==========================================================================================
