#include "tree_commands.h"
#include "lang_funcs.h"
#include "lexer.h"
#include "data_read.h"
#include <wchar.h>
#include <locale.h>
#include "parser.h"
#include "AST_write.h"

//==========================================================================================

//TODO - в readme parser и lexer GRAMMAR

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "en_US.utf8");

    if (argc == 1)
    {
        WPRINTERR(L"Expected code input file through terminal args");
        return EXIT_FAILURE;
    }

    LangCtx_t lang_ctx = {};

    if (LangCtxCtor(&lang_ctx))
        return EXIT_FAILURE;

    WDPRINTF(L"tree_ptr = %p\n", lang_ctx.tree);

    do {
        if (TreeReadInputData(&lang_ctx, argv[1]))
            break;

        if (Tokenize(&lang_ctx))
            break;

        if (ParseTokens(&lang_ctx))
            break;

        if (ASTWriteData(&lang_ctx))
            break;

    } while (0);

    if (lang_ctx.error_info.error != LANG_SUCCESS)
        LangPrintError(&lang_ctx);

    LangCtxDtor(&lang_ctx);

    return EXIT_SUCCESS;
}

//==========================================================================================
