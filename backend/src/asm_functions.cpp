#include "op_cases.h"
#include "lang_funcs.h"

// сдвинуть rhx в самом начале под все переменные

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_DEFINE_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t AssembleNumber              (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleVariable            (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleVariableBody        (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleVariableDeclaration (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleFunctionDeclaration (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleNewVariable         (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleFunctionParameters  (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleFunctionCall        (LangCtx_t* lang_ctx, TreeNode_t* node);
static LangErr_t AssembleFunctionArguments   (LangCtx_t* lang_ctx, TreeNode_t* node);
static void      AssembleArgument            (LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t AssembleNode(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    switch (node->data.type)
    {
        case TYPE_NUM:
            return AssembleNumber(lang_ctx, node);

        case TYPE_ID:
            return LANG_INVALID_AST_INPUT;

        case TYPE_OP:
            if (OP_CASES_TABLE[node->data.value.opcode].asm_function == NULL)
            {
                WPRINTERR("Error: operator %ls doesn't support assembling",
                          OP_CASES_TABLE[node->data.value.opcode].name);

                return LANG_UNASSEMBLE_OPERATOR;
            }
            return OP_CASES_TABLE[node->data.value.opcode].asm_function (lang_ctx, node);

        case TYPE_VAR:
            return AssembleVariable(lang_ctx, node);

        case TYPE_VAR_DECL:
            return AssembleVariableDeclaration(lang_ctx, node);

        case TYPE_FUNC_DECL:
            return AssembleFunctionDeclaration(lang_ctx, node);

        case TYPE_FUNC_CALL:
            return AssembleFunctionCall(lang_ctx, node);

        default:
            return LANG_UNKNOWN_TOKEN_TYPE;
    }

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleNumber(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(node);

    ASM_VERIFY_(IS_NUMBER_(node));
    ASM_VERIFY_(node->left  == NULL);
    ASM_VERIFY_(node->right == NULL);

    ASM_PRINT_(L"; number: %lg\n", node->data.value.number);

    ASM_PRINT_(L"PUSH %lg\n", node->data.value.number);

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleVariable(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    if (lang_ctx->getting_function_params == true)
    {
        return AssembleNewVariable(lang_ctx, node);
    }

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleVariableBody(lang_ctx, node)))
    {
        return error;
    }

    ASM_PRINT_(L"PUSHM [RBX] ; push [rbp + addr] \n");
    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleVariableBody(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_VARIABLE_(node));
    ASM_VERIFY_(node->left  == NULL);
    ASM_VERIFY_(node->right == NULL);

    IdTable_t* id_table = NULL;

    if (lang_ctx->is_in_function)
    {
        id_table = &lang_ctx->func_id_table;
    }
    else
    {
        id_table = &lang_ctx->main_id_table;
    }

    int addr = 0;

    if (LangIdTableGetAddress(id_table, node->data.value.id, &addr))
    {
        WPRINTERR(L"Syntax error: variable %ls was not declared\n"
                  L"lang_ctx->is_in_function = %d\n",
                    lang_ctx->names_pool.data[node->data.value.id],
                    lang_ctx->is_in_function ? 1 : 0);
        return LANG_VAR_NOT_DECLARED;
    }

    ASM_PRINT_(L"; variable %ls (addr = %zu)\n\n",
                lang_ctx->names_pool.data[node->data.value.id],
                addr);

    WDPRINTF(L"; variable %ls (addr = %zu)\n\n",
                lang_ctx->names_pool.data[node->data.value.id],
                addr);

    if (lang_ctx->is_in_function)
    {
        WDPRINTF(L"assemble var %ls func table dump:\n",
                lang_ctx->names_pool.data[node->data.value.id]);
        LangIdTableDump(&lang_ctx->func_id_table);
        ASM_PRINT_(L"; rbp + %zu (local address)\n", addr);
        ASM_PRINT_(L"PUSHR RGX ; rbp\n");
        ASM_PRINT_(L"PUSH %zu ; local addr\n", addr);
        ASM_PRINT_(L"ADD\n");
    }
    else
    {
        WDPRINTF(L"assemble var %ls main table dump:\n",
                lang_ctx->names_pool.data[node->data.value.id]);
        LangIdTableDump(&lang_ctx->main_id_table);
        ASM_PRINT_(L"PUSH %zu ; global addr\n", addr);
    }

    ASM_PRINT_(L"POPR RBX ; RBX = global addr\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleVariableDeclaration(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_VAR_DECL_(node));
    ASM_VERIFY_(node->left  == NULL);
    ASM_VERIFY_(node->right == NULL);

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNewVariable(lang_ctx, node)))
        return error;

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleNewVariable(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_PRINT_(L"; variable declaration: %ls\n\n",
               lang_ctx->names_pool.data[node->data.value.id]);

    IdTable_t* id_table = NULL;

    if (lang_ctx->is_in_function)
    {
        id_table = &lang_ctx->func_id_table;
    }
    else
    {
        id_table = &lang_ctx->main_id_table;
    }

    // if (lang_ctx->getting_function_params)
    // {
    //     lang_ctx->params_count++;
    // }

    LangErr_t error = LANG_SUCCESS;

    WDPRINTF(L"setting addr for var %ls = %zu\n",
            lang_ctx->names_pool.data[node->data.value.id],
            lang_ctx->cur_addr);

    IdData_t var_id_data = {.name_index = node->data.value.id,
                            .name = lang_ctx->names_pool.data[node->data.value.id],
                            .type = ID_TYPE_VARIABLE,
                            .addr = lang_ctx->cur_addr};

    if ((error = LangIdTablePush(id_table, &var_id_data)))
        return error;

    lang_ctx->cur_addr++;

    WDPRINTF(L"Incremented cur_addr\n");

//     ASM_PRINT_(L"; pushing stack of variables (rsp++)\n");
//
//     ASM_PRINT_(L"PUSHR RHX ; rsp\n");
//     ASM_PRINT_(L"PUSH 1\n");
//     ASM_PRINT_(L"ADD\n");
//     ASM_PRINT_(L"POPR RHX ; rsp = rsp + 1\n");
//     ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleFunctionDeclaration(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    int last_cur_addr = lang_ctx->cur_addr;

    ASM_VERIFY_(IS_FUNC_DECL_(node));
    ASM_VERIFY_(node->right != NULL);

    if (lang_ctx->is_in_function)
    {
        WPRINTERR("Syntax error: function declaration inside of function is not supported");
        return LANG_FUNC_DECL_IN_FUNC;
    }

    size_t table_index = 0;

    if (!LangGetIdInTable(&lang_ctx->main_id_table, node->data.value.id, &table_index))
    {
        WPRINTERR(L"Error: declared function is not in table %ls",
                    lang_ctx->names_pool.data[node->data.value.id]);
        return LANG_FUNC_REDECLARATION;
    }

    LangErr_t error = LANG_SUCCESS;

    if ((error = LangIdTableCtor(&lang_ctx->func_id_table)))
        return error;

    lang_ctx->is_in_function = true;

    ASM_PRINT_(L"; function declaration: %ls\n\n",
               lang_ctx->names_pool.data[node->data.value.id]);

// transliterate?
    ASM_PRINT_(L"JMP :%ls_end\n", lang_ctx->names_pool.data[node->data.value.id]);
    ASM_PRINT_(L":%ls\n", lang_ctx->names_pool.data[node->data.value.id]);

    if (node->left)
    {
        if ((error = AssembleFunctionParameters(lang_ctx, node)))
        {
            LangIdTableDtor(&lang_ctx->func_id_table);
            WPRINTERR("Parameters error");
            return error;
        }
    }

    ASM_PRINT_(L"; after assemble_params: cur_addr = %zu\n", lang_ctx->cur_addr);

    // wfcprintf(stderr, RED, L"pushing in table: n_params = %zu\n", lang_ctx->params_count);

    // if ((error = LangIdTablePush(&lang_ctx->main_id_table, node->data.value.id,
    //                              ID_TYPE_FUNCTION, lang_ctx->params_count)))
    // {
    //     LangIdTableDtor(&lang_ctx->func_id_table);
    //     return error;
    // }

    if ((error = AssembleNode(lang_ctx, node->right)))
    {
        LangIdTableDtor(&lang_ctx->func_id_table);
        return error;
    }

    ASM_PRINT_(L":%ls_end\n", lang_ctx->names_pool.data[node->data.value.id]);

    lang_ctx->is_in_function = false;
    lang_ctx->cur_addr = last_cur_addr;

    LangIdTableDtor(&lang_ctx->func_id_table);

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleFunctionCall(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_FUNC_CALL_(node));

    size_t id_index = 0;

    if (!LangGetIdInTable(&lang_ctx->main_id_table, node->data.value.id, &id_index))
    {
        WPRINTERR(L"Syntax error: function %ls was not declared",
                    lang_ctx->names_pool.data[node->data.value.id]);
        return LANG_FUNC_NOT_DECLARED;
    }

    ASM_PRINT_(L"; function call %ls\n\n", lang_ctx->names_pool.data[node->data.value.id]);

    ASM_PRINT_(L"PUSHR RGX ; save rbp\n\n");
    ASM_PRINT_(L"; copy rsp to rbp\n");
    ASM_PRINT_(L"PUSHR RHX\n");
    ASM_PRINT_(L"POPR RGX\n");

    // move stack pointer to the end of rbp + memory needed

    ASM_PRINT_(L"; move rsp to the end of function call\n");
    ASM_PRINT_(L"PUSHR RHX\n"); // rsp
    ASM_PRINT_(L"PUSH %zu\n", lang_ctx->main_id_table.data[id_index].memory_needed); // memory
    ASM_PRINT_(L"ADD\n");
    ASM_PRINT_(L"POPR RHX\n");

    LangErr_t error = LANG_SUCCESS;

    size_t n_params = lang_ctx->main_id_table.data[id_index].n_params;

    if (node->left)
    {
        if ((error = AssembleFunctionArguments(lang_ctx, node)))
            return error;
    }

    if (n_params != lang_ctx->params_count)
    {
        WDPRINTF(L"n_params = %zu | lang_ctx->params_count = %zu\n",
                 n_params, lang_ctx->params_count);

        WPRINTERR(L"Syntax error: wrong args count for %ls",
                  lang_ctx->names_pool.data[node->data.value.id]);

        return LANG_WRONG_ARGS_COUNT;
    }

    ASM_PRINT_(L"CALL :%ls\n", lang_ctx->names_pool.data[node->data.value.id]);
    ASM_PRINT_(L"PUSHR RAX ; get return value\n");

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleFunctionParameters(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_PRINT_(L"; function parameters\n\n");

    lang_ctx->getting_function_params = true;

    LangErr_t error = LANG_SUCCESS;

    if (node->left)
    {
        if ((error = AssembleNode(lang_ctx, node->left)))
            return error;
    }
    // if (node->right)
    // {
    //     if ((error = AssembleNode(lang_ctx, node->right)))
    //         return error;
    // }

    lang_ctx->getting_function_params = false;

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleFunctionArguments(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_PRINT_(L"; function arguments\n\n");

    lang_ctx->assembling_args = true;
    lang_ctx->params_count = 0;

    if (!node->left)
        return LANG_SUCCESS;

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->left)))
        return error;

    if (!IS_OPERATOR_(node->left, OP_PARAMS_SEPARATOR))
    {
        AssembleArgument(lang_ctx);
    }

    // if ((error = AssembleNode(lang_ctx, node->right)))
    //     return error;

    ASM_PRINT_(L"\n");

    lang_ctx->assembling_args = false;

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleReturn(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_RETURN));
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; return\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"; set rsp (RHX) to current rbp (RGX)\n");
    ASM_PRINT_(L"PUSHR RGX\n");
    ASM_PRINT_(L"POPR RHX\n\n");
    ASM_PRINT_(L"POPR RAX ; put return value in RAX\n");
    ASM_PRINT_(L"; get previous rbp (RGX) from stack\n");
    ASM_PRINT_(L"POPR RGX\n\n");

    ASM_PRINT_(L"RET\n");
    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static void AsmIncrementRsp(LangCtx_t* lang_ctx)
{
    ASM_PRINT_(L"; rsp++\n");
    ASM_PRINT_(L"PUSHR RHX\n");
    ASM_PRINT_(L"PUSH 1\n");
    ASM_PRINT_(L"ADD\n");
    ASM_PRINT_(L"POPR RHX\n");
    lang_ctx->cur_addr++;
}

//==========================================================================================

LangErr_t AssembleParamsSeparator(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_PARAMS_SEPARATOR));

    LangErr_t error = LANG_SUCCESS;

    if (node->right)
    {
        if ((error = AssembleNode(lang_ctx, node->right)))
            return error;

        if (lang_ctx->assembling_args)
        {
            AssembleArgument(lang_ctx);
        }
    }
    if (node->left)
    {
        if ((error = AssembleNode(lang_ctx, node->left)))
            return error;

        if (lang_ctx->assembling_args && !IS_OPERATOR_(node->left, OP_PARAMS_SEPARATOR))
        {
            AssembleArgument(lang_ctx);
        }
    }

    return LANG_SUCCESS;
}

//==========================================================================================

static void AssembleArgument(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    ASM_PRINT_(L"; argument %zu \n", lang_ctx->params_count);
    ASM_PRINT_(L"PUSHR RGX ; rbp\n");
    ASM_PRINT_(L"PUSH %zu ; local addr\n", lang_ctx->params_count);
    ASM_PRINT_(L"ADD \n");
    ASM_PRINT_(L"POPR RBX ;\n");
    ASM_PRINT_(L"POPM [RBX] ; RBX = RGX + addr\n\n");
    lang_ctx->params_count++;
}

//==========================================================================================

LangErr_t AssembleCmdSeparator(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_CMD_SEPARATOR));
    ASM_VERIFY_(node->left);

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->left)))
        return error;

    /*NOTE - there might be a cmd_separator with only left node
             for AST standard
    */

    if (node->right == NULL)
        return LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleCondition(LangCtx_t* lang_ctx, TreeNode_t* node);

//——————————————————————————————————————————————————————————————————————————————————————————

LangErr_t AssembleIf(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_IF_LHS));
    ASM_VERIFY_(node->left );
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; if\n");
    ASM_PRINT_(L"; ----------------condition----------------\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleCondition(lang_ctx, node->left)))
        return error;

    size_t label_count = lang_ctx->endif_labels_count;
    lang_ctx->endif_labels_count++;

    ASM_PRINT_(L" :endif_%zu\n", label_count);

    ASM_PRINT_(L"; ----------------statement----------------\n\n");

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L":endif_%zu\n", label_count);

    ASM_PRINT_(L"; ------------------endif------------------\n\n");

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleWhile(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_WHILE));
    ASM_VERIFY_(node->left );
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; while\n");

    size_t labels_count = lang_ctx->while_labels_count;
    lang_ctx->while_labels_count++;

    ASM_PRINT_(L":while_start_%zu\n\n", labels_count);
    ASM_PRINT_(L"; ----------------condition----------------\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleCondition(lang_ctx, node->left)))
        return error;

    ASM_PRINT_(L" :while_end_%zu\n", labels_count);

    ASM_PRINT_(L"; ----------------statement----------------\n\n");

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"JMP :while_start_%zu\n", labels_count);
    ASM_PRINT_(L":while_end_%zu\n", labels_count);

    ASM_PRINT_(L"; ----------------while_end----------------\n\n");

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t AssembleCondition(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_TYPE_(node, TYPE_OP));
    ASM_VERIFY_(node->left );
    ASM_VERIFY_(node->right);

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->left)))
        return error;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"%ls", OP_CASES_TABLE[node->data.value.opcode].asm_name);

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleAssignment(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_ASSIGNMENT));
    ASM_VERIFY_(node->left && IS_VARIABLE_(node->left));
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; assignment:\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    if ((error = AssembleVariableBody(lang_ctx, node->left)))
        return error;

    ASM_PRINT_(L"POPM [RBX] ; pop [rbp + addr] \n");
    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleMathOperation(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_ADD) ||
                IS_OPERATOR_(node, OP_SUB) ||
                IS_OPERATOR_(node, OP_MUL) ||
                IS_OPERATOR_(node, OP_DIV) ||
                IS_OPERATOR_(node, OP_POW));

    ASM_VERIFY_(node->left );
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; math operation: %ls\n\n", OP_CASES_TABLE[node->data.value.opcode].asm_name);

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->left)))
        return error;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"%ls\n", OP_CASES_TABLE[node->data.value.opcode].asm_name);

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssemblePoint(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    LangErr_t error = LANG_SUCCESS;

    if (lang_ctx->first_point)
    {
        ASM_PRINT_(L"; start DRAW there\n");
        ASM_PRINT_(L"PUSH RHX\n\n");
        lang_ctx->first_point = false;
    }

    ASM_PRINT_(L"PUSH %d", '#');
    ASM_PRINT_(L"PUSHR RHX\n");

    if ((error = AssembleNode(lang_ctx, node->right)))
    {
        return error;
    }

    ASM_PRINT_(L"ADD\n");
    ASM_PRINT_(L"POPR RCX\n");
    ASM_PRINT_(L"POPM [RCX]\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleUnaryOperation(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_INPUT ) ||
                IS_OPERATOR_(node, OP_OUTPUT) ||
                IS_OPERATOR_(node, OP_DRAW  ) ||
                IS_OPERATOR_(node, OP_SQRT  ));

    ASM_VERIFY_(node->left == NULL);
    ASM_VERIFY_(node->right);

    ASM_PRINT_(L"; output\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"%ls\n", OP_CASES_TABLE[node->data.value.opcode].asm_name);

    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleInput(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_INPUT));

    ASM_VERIFY_(node->left == NULL);
    ASM_VERIFY_(node->right && IS_VARIABLE_(node->right));

    ASM_PRINT_(L"; input\n\n", lang_ctx->names_pool.data[node->right->data.value.id]);
    ASM_PRINT_(L"%ls\n", OP_CASES_TABLE[node->data.value.opcode].asm_name);

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleVariableBody(lang_ctx, node->right)))
        return error;

    ASM_PRINT_(L"POPM [RBX] ; push [rbp + addr] \n");
    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//==========================================================================================

LangErr_t AssembleHlt(LangCtx_t* lang_ctx, TreeNode_t* node)
{
    assert(lang_ctx);
    assert(node);

    ASM_VERIFY_(IS_OPERATOR_(node, OP_ABORT));
    ASM_VERIFY_(node->left  == NULL);
    ASM_VERIFY_(node->right == NULL);

    ASM_PRINT_(L"; halt\n\n");

    ASM_PRINT_(L"%ls\n", OP_CASES_TABLE[node->data.value.opcode].asm_name);
    ASM_PRINT_(L"\n");

    return LANG_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_UNDEF_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————
