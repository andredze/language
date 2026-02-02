#ifndef LANG_STRUCT_H
#define LANG_STRUCT_H

//——————————————————————————————————————————————————————————————————————————————————————————

#include "tree_types.h"
#include "stack.h"
#include <stdio.h>
#include <wchar.h>

//——————————————————————————————————————————————————————————————————————————————————————————

#define LANG_NUM_SPEC L"%lg"

typedef double Number_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef enum IdType
{
    ID_TYPE_UNKNOWN  = 0,
    ID_TYPE_VARIABLE = 1,
    ID_TYPE_FUNCTION = 2
} IdType_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef struct IdData
{
    size_t    name_index;

    wchar_t*  name;

    IdType_t  type;

    size_t    memory_needed;
    size_t    n_params;

    int       addr;

} IdData_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef struct NamesPool
{
    wchar_t** data;

    size_t    size;
    size_t    capacity;

} NamesPool_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef struct IdTable
{
    IdData_t* data;

    size_t    size;
    size_t    capacity;

} IdTable_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef enum LangErr
{
    LANG_SUCCESS = 0,

    LANG_SYNTAX_ERROR,
    LANG_MEMALLOC_ERROR,
    LANG_FILE_ERROR,
    LANG_STACK_ERROR,
    LANG_TREE_ERROR,

    LANG_INVALID_INPUT,
    LANG_INVALID_AST_INPUT,
    LANG_BACKEND_AST_SYNTAX_ERROR,
    LANG_UNKNOWN_TOKEN_TYPE,
    LANG_UNASSEMBLE_OPERATOR,
    LANG_REVERSEBLE_OPERATOR,

    LANG_PARSER_SYNTAX_ERROR,
    LANG_LEXER_SYNTAX_ERROR,

    LAND_ID_TABLE_WRONG_INDEX,

    LANG_VAR_REDECLARATION,
    LANG_VAR_NOT_DECLARED,
    LANG_FUNC_DECL_IN_FUNC,
    LANG_FUNC_REDECLARATION,
    LANG_FUNC_NOT_DECLARED,
    LANG_FUNC_USED_AS_VAR,
    LANG_WRONG_ARGS_COUNT,
    LANG_WRONG_AST_FORMAT

} LangErr_t;

//——————————————————————————————————————————————————————————————————————————————————————————

const size_t MAX_MESSAGE_LEN = 128;

//------------------------------------------------------------------------------------------

typedef struct
{
    enum LangErr   error;

    TreeNode_t*    node;

    const char*    func;
    const char*    file;
    int            line;

    wchar_t        message[MAX_MESSAGE_LEN];

} LangErrorInfo_t;

//——————————————————————————————————————————————————————————————————————————————————————————

typedef struct LangCtx
{
    char          ast_file_name[MAX_FILENAME_LEN];

    LangErrorInfo_t error_info;

//TODO: код ошибки вся инфа об ошибке передавать до main
    wchar_t*      cur_symbol_ptr; // cur
    wchar_t*      buffer;
    size_t        buffer_size;
    size_t        current_line;

    Stack_t       tokens;
    Tree_t        tree;

    NamesPool_t   names_pool;

    TreeDebugData debug;

    size_t        cur_token_index; // for parser rename

    FILE*         output_file;

    bool          is_in_func;
    int           in_func_vars_count;

    IdTable_t     main_id_table;
    IdTable_t     func_id_table;

#ifdef BACKEND
    size_t        endif_labels_count;
    size_t        while_labels_count;

    bool          is_in_function;

    int           cur_addr;

    size_t        params_count;

    bool          assembling_args;

    bool          getting_function_params;

    size_t        global_vars_count;

    bool          first_point;

#endif /* BACKEND */

#ifdef REVERSE

    size_t        tabs;

#endif /* REVERSE */

} LangCtx_t;

//——————————————————————————————————————————————————————————————————————————————————————————

#endif /* LANG_STRUCT_H */
