#include "parser.h"

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_DEFINE_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————

#define PARSER_DUMP_(node, fmt, ...)                                              \
        BEGIN                                                                     \
        GRAPH_DUMP_(lang_ctx, (node), DUMP_SHORT, fmt, ##__VA_ARGS__);            \
        if (node)                                                                 \
            wfcprintf(stderr, PURPLE, L"dump %-30ls | %5p | %20s | type = %-3s |" \
                                      L"l = %-15p | r = %-15p      \n",           \
                   (fmt), (node), #node, TYPE_CASES_TABLE[(node)->data.type].name,\
                   (node)->left, (node)->right);                                  \
        END

//——————————————————————————————————————————————————————————————————————————————————————————

static TreeNode_t* ParseProgram             (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseBody                (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseCmdSeparator        (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseFunctionDeclaration (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFunctionParameters  (LangCtx_t*  lang_ctx, int* params_count_p);

static TreeNode_t* ParseStatement           (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFunctionStatement   (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFunctionBlock       (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseReturn              (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseIfStatement         (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseWhileStatement      (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseBlockStatement      (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseVariableDeclaration (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseAssignment          (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseExpression          (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseTerm                (LangCtx_t*  lang_ctx);
static TreeNode_t* ParsePower               (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFactor              (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseVariable            (LangCtx_t*  lang_ctx);

static TreeNode_t* ParseBracketsExpression  (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFunctionCall        (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseFunctionArguments   (LangCtx_t*  lang_ctx, int* args_count_p);

static TreeNode_t* ParseUnaryOperatorCall   (LangCtx_t*  lang_ctx);
static TreeNode_t* ParseUnaryOperator       (LangCtx_t*  lang_ctx);

static void        SetIdentifierTokenType   (LangCtx_t*  lang_ctx,
                                             TreeNode_t* cur_token,
                                             TokenType_t new_type);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t ParseTokens(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    wfcprintf(stderr, BLUE, L"Parsing tokens...\n");

    TreeNode_t* root = ParseProgram(lang_ctx);

    if (root == NULL)
        return LANG_SYNTAX_ERROR;

    lang_ctx->tree.dummy->right = root;

    TREE_CALL_DUMP(lang_ctx, "parser");

    wfcprintf(stderr, GREEN, L"Parsing success\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static TreeNode_t* ParseProgram(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = ParseBody(lang_ctx);

    PARSER_DUMP_(cur_token, L"program");

    if (cur_token == NULL)
        return NULL;

    if (StackSize(&lang_ctx->tokens) != lang_ctx->cur_token_index)
    {
        WPRINTERR("INVALID TOKEN, size = %zu; cur_tok_ind = %zu",
                   StackSize(&lang_ctx->tokens),
                   lang_ctx->cur_token_index);
        return NULL;
    }

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseBody(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t  dummy_root     = {};
    TreeNode_t* last_separator = &dummy_root;

    while (true)
    {
        TreeNode_t* statement = ParseStatement(lang_ctx);

        if (statement == NULL)
            break;

        TreeNode_t* separator = ParseCmdSeparator(lang_ctx);

        if (separator == NULL)
        {
            SET_PARSER_ERROR_(statement, L"Missing \"%ls\"", GetOpName(OP_CMD_SEPARATOR));
            // WPRINTERR("There should be a cmd separator after statement, cur_tok_ind = %zu",
            //             lang_ctx->cur_token_index);
            // PARSER_DUMP_(lang_ctx->tokens.data[lang_ctx->cur_token_index - 1],
            //                 L"expected to have a separator after");
            return NULL;
        }

        separator->left       = statement;
        last_separator->right = separator;
        last_separator        = separator;

        PARSER_DUMP_(separator, L"body: separator");
    }

    if (dummy_root.right == NULL)
        return NULL;

    PARSER_DUMP_(dummy_root.right, L"body la finale");

    return dummy_root.right;
}

//==========================================================================================

static TreeNode_t* ParseFunctionDeclaration(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* func_decl_lhs = LangGetCurrentToken(lang_ctx);

    if (func_decl_lhs == NULL || !IS_OPERATOR_(func_decl_lhs, OP_FUNCTION_DECL_LHS))
        return NULL;

    lang_ctx->cur_token_index++;

    lang_ctx->is_in_func = 1;

    PARSER_DUMP_(func_decl_lhs, L"function declaration lhs");

    TreeNode_t* function_name = LangGetCurrentToken(lang_ctx);

    if (function_name == NULL || !IS_IDENTIFIER_(function_name))
    {
        SET_PARSER_ERROR_(func_decl_lhs, L"Missing function name after \"%ls\"",
                                         GetOpName(OP_FUNCTION_DECL_LHS));

        WPRINTERR(L"Expected function name after function declaration lhs");

        return NULL;
    }

    SetIdentifierTokenType(lang_ctx, function_name, TYPE_FUNC_DECL);

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(function_name, L"function declaration: name %ls",
                 lang_ctx->names_pool.data[function_name->data.value.id]);

    TreeNode_t* func_decl_rhs = LangGetCurrentToken(lang_ctx);

    if (func_decl_rhs == NULL || !IS_OPERATOR_(func_decl_rhs, OP_FUNCTION_DECL_RHS))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(func_decl_rhs, L"function declaration rhs");

    int params_count = 0;

    if (LangIdTableCtor(&lang_ctx->func_id_table))
    {
        return NULL;
    }

    TreeNode_t* function_parameters = ParseFunctionParameters(lang_ctx, &params_count);
    //TODO - add check error

    lang_ctx->in_func_vars_count = 0;

    TreeNode_t* function_block = ParseFunctionBlock(lang_ctx);

    if (function_block == NULL)
    {
        SET_PARSER_ERROR_(func_decl_rhs, L"Missing function \"%ls\" body",
                          LangGetIdName(&lang_ctx->names_pool, function_name->data.value.id));

        WPRINTERR(L"Expected function block after function declaration");
        return NULL;
    }

    PARSER_DUMP_(function_block, L"function declaration: got function block");

    function_name->left  = function_parameters;
    function_name->right = function_block;

    PARSER_DUMP_(function_name, L"function declaration la finale");

    int memory_needed = lang_ctx->in_func_vars_count + params_count;
    WDPRINTF(L"params_count for func:  %ls = %d\n", lang_ctx->names_pool.data[function_name->data.value.id], params_count);
    WDPRINTF(L"memory_needed for func: %ls = %d\n", lang_ctx->names_pool.data[function_name->data.value.id], memory_needed);

    IdData_t func_id_data = {.name_index    = function_name->data.value.id,
                             .type          = ID_TYPE_FUNCTION,
                             .memory_needed = memory_needed,
                             .n_params      = params_count};

    if (LangIdTablePush(&lang_ctx->main_id_table, &func_id_data))
    {
    //TODO - redeclaration error message
        WPRINTERR(L"Function %ls redeclared\n",
                  lang_ctx->names_pool.data[func_id_data.name_index]);
        return NULL;
    }

    lang_ctx->in_func_vars_count = 0;
    lang_ctx->is_in_func = 0;

    LangIdTableDtor(&lang_ctx->func_id_table);

    return function_name;
}

//==========================================================================================

static TreeNode_t* ParseFunctionParameters(LangCtx_t* lang_ctx, int* params_count_p)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = LangGetCurrentToken(lang_ctx);

    if (cur_token == NULL || !IS_IDENTIFIER_(cur_token))
        return NULL;

    SetIdentifierTokenType(lang_ctx, cur_token, TYPE_VAR);

    (*params_count_p)++;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(cur_token, L"function first parameter");

    IdData_t id_data = {.name_index = cur_token->data.value.id,
                        .type = ID_TYPE_VARIABLE};

    if (LangSafePushIdTable(lang_ctx, &lang_ctx->func_id_table, &id_data))
    {
        return NULL;
    }

    TreeNode_t* params_separator = LangGetCurrentToken(lang_ctx);

    if (params_separator == NULL)
        return cur_token;

    TreeNode_t* next_param = NULL;

    while (IS_OPERATOR_(params_separator, OP_PARAMS_SEPARATOR))
    {
        (*params_count_p)++;

        lang_ctx->cur_token_index++;

        params_separator->left = cur_token;

        cur_token = params_separator;

        next_param = LangGetCurrentToken(lang_ctx);

        if (next_param == NULL || !IS_IDENTIFIER_(next_param))
        {
            WPRINTERR("Should be an parameter after param separator");
            return NULL;
        }

        SetIdentifierTokenType(lang_ctx, next_param, TYPE_VAR);

        lang_ctx->cur_token_index++;

        id_data = {.name_index = next_param->data.value.id,
                   .type = ID_TYPE_VARIABLE};

        if (LangSafePushIdTable(lang_ctx, &lang_ctx->func_id_table, &id_data))
        {
            return NULL;
        }

        cur_token->right = next_param;

        params_separator = LangGetCurrentToken(lang_ctx);

        if (params_separator == NULL)
            break;
    }

    PARSER_DUMP_(cur_token, L"function parameters all");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseFunctionBlock(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* block_begin = LangGetCurrentToken(lang_ctx);

    if (block_begin == NULL || !IS_OPERATOR_(block_begin, OP_FUNCTION_BLOCK_BEGIN))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(block_begin, L"function block: begin");

    TreeNode_t  dummy_root     = {};
    TreeNode_t* last_separator = &dummy_root;

    while (true)
    {
        TreeNode_t* statement = ParseFunctionStatement(lang_ctx);

        if (statement == NULL)
            break;

        TreeNode_t* separator = ParseCmdSeparator(lang_ctx);

        if (separator == NULL)
        {
            WPRINTERR("There should be a cmd separator after function statement, cur_tok_ind = %zu",
                        lang_ctx->cur_token_index);
            PARSER_DUMP_(lang_ctx->tokens.data[lang_ctx->cur_token_index - 1],
                            L"expected to have a separator after");
            return NULL;
        }

        separator->left       = statement;
        last_separator->right = separator;
        last_separator        = separator;

        PARSER_DUMP_(separator, L"function block: separator");
    }

    if (dummy_root.right == NULL)
        return NULL;

    PARSER_DUMP_(dummy_root.right, L"function block body");

    TreeNode_t* block_end = LangGetCurrentToken(lang_ctx);

    if (block_end == NULL || !IS_OPERATOR_(block_end, OP_FUNCTION_BLOCK_END))
    {
        WPRINTERR(L"Expected function block end");
        return NULL;
    }

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(block_end,        L"function block: end");
    PARSER_DUMP_(dummy_root.right, L"function block statement");

    return dummy_root.right;
}

//==========================================================================================

#define TRY_PARSING_(ParseFunc_, message)                               \
        BEGIN                                                           \
            cur_token = ParseFunc_(lang_ctx);                           \
                                                                        \
            if (cur_token != NULL)                                      \
            {                                                           \
                PARSER_DUMP_(cur_token, L"statement: got %ls" message); \
                return cur_token;                                       \
            }                                                           \
        END

static TreeNode_t* ParseStatement(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = NULL;

    TRY_PARSING_(ParseIfStatement,          L"statement");
    TRY_PARSING_(ParseWhileStatement,       L"statement");
    TRY_PARSING_(ParseVariableDeclaration,  L"declaration");
    TRY_PARSING_(ParseFunctionDeclaration,  L"declaration");
    TRY_PARSING_(ParseAssignment,           L"assignment");
    TRY_PARSING_(ParseExpression,           L"expression");

    return NULL;
}

#undef TRY_PARSING_

//==========================================================================================

static TreeNode_t* ParseFunctionStatement(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = NULL;

    cur_token = ParseReturn(lang_ctx);

    if (cur_token != NULL)
    {
        PARSER_DUMP_(cur_token, L"function statement: got return statement");
        return cur_token;
    }

    cur_token = ParseStatement(lang_ctx);

    if (cur_token != NULL)
    {
        PARSER_DUMP_(cur_token, L"function statement: got statement");
        return cur_token;
    }

    return NULL;
}

//==========================================================================================

static TreeNode_t* ParseReturn(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = LangGetCurrentToken(lang_ctx);

    if (cur_token == NULL || !IS_OPERATOR_(cur_token, OP_RETURN))
        return NULL;

    PARSER_DUMP_(cur_token, L"return operator");

    lang_ctx->cur_token_index++;

    cur_token->right = ParseExpression(lang_ctx);

    PARSER_DUMP_(cur_token, L"return");

    if (cur_token->right == NULL)
    {
        WPRINTERR("After return there should be an expression");
        return NULL;
    }

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseCondition(LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

static TreeNode_t* ParseIfStatement(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* if_lhs = LangGetCurrentToken(lang_ctx);

    if (if_lhs == NULL || !IS_OPERATOR_(if_lhs, OP_IF_LHS))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(if_lhs, L"if_lhs");

    TreeNode_t* condition = ParseCondition(lang_ctx);

    if (condition == NULL)
    {
        WPRINTERR("There should be a condition inside if");
        return NULL;
    }

    PARSER_DUMP_(condition, L"if_condition");

    TreeNode_t* if_rhs = LangGetCurrentToken(lang_ctx);

    if (if_rhs == NULL && !IS_OPERATOR_(if_rhs, OP_IF_RHS))
    {
        WPRINTERR("There should be an if right side");
        return NULL;
    }

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(if_rhs, L"if_rhs");

    TreeNode_t* block_statement = ParseBlockStatement(lang_ctx);

    if (block_statement == NULL)
    {
        WPRINTERR("Expected block after if statement");
        return NULL;
    }

    PARSER_DUMP_(block_statement, L"if_block");

    if_lhs->left  = condition;
    if_lhs->right = block_statement;

    PARSER_DUMP_(if_lhs, L"if finale");

    return if_lhs;
}

//==========================================================================================

static TreeNode_t* ParseWhileStatement(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* while_token = LangGetCurrentToken(lang_ctx);

    if (while_token == NULL || !IS_OPERATOR_(while_token, OP_WHILE))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(while_token, L"while");

    TreeNode_t* condition = ParseCondition(lang_ctx);

    if (condition == NULL)
    {
        WPRINTERR("Expected condition inside while");
        return NULL;
    }

    PARSER_DUMP_(condition, L"while_condition");

    TreeNode_t* block_statement = ParseBlockStatement(lang_ctx);

    if (block_statement == NULL)
    {
        WPRINTERR("Expected block after while condition");
        return NULL;
    }

    PARSER_DUMP_(block_statement, L"while block");

    while_token->left  = condition;
    while_token->right = block_statement;

    PARSER_DUMP_(while_token, L"while finale");

    return while_token;
}

//==========================================================================================

static TreeNode_t* ParseCondition(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* lhs = ParseExpression(lang_ctx);

    if (lhs == NULL)
        return NULL;

    PARSER_DUMP_(lhs, L"condition: lhs");

    TreeNode_t* comp = LangGetCurrentToken(lang_ctx);

    if (comp == NULL || !IS_TYPE_(comp, TYPE_OP) ||
        !(HAS_OPCODE_(comp, OP_EQUAL        ) ||
          HAS_OPCODE_(comp, OP_BIGGER_EQUAL ) ||
          HAS_OPCODE_(comp, OP_BIGGER       ) ||
          HAS_OPCODE_(comp, OP_SMALLER_EQUAL) ||
          HAS_OPCODE_(comp, OP_SMALLER      )))
    {
        WPRINTERR("Expected comparison sign in condition");
        return NULL;
    }

    PARSER_DUMP_(comp, L"condition: comp sign");

    lang_ctx->cur_token_index++;

    TreeNode_t* rhs = ParseExpression(lang_ctx);

    if (rhs == NULL)
    {
        WPRINTERR("Expected second expression after sign in condition");
        return NULL;
    }

    PARSER_DUMP_(rhs, L"condition: rhs");

    comp->left  = lhs;
    comp->right = rhs;

    PARSER_DUMP_(comp, L"condition finale");

    return comp;
}

//==========================================================================================

static TreeNode_t* ParseBlockStatement(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* block_open = LangGetCurrentToken(lang_ctx);

    if (block_open == NULL || !IS_OPERATOR_(block_open, OP_BLOCK_BEGIN))
    {
        if (block_open)
            WDPRINTF(L"Got type %ls\n", TYPE_CASES_TABLE[block_open->data.type].name);
        return NULL;
    }

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(block_open, L"block open");

    TreeNode_t* statement = ParseBody(lang_ctx);

    if (statement == NULL)
    {
        WPRINTERR("Expected block body");
        return NULL;
    }

    TreeNode_t* block_close = LangGetCurrentToken(lang_ctx);

    if (block_close == NULL || !IS_OPERATOR_(block_close, OP_BLOCK_END))
    {
        WPRINTERR("There should be a block end");
        return NULL;
    }

    lang_ctx->cur_token_index++;

    WDPRINTF(L"block stmt end: lang_ctx->cur_token_index = %zu\n", lang_ctx->cur_token_index);

    PARSER_DUMP_(block_close, L"block close");
    PARSER_DUMP_(statement, L"block statement");

    return statement;
}

//==========================================================================================

static TreeNode_t* ParseCmdSeparator(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* op_separator = LangGetCurrentToken(lang_ctx);

    if (op_separator == NULL || !IS_OPERATOR_(op_separator, OP_CMD_SEPARATOR))
    {
        return NULL;
    }

    lang_ctx->cur_token_index++;

    return op_separator;
}

//==========================================================================================

static TreeNode_t* ParseVariableDeclaration(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* var_decl = LangGetCurrentToken(lang_ctx);

    if (var_decl == NULL || !IS_OPERATOR_(var_decl, OP_VARIABLE_DECL))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(var_decl, L"var declaration operator");

    TreeNode_t* cur_token = LangGetCurrentToken(lang_ctx);

    if (cur_token == NULL || !IS_IDENTIFIER_(cur_token))
    {
        WPRINTERR(L"Expected identifier in variable declaration");
        return NULL;
    }

    SetIdentifierTokenType(lang_ctx, cur_token, TYPE_VAR_DECL);

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(cur_token, L"var_declaration (finale): identifier");

    IdData_t id_data = {.name_index = cur_token->data.value.id,
                        .type       = ID_TYPE_VARIABLE};

    if (LangFuncWasDeclared(lang_ctx, cur_token->data.value.id))
    {
        WPRINTERR(L"Function with such name as var %ls was declared\n",
                  lang_ctx->names_pool.data[cur_token->data.value.id]);
        return NULL;
    }

    IdTable_t* id_table = NULL;

    if (lang_ctx->is_in_func)
    {
        lang_ctx->in_func_vars_count++;
        id_table = &lang_ctx->func_id_table;
    }
    else
    {
        id_table = &lang_ctx->main_id_table;
    }

    if (LangIdInTable(id_table, id_data.name_index))
    {
        WPRINTERR(L"Variable %ls was already declared\n",
                    lang_ctx->names_pool.data[cur_token->data.value.id]);
        return NULL;
    }
    if (LangIdTablePush(id_table, &id_data))
    {
        return NULL;
    }

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseAssignment(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* assignment_token = LangGetCurrentToken(lang_ctx);

    if (assignment_token == NULL || !IS_OPERATOR_(assignment_token, OP_ASSIGNMENT))
        return NULL;

    lang_ctx->cur_token_index++;

    TreeNode_t* cur_token = ParseVariable(lang_ctx);

    if (cur_token == NULL)
    {
        WPRINTERR("There should be an identifier after assignment");
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"assignment: got variable");

    TreeNode_t* expression = ParseExpression(lang_ctx);

    if (expression == NULL)
    {
        WPRINTERR("There should be an expression after identifier in assignment");
        return NULL;
    }

    assignment_token->left  = cur_token;
    assignment_token->right = expression;

    PARSER_DUMP_(assignment_token, L"assignment");

    return assignment_token;
}

//==========================================================================================

static TreeNode_t* ParseExpression(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = ParseTerm(lang_ctx); // moves cur_token_index by itself

    if (cur_token == NULL)
    {
        WDPRINTF(L"No term in expression parse\n");
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"parse expression got first token");

    TreeNode_t* expr_token = LangGetCurrentToken(lang_ctx);

    if (expr_token == NULL)
        return cur_token;

    TreeNode_t* next_token = NULL;

    while (IS_OPERATOR_(expr_token, OP_ADD) ||
           IS_OPERATOR_(expr_token, OP_SUB))
    {
        lang_ctx->cur_token_index++;

        expr_token->left = cur_token;
        cur_token = expr_token;

        next_token = ParseTerm(lang_ctx); // moves cur_token_index by itself

        if (next_token == NULL)
        {
            WPRINTERR("Should be an argument after expression operation");
            return NULL;
        }

        cur_token->right = next_token;

        expr_token = LangGetCurrentToken(lang_ctx);

        if (expr_token == NULL)
            break;
    }

    PARSER_DUMP_(cur_token, L"expression all");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseTerm(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = ParsePower(lang_ctx); // moves cur_token_index by itself

    if (cur_token == NULL)
    {
        WDPRINTF(L"No power in term parse\n");
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"parse term got first token");

    TreeNode_t* term_token = LangGetCurrentToken(lang_ctx);

    if (term_token == NULL)
        return cur_token;

    TreeNode_t* next_token = NULL;

    while (IS_OPERATOR_(term_token, OP_MUL) ||
           IS_OPERATOR_(term_token, OP_DIV))
    {
        lang_ctx->cur_token_index++;

        term_token->left = cur_token;

        cur_token = term_token;

        next_token = ParsePower(lang_ctx); // moves cur_token_index by itself

        if (next_token == NULL)
        {
            WPRINTERR("Should be an argument after term operation");
            return NULL;
        }

        cur_token->right = next_token;

        term_token = LangGetCurrentToken(lang_ctx);

        if (term_token == NULL)
            break;
    }

    PARSER_DUMP_(cur_token, L"term all");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParsePower(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = ParseFactor(lang_ctx); // moves cur_token_index by itself

    if (cur_token == NULL)
    {
        WDPRINTF(L"No factor in parse power\n");
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"parse power got first token");

    TreeNode_t* power_token = LangGetCurrentToken(lang_ctx);

    if (power_token == NULL)
        return cur_token;

    TreeNode_t* next_token = NULL;

    while (IS_OPERATOR_(power_token, OP_POW))
    {
        lang_ctx->cur_token_index++;

        power_token->left = cur_token;

        cur_token = power_token;

        next_token = ParseFactor(lang_ctx); // moves cur_token_index by itself

        if (next_token == NULL)
        {
            WPRINTERR("Should be an argument after power operation");
            return NULL;
        }

        cur_token->right = next_token;

        power_token = LangGetCurrentToken(lang_ctx);

        if (power_token == NULL)
            break;
    }

    PARSER_DUMP_(cur_token, L"power all");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseFactor(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = NULL;

    cur_token = ParseBracketsExpression(lang_ctx);
    //TODO - error check

    if (cur_token != NULL) // expression with brackets parsed successfully
        return cur_token;

    cur_token = ParseUnaryOperatorCall(lang_ctx);
    //TODO - error check

    if (cur_token != NULL) // unary op call parsed successfully
        return cur_token;

    cur_token = ParseFunctionCall(lang_ctx);
    //TODO - error check

    if (cur_token != NULL) // function call parsed successfully
        return cur_token;

    cur_token = ParseVariable(lang_ctx);

    if (cur_token != NULL) // variable parsed successfully
        return cur_token;

    cur_token = LangGetCurrentToken(lang_ctx);
    //TODO - error check

    if (cur_token && IS_NUMBER_(cur_token)) // number or variable parsed successfully
    {
        PARSER_DUMP_(cur_token, L"number");

        lang_ctx->cur_token_index++;

        return cur_token;
    }

    // WPRINTERR("UNKNOWN TOKEN");
    //TODO - error set

    return NULL;
}

//==========================================================================================

static TreeNode_t* ParseVariable(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = LangGetCurrentToken(lang_ctx);

    if (cur_token == NULL || !IS_IDENTIFIER_(cur_token))
        return NULL;

    SetIdentifierTokenType(lang_ctx, cur_token, TYPE_VAR);

    lang_ctx->cur_token_index++;

    IdTable_t* id_table = NULL;

    if (lang_ctx->is_in_func)
    {
        id_table = &lang_ctx->func_id_table;
    }
    else
    {
        id_table = &lang_ctx->main_id_table;
    }

    if (!LangIdInTable(id_table, cur_token->data.value.id))
    {
        WPRINTERR(L"Variable %ls was not declared\n",
                  lang_ctx->names_pool.data[cur_token->data.value.id]);
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"variable");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseBracketsExpression(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* open_bracket = LangGetCurrentToken(lang_ctx);

    if (open_bracket == NULL || !IS_OPERATOR_(open_bracket, OP_BRACKET_OPEN))
        return NULL;

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(open_bracket, L"opening bracket");

    TreeNode_t* cur_token = ParseExpression(lang_ctx);

    if (cur_token == NULL)
    {
        //TODO - seterror
        WPRINTERR("No expression after opening bracket");
        return NULL;
    }

    TreeNode_t* close_bracket = LangGetCurrentToken(lang_ctx);

    PARSER_DUMP_(close_bracket, L"closing bracket");

    if (close_bracket == NULL || !IS_OPERATOR_(close_bracket, OP_BRACKET_CLOSE))
    {
        //TODO - seterror
        WPRINTERR("No closing bracket after opening bracket");
        return NULL;
    }

    lang_ctx->cur_token_index++;

    PARSER_DUMP_(cur_token, L"in brackets expression");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseFunctionCall(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* function_call_lhs = LangGetCurrentToken(lang_ctx);

    if (function_call_lhs == NULL)
        return NULL;

    if (!IS_OPERATOR_(function_call_lhs, OP_FUNCTION_CALL_LHS))
        return NULL;

    lang_ctx->cur_token_index++;

    TreeNode_t* function_name = LangGetCurrentToken(lang_ctx);

    if (function_name == NULL || !IS_IDENTIFIER_(function_name))
    {
        WPRINTERR("There should be a function identifier after function call");
        // lang_ctx->error_info.error = LANG_SYNTAX_ERROR;
        //TODO - set error
        return NULL;
    }

    SetIdentifierTokenType(lang_ctx, function_name, TYPE_FUNC_CALL);

    lang_ctx->cur_token_index++;

    size_t func_id_index = 0;

    if (LangGetFuncIndex(lang_ctx, function_name->data.value.id, &func_id_index))
    {
        WPRINTERR(L"Function %ls was not declared\n",
                  LangGetIdName(&lang_ctx->names_pool, function_name->data.value.id));
        return NULL;
    }

    TreeNode_t* function_call_rhs = LangGetCurrentToken(lang_ctx);

    if (function_call_rhs == NULL || !IS_OPERATOR_(function_call_rhs, OP_FUNCTION_CALL_RHS))
    {
        WPRINTERR("There should be a function call ending");
        //TODO - set error
        // lang_ctx->error_info.error = LANG_SYNTAX_ERROR;
        return NULL;
    }

    lang_ctx->cur_token_index++;

    int args_count = 0;

    function_name->left = ParseFunctionArguments(lang_ctx, &args_count);

    WDPRINTF(L"args_count for func %ls = %d\n", lang_ctx->names_pool.data[function_name->data.value.id], args_count);

    //TODO - check args_count in ID_TABLE
    //TODO - устанавливать ошибку внутри функции
    if (LangFuncCallRightArgs(lang_ctx, func_id_index, args_count))
    {
        return NULL;
    }

    PARSER_DUMP_(function_name, L"function call");

//TODO - if (LangError()) --> мб макросики какие-нибудь

    return function_name;
}

//==========================================================================================

static TreeNode_t* ParseFunctionArguments(LangCtx_t* lang_ctx, int* args_count_p)
{
    assert(lang_ctx);

    *args_count_p = 0;

    TreeNode_t* cur_token = ParseExpression(lang_ctx); // moves cur_token_index by itself

    if (cur_token == NULL)
    {
        WDPRINTF(L"No args in function\n");
        return NULL;
    }

    PARSER_DUMP_(cur_token, L"function first argument");

    (*args_count_p)++;

    TreeNode_t* params_separator = LangGetCurrentToken(lang_ctx);

    if (params_separator == NULL)
        return cur_token;

    TreeNode_t* next_param = NULL;

    while (IS_OPERATOR_(params_separator, OP_PARAMS_SEPARATOR))
    {
        (*args_count_p)++;

        lang_ctx->cur_token_index++;

        params_separator->left = cur_token;

        cur_token = params_separator;

        next_param = ParseExpression(lang_ctx); // moves cur_token_index by itself

        if (next_param == NULL)
        {
            WPRINTERR("Should be an argument after param separator");
            return NULL;
        }

        cur_token->right = next_param;

        params_separator = LangGetCurrentToken(lang_ctx);

        if (params_separator == NULL)
            break;
    }

    PARSER_DUMP_(cur_token, L"function arguments all");

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseUnaryOperatorCall(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    TreeNode_t* cur_token = ParseUnaryOperator(lang_ctx);

    if (cur_token == NULL)
        return NULL;

    cur_token->right = ParseExpression(lang_ctx);

    PARSER_DUMP_(cur_token, L"unary operator call");

    if (cur_token->right == NULL)
    {
        WPRINTERR("After unary op there should be an expression");
        return NULL;
    }

    return cur_token;
}

//==========================================================================================

static TreeNode_t* ParseUnaryOperator(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    // WDPRINTF(L"Not an unary op\n");

    TreeNode_t* cur_token = LangGetCurrentToken(lang_ctx);

    if (cur_token == NULL || cur_token->data.type != TYPE_OP)
        return NULL;

    if ((cur_token->data.value.opcode == OP_OUTPUT) |
        (cur_token->data.value.opcode == OP_INPUT ) |
        (cur_token->data.value.opcode == OP_SQRT  ) |
        (cur_token->data.value.opcode == OP_DRAW  ) |
        (cur_token->data.value.opcode == OP_RETURN))
    {
        PARSER_DUMP_(cur_token, L"unary operator");
        lang_ctx->cur_token_index++;
        return cur_token;
    }

    return NULL;
}

//==========================================================================================

static void SetIdentifierTokenType(LangCtx_t* lang_ctx, TreeNode_t* cur_token,
                                   TokenType_t new_type)
{
    assert(cur_token);
    assert(lang_ctx);

    assert(cur_token->data.type == TYPE_ID);

    cur_token->data.type = new_type;
}

//==========================================================================================

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_UNDEF_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————
