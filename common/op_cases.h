#ifndef OP_CASES_H
#define OP_CASES_H

//——————————————————————————————————————————————————————————————————————————————————————————

#include "operators.h"
#include "lang_ctx.h"

//——————————————————————————————————————————————————————————————————————————————————————————

typedef struct OperatorCase
{
    Operator_t     code;

    const char*    code_str;

    int            repeat_times;

    const wchar_t* name;
    size_t         name_len;

    const wchar_t* ast_format;

#ifdef BACKEND
    LangErr_t (*asm_function) (LangCtx_t*, TreeNode_t*);

    const wchar_t* asm_name;

#endif /* BACKEND */

#ifdef REVERSE
    LangErr_t (*src_function) (LangCtx_t*, TreeNode_t*);
#endif /* REVERSE */

} OperatorCase_t;

//——————————————————————————————————————————————————————————————————————————————————————————

#ifdef BACKEND

LangErr_t AssembleNode           (LangCtx_t* lang_ctx, TreeNode_t* node);

LangErr_t AssembleIf             (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleAssignment     (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleMathOperation  (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleUnaryOperation (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleInput          (LangCtx_t* lang_ctx, TreeNode_t* node);
// LangErr_t AssembleElse         (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleWhile          (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleCmdSeparator   (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleParamsSeparator(LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleHlt            (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssembleReturn         (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t AssemblePoint          (LangCtx_t* lang_ctx, TreeNode_t* node);

#endif /* BACKEND */

//——————————————————————————————————————————————————————————————————————————————————————————

#ifdef REVERSE

LangErr_t SrcNode(LangCtx_t* lang_ctx, TreeNode_t* node);

LangErr_t SrcCmdSeparator   (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcAssignment     (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcWhile          (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcIf             (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcAbort          (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcMathOperation  (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcUnaryOperator  (LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcCompareOperator(LangCtx_t* lang_ctx, TreeNode_t* node);
LangErr_t SrcParamsSeparator(LangCtx_t* lang_ctx, TreeNode_t* node);

#endif /* REVERSE */

//——————————————————————————————————————————————————————————————————————————————————————————

#ifdef BACKEND

#define SET_OP_CASE_(code,          repeat_times,   name,                                     ast_format,                                           asm_function,   asm_name,  src_function) \
        [(code)] = {(code), #code, (repeat_times), (name), sizeof(name) / sizeof(*name) - 1, (ast_format), (LangErr_t (*)(LangCtx_t*, TreeNode_t*))(asm_function), (asm_name)              }

#endif /* BACKEND */

#ifdef REVERSE

#define SET_OP_CASE_(code,          repeat_times,   name,                                     ast_format,   asm_function,  asm_name,                 src_function) \
        [(code)] = {(code), #code, (repeat_times), (name), sizeof(name) / sizeof(*name) - 1, (ast_format), (LangErr_t (*)(LangCtx_t*, TreeNode_t*))(src_function)}

#endif /* REVERSE */

#ifdef FRONTEND

#define SET_OP_CASE_(code,          repeat_times,   name,                                     ast_format,   asm_function,  asm_name, src_function) \
        [(code)] = {(code), #code, (repeat_times), (name), sizeof(name) / sizeof(*name) - 1, (ast_format)                                         }

#endif /* FRONTEND */


//==========================================================================================
const OperatorCase_t OP_CASES_TABLE[] =
{
//                      code,         repeat_times,         name                  ast_format                asm_function        asm_name    src_function
    SET_OP_CASE_(OP_NONE                ,   1,   L""                            , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_CMD_SEPARATOR       ,   1,   L"ЗАМОЛЧИ"                     , L"_END_STATEMENT_" , AssembleCmdSeparator   , L""     , SrcCmdSeparator   ),
    SET_OP_CASE_(OP_BRACKET_OPEN        ,   1,   L"("                           , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_BRACKET_CLOSE       ,   1,   L")"                           , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_ASSIGNMENT          ,   1,   L"пересади в"                  , L"_ASSIGNMENT_"    , AssembleAssignment     , L""     , SrcAssignment     ),
    SET_OP_CASE_(OP_IF_LHS              ,   1,   L"ты думаешь"                  , L"_IF_"            , AssembleIf             , L""     , SrcIf             ), // алгоритмы маркова
    SET_OP_CASE_(OP_IF_RHS              ,   1,   L"сможет что-то изменить?"     , L""                , NULL                   , L""     , NULL              ), // некоторые токены включают проверку
    SET_OP_CASE_(OP_ELSE                ,   1,   L"не верь им"                  , L"_ELSE_"          , NULL                   , L""     , NULL              ), // никогда не / запрещаю / нисколько -> не
    SET_OP_CASE_(OP_WHILE               ,   5,   L"снова и снова"               , L"_WHILE_"         , AssembleWhile          , L""     , SrcWhile          ),
    SET_OP_CASE_(OP_BLOCK_BEGIN         ,   5,   L"ЗАТКНИСЬ"                    , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_BLOCK_END           ,   4,   L"ОНИ СМОТРЯТ"                 , L""                , NULL                   , L""     , NULL              ), // сжимать все пробелы до 1 либо совмещать 2 токена в 1 на этапе лексики
    SET_OP_CASE_(OP_FUNCTION_BLOCK_BEGIN,   3,   L"за что?"                     , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_FUNCTION_BLOCK_END  ,   1,   L"нет нет нет нет все кончено?", L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_FUNCTION_DECL_LHS   ,   1,   L"за"                          , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_FUNCTION_DECL_RHS   ,   1,   L"отомсти"                     , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_FUNCTION_CALL_LHS   ,   1,   L"ритуал: во имя"              , L""                , NULL                   , L"CALL" , NULL              ),
    SET_OP_CASE_(OP_FUNCTION_CALL_RHS   ,   1,   L"принеси в жертву"            , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_PARAMS_SEPARATOR    ,   1,   L"и"                           , L"_COMMA_"         , AssembleParamsSeparator, L""     , SrcParamsSeparator),
    SET_OP_CASE_(OP_RETURN              ,   1,   L"оставь"                      , L"_RETURN_"        , AssembleReturn         , L"RET"  , SrcUnaryOperator  ),
    SET_OP_CASE_(OP_ADD                 ,   1,   L"нарастить на"                , L"_MATH_ADD_"      , AssembleMathOperation  , L"ADD"  , SrcMathOperation  ),
    SET_OP_CASE_(OP_SUB                 ,   1,   L"избавить от"                 , L"_MATH_SUB_"      , AssembleMathOperation  , L"SUB"  , SrcMathOperation  ),
    SET_OP_CASE_(OP_MUL                 ,   1,   L"усилить в"                   , L"_MATH_MUL_"      , AssembleMathOperation  , L"MUL"  , SrcMathOperation  ),
    SET_OP_CASE_(OP_DIV                 ,   1,   L"расщепить на"                , L"_MATH_DIV_"      , AssembleMathOperation  , L"DIV"  , SrcMathOperation  ),
    SET_OP_CASE_(OP_POW                 ,   1,   L"расплодить в"                , L"_MATH_POW_"      , AssembleMathOperation  , L"POW"  , SrcMathOperation  ),
    SET_OP_CASE_(OP_OUTPUT              ,   1,   L"заставь их услышать"         , L"_OUTPUT_"        , AssembleUnaryOperation , L"OUT"  , SrcUnaryOperator  ),
    SET_OP_CASE_(OP_INPUT               ,   1,   L"скажи мне кто ты,"           , L"_INPUT_"         , AssembleInput          , L"IN"   , SrcUnaryOperator  ),
    SET_OP_CASE_(OP_ABORT               ,   1,   L"аборт"                       , L"_ABORT_"         , AssembleHlt            , L"HLT"  , SrcAbort          ),
    SET_OP_CASE_(OP_VARIABLE_DECL       ,   1,   L"голос в голове"              , L""                , NULL                   , L""     , NULL              ),
    SET_OP_CASE_(OP_EQUAL               ,   1,   L"такой же как"                , L"_LOG_EQUAL_"     , NULL                   , L"JNE"  , SrcCompareOperator),
    SET_OP_CASE_(OP_NOT_EQUAL           ,   1,   L"противоположен"              , L"_LOG_NOT_EQUAL_" , NULL                   , L"JE"   , SrcCompareOperator),
    SET_OP_CASE_(OP_BIGGER_EQUAL        ,   1,   L"больше-равен"                , L"_LOG_MORE_EQUAL_", NULL                   , L"JB"   , SrcCompareOperator),
    SET_OP_CASE_(OP_BIGGER              ,   1,   L"больше"                      , L"_LOG_MORE_"      , NULL                   , L"JBE"  , SrcCompareOperator),
    SET_OP_CASE_(OP_SMALLER_EQUAL       ,   1,   L"меньше-равен"                , L"_LOG_LESS_EQUAL_", NULL                   , L"JA"   , SrcCompareOperator),
    SET_OP_CASE_(OP_SMALLER             ,   1,   L"меньше"                      , L"_LOG_LESS_"      , NULL                   , L"JAE"  , SrcCompareOperator),
    SET_OP_CASE_(OP_SQRT                ,   1,   L"исток"                       , L"_MATH_SQRT_"     , AssembleUnaryOperation , L"SQRT" , SrcUnaryOperator  ),
    SET_OP_CASE_(OP_DRAW                ,   1,   L"рисуй"                       , L"_DRAW_"          , AssembleUnaryOperation , L"DRAW" , SrcUnaryOperator  ),
    SET_OP_CASE_(OP_POINT               ,   1,   L"поставь точку"               , L"_POINT_"         , AssemblePoint          , L"POINT", SrcUnaryOperator  )
};

//==========================================================================================

#undef SET_OP_CASE_

//——————————————————————————————————————————————————————————————————————————————————————————

const size_t OPERATORS_COUNT       = sizeof(OP_CASES_TABLE) / sizeof(OP_CASES_TABLE[0]);
const size_t MAX_OPERATOR_NAME_LEN = 128;

//——————————————————————————————————————————————————————————————————————————————————————————

#endif /* OP_CASES_H */
