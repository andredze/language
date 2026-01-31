#include "lexer.h"
#include "lang_ctx.h"
#include "lang_funcs.h"
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_DEFINE_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————

static inline int IsAcceptableFirstSymbol(wint_t ch);
static inline int IsAcceptableSymbol     (wint_t ch);

static void PrintSyntaxError(LangCtx_t* lang_ctx, const char* file, const char* func,
                             const int  line,     const char* fmt, ...);

//==========================================================================================

#define SYNTAX_ERROR(lang_ctx, fmt, ...)                                                                \
        {   TreeDumpInfo_t dump_info = {TREE_SUCCESS, __PRETTY_FUNCTION__, __FILE__, __LINE__};         \
            PrintSyntaxError(lang_ctx, __FILE__, __PRETTY_FUNCTION__, __LINE__, (fmt), ##__VA_ARGS__);  \
            TreeDump((lang_ctx), &dump_info, DUMP_SHORT, (fmt), ##__VA_ARGS__);                         \
            return LANG_SYNTAX_ERROR; }

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t ParseToken(LangCtx_t* lang_ctx);

//——————————————————————————————————————————————————————————————————————————————————————————

    // TODO: SyntaxError(); with lines and |
                                        // |
                                        // |
    // TODO: mystrncmp позволяющий любой регистр
    // TODO: алгорифмы Маркова

// FIXME: delete

static void PrintSyntaxError(LangCtx_t* lang_ctx, const char* file, const char* func,
                             const int  line,     const char* fmt, ...)
{
    char message[MAX_SYNTAX_ERR_MESSAGE_LEN] = {};

    va_list args = {};
    va_start(args, fmt);

    vsnprintf(message, sizeof(message), fmt, args);

    va_end(args);

    wfcprintf(stderr, RED, L"ERROR from %s at %s:%d\n\tSyntax error: %s\n"
                           L"  %d   | ",
                           func, file, line, message, lang_ctx->current_line);

    wchar_t* cur_symbol_ptr = lang_ctx->cur_symbol_ptr;

    while (*cur_symbol_ptr != L'\n' && *cur_symbol_ptr != L'\0')
    {
        fwprintf(stderr, L"%lc", *cur_symbol_ptr);
        cur_symbol_ptr++;
    }

    fwprintf(stderr, L"\n"
                     L"      |\n"
                     L"      |\n");

    fwprintf(stderr, L"code:\n");

    for (int i = 0; i < lang_ctx->cur_symbol_ptr - lang_ctx->buffer; i++)
    {
        wfcprintf(stderr, GRAY, L"%lc", lang_ctx->buffer[i]);
    }

    wfcprintf(stderr, RED, L"%lc", *lang_ctx->cur_symbol_ptr);

    wfcprintf(stderr, BLUE, L"%ls\n", lang_ctx->cur_symbol_ptr + 1);
}

//==========================================================================================

LangErr_t Tokenize(LangCtx_t* lang_ctx)
{
    assert(lang_ctx       != NULL);
    assert(lang_ctx->cur_symbol_ptr != NULL);

    LangErr_t status = LANG_SUCCESS;

    wfcprintf(stderr, BLUE, L"Tokenizing...\n");

    // WDPRINTF(L"LEXER: code = %ls\n", lang_ctx->cur_symbol_ptr);

    while (*lang_ctx->cur_symbol_ptr != '\0')
    {
        if ((status = ParseToken(lang_ctx)))
            return status;
    }

    wfcprintf(stderr, GREEN, L"Tokenizing success\n");

    return LANG_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t ProcessOperatorTokenCase   (LangCtx_t* lang_ctx, bool* do_continue);
static LangErr_t ProcessNumberTokenCase     (LangCtx_t* lang_ctx, bool* do_continue);
static LangErr_t ProcessIdentifierTokenCase (LangCtx_t* lang_ctx, bool* do_continue);
static void      ProcessSpacesCase          (LangCtx_t* lang_ctx, bool* do_continue);

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t ParseToken(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    LangErr_t status = LANG_SUCCESS;
    bool do_continue = false;

    ProcessSpacesCase(lang_ctx, &do_continue);

    if (do_continue)
        return status;

    status = ProcessOperatorTokenCase(lang_ctx, &do_continue);

    if (status != LANG_SUCCESS || do_continue)
        return status;

    status = ProcessNumberTokenCase(lang_ctx, &do_continue);

    if (status != LANG_SUCCESS || do_continue)
        return status;

    status = ProcessIdentifierTokenCase(lang_ctx, &do_continue);

    if (status != LANG_SUCCESS || do_continue)
        return status;

    SET_LEXER_ERROR_(LANG_LEXER_SYNTAX_ERROR, NULL, L"Unacceptable symbol");

    return LANG_LEXER_SYNTAX_ERROR;
}

//——————————————————————————————————————————————————————————————————————————————————————————

#define PUSH_TOKEN_(lang_ctx, __token)                                                          \
        TreeNode_t* _token = __token;                                                           \
        TreeDumpInfo_t _dump_info_ = {TREE_SUCCESS, __func__, __FILE__, __LINE__};              \
        GraphDump(lang_ctx, (_token), &_dump_info_, DUMP_SHORT, L"pushing token %p\n", (_token)); \
                                                                                                \
        WDPRINTF(L"Pushing token %p\n", (_token));                                              \
                                                                                                \
        if (StackPush(&lang_ctx->tokens, (_token)))                                             \
        {                                                                                       \
            WPRINTERR("Failed stack push token");                                               \
            return LANG_STACK_ERROR;                                                            \
        }

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t ProcessOperatorTokenRepetitions(LangCtx_t* lang_ctx, size_t op_code);

//——————————————————————————————————————————————————————————————————————————————————————————

static LangErr_t ProcessOperatorTokenCase(LangCtx_t* lang_ctx, bool* do_continue)
{
    assert(do_continue);
    assert(lang_ctx);

    LangErr_t status = LANG_SUCCESS;

    for (size_t op_code = 1; op_code < OPERATORS_COUNT; op_code++)
    {
        const wchar_t* opname     = OP_CASES_TABLE[op_code].name;
        size_t         opname_len = OP_CASES_TABLE[op_code].name_len;

        // WDPRINTF(L"code = \"%ls\"\n"
        //          L"op_name = %ls;\n"
        //          L"op_len = %zu;\n"
        //          L"------------------------------\n",
        //          lang_ctx->cur_symbol_ptr,
        //          opname,
        //          opname_len);

        if (wcsncmp(lang_ctx->cur_symbol_ptr, opname, opname_len) == 0 &&
            !(op_code != OP_BRACKET_OPEN  &&
              op_code != OP_BRACKET_CLOSE &&
              IsAcceptableSymbol(lang_ctx->cur_symbol_ptr[opname_len])))
        {

            lang_ctx->cur_symbol_ptr += opname_len;

            if ((status = ProcessOperatorTokenRepetitions(lang_ctx, op_code)))
                return status;

            *do_continue = true;

            PUSH_TOKEN_(lang_ctx, OPERATOR_((Operator_t) op_code));

            return LANG_SUCCESS;
        }
    }

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t ProcessOperatorTokenRepetitions(LangCtx_t* lang_ctx, size_t op_code)
{
    assert(lang_ctx);

    const wchar_t* opname          = OP_CASES_TABLE[op_code].name;
    size_t         opname_len      = OP_CASES_TABLE[op_code].name_len;
    int            op_repeat_times = OP_CASES_TABLE[op_code].repeat_times;

    for (int i = 1; i < op_repeat_times; i++)
    {
        ProcessSpacesCase(lang_ctx, NULL);

        if (!(wcsncmp(lang_ctx->cur_symbol_ptr, opname, opname_len) == 0 &&
              !IsAcceptableSymbol(lang_ctx->cur_symbol_ptr[opname_len])))
        {
            SET_LEXER_ERROR_(LANG_LEXER_SYNTAX_ERROR, NULL, L"operator should repeat %d times", op_repeat_times);
            // SYNTAX_ERROR(lang_ctx, "operator should repeat %d times", op_repeat_times);
        }

        lang_ctx->cur_symbol_ptr += opname_len;
    }

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t ProcessNumberTokenCase(LangCtx_t* lang_ctx, bool* do_continue)
{
    assert(do_continue);
    assert(lang_ctx);

    if ((*lang_ctx->cur_symbol_ptr != L'-') && !isdigit(*lang_ctx->cur_symbol_ptr))
        return LANG_SUCCESS;

    if (*lang_ctx->cur_symbol_ptr == L'-' && !isdigit(*(lang_ctx->cur_symbol_ptr + 1)))
    {
        SET_LEXER_ERROR_(LANG_LEXER_SYNTAX_ERROR, NULL,
                         L"Invalid sequence: minus should be followed with a number");

        return LANG_LEXER_SYNTAX_ERROR;
    }

    *do_continue = true;

    double   value = 0.0;
    wchar_t* num_code_end = NULL;

    value = wcstod(lang_ctx->cur_symbol_ptr, &num_code_end);

    lang_ctx->cur_symbol_ptr = num_code_end;

    PUSH_TOKEN_(lang_ctx, NUMBER_(value));

    return LANG_SUCCESS;
}

//==========================================================================================

static LangErr_t ProcessIdentifierTokenCase(LangCtx_t* lang_ctx, bool* do_continue)
{
    assert(do_continue);
    assert(lang_ctx);

    if (!IsAcceptableFirstSymbol(*lang_ctx->cur_symbol_ptr))
        return LANG_SUCCESS;

    *do_continue = true;

    wchar_t buf[MAX_OPERATOR_NAME_LEN] = L"";

    size_t i = 0;

    do
    {
        buf[i++] = *(lang_ctx->cur_symbol_ptr++);
    }
    while (IsAcceptableSymbol(*lang_ctx->cur_symbol_ptr));

    size_t name_index = 0;

    LangErr_t status = LANG_SUCCESS;

    if ((status = LangNamesPoolPush(&lang_ctx->names_pool, buf, &name_index)))
        return status;

    PUSH_TOKEN_(lang_ctx, IDENTIFIER_(name_index));

    return LANG_SUCCESS;
}

//==========================================================================================

#undef PUSH_TOKEN_

//==========================================================================================

static inline int IsAcceptableSymbol(wint_t ch)
{
   return iswalnum(ch) || ch == '_';
}

//==========================================================================================

static inline int IsAcceptableFirstSymbol(wint_t ch)
{
    return iswalpha(ch) || ch == '_';
}

//==========================================================================================

static void ProcessSpacesCase(LangCtx_t* lang_ctx, bool* do_continue)
{
    assert(lang_ctx);

    if (!iswspace(*lang_ctx->cur_symbol_ptr))
        return;

    if (do_continue != NULL)
        *do_continue = true;

    do
    {
        if (*lang_ctx->cur_symbol_ptr == '\n')
        {
            lang_ctx->current_line++;
        }
        lang_ctx->cur_symbol_ptr++;
    }
    while (iswspace(*lang_ctx->cur_symbol_ptr));
}

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_UNDEF_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————
