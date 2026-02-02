#include "backend.h"
#include "AST_read.h"
#include "lang_funcs.h"
#include <locale.h>

//==========================================================================================

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "en_US.utf8");

    if (argc == 1)
    {
        WPRINTERR("Expected ast input file through terminal args");
        return EXIT_FAILURE;
    }

    LangCtx_t lang_ctx = {};

    if (LangCtxCtor(&lang_ctx))
        return EXIT_FAILURE;

    do {
        if (ASTReadData(&lang_ctx, argv[1]))
            break;

        LangIdTableDump(&lang_ctx.main_id_table);

        if (LangOpenAsmFile(&lang_ctx))
            break;

        if (AssembleProgram(&lang_ctx))
            break;

    } while (0);

    LangCtxDtor(&lang_ctx);

    return EXIT_SUCCESS;
}

//==========================================================================================
