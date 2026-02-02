#ifndef LANG_FUNCS_H
#define LANG_FUNCS_H

//——————————————————————————————————————————————————————————————————————————————————————————

#include "lang_ctx.h"
#include "tree_commands.h"
#include "stack.h"
#include <wchar.h>

//——————————————————————————————————————————————————————————————————————————————————————————

#define LANG_SET_ERROR_(lang_ctx, error, node, message, ...)                      \
        BEGIN                                                                     \
            LangErrorInfo_t info_ = {error, node, __func__, __FILE__, __LINE__};  \
            LangSetError(lang_ctx, &info_, message, ##__VA_ARGS__);               \
        END

//------------------------------------------------------------------------------------------

const wchar_t* GetOpName(Operator_t opcode);

//------------------------------------------------------------------------------------------

void LangSetError(LangCtx_t*       lang_ctx,
                  LangErrorInfo_t* error_info,
                  const wchar_t*   message,
                  ...);

void LangPrintNode       (LangCtx_t* lang_ctx, TreeNode_t* node);
void LangPrintError      (LangCtx_t* lang_ctx);
void LangPrintSyntaxError(LangCtx_t* lang_ctx);

//------------------------------------------------------------------------------------------

LangErr_t   LangCtxCtor           (LangCtx_t* lang_ctx);
void        LangCtxDtor           (LangCtx_t* lang_ctx);

LangErr_t   LangOpenAsmFile       (LangCtx_t* lang_ctx);

LangErr_t   LangOpenReverseFile   (LangCtx_t* lang_ctx);

//==========================================================================================

LangErr_t LangNamesPoolCtor       (NamesPool_t* names_pool);
void      LangNamesPoolDtor       (NamesPool_t* names_pool);
LangErr_t LangNamesPoolPush       (NamesPool_t* names_pool, const wchar_t* name_buf, size_t* name_index);
wchar_t*  LangGetIdName           (NamesPool_t* names_pool, Identifier_t index);

//==========================================================================================

LangErr_t   LangIdTableCtor       (IdTable_t* id_table);
void        LangIdTableDtor       (IdTable_t* id_table);
LangErr_t   LangIdTablePush       (IdTable_t* id_table, IdData_t* id_data);
void        LangIdTableDump       (IdTable_t* id_table);

LangErr_t   LangGetIdData                 (IdTable_t* id_table, size_t index, IdData_t* id_data);
LangErr_t   LangFuncCallRightArgs         (LangCtx_t* lang_ctx, size_t func_id_index, int args_count);
LangErr_t   LangSafePushIdTable           (LangCtx_t* lang_ctx, IdTable_t* id_table, IdData_t* id_data);
size_t      LangIdTableCountVars          (IdTable_t* id_table);

LangErr_t   LangGetFuncIndex              (LangCtx_t* lang_ctx, Identifier_t id, size_t* func_id_index);
bool        LangFuncWasDeclared           (LangCtx_t* lang_ctx, Identifier_t id);
LangErr_t   LangCheckVariableIsNotFunction(IdTable_t* id_table, Identifier_t id);
LangErr_t   LangIdTableGetAddress         (IdTable_t* id_table, Identifier_t id, int* addr);
bool        LangGetIdInTable              (IdTable_t* id_table, Identifier_t id, size_t* id_index);
bool        LangIdInTable                 (IdTable_t* id_table, Identifier_t id);

//==========================================================================================

TreeNode_t* LangGetCurrentToken   (LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

const size_t DEFAULT_ID_TABLE_CAPACITY   = 64;
const size_t DEFAULT_NAMES_POOL_CAPACITY = 64;
const size_t MAX_BUFFER_SIZE             = 256;

//——————————————————————————————————————————————————————————————————————————————————————————

#endif /* LANG_FUNCS_H */
