#include "backend.h"
#include "lang_funcs.h"

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_DEFINE_
#include "dsl.h"

//==========================================================================================

LangErr_t AssembleProgram(LangCtx_t* lang_ctx)
{
    assert(lang_ctx);

    ASM_PRINT_(L"; push rbp for all global vars\n");
    ASM_PRINT_(L"PUSH %zu\n", lang_ctx->global_vars_count);
    ASM_PRINT_(L"POPR RGX\n");
    ASM_PRINT_(L"PUSHR RGX\n");
    ASM_PRINT_(L"POPR RHX\n\n");

    LangErr_t error = LANG_SUCCESS;

    if ((error = AssembleNode(lang_ctx, lang_ctx->tree.dummy->right)))
        return error;

    ASM_PRINT_(L"; end program\n\n");

    ASM_PRINT_(L"HLT\n");

    return LANG_SUCCESS;
}

//——————————————————————————————————————————————————————————————————————————————————————————

#define _DSL_UNDEF_
#include "dsl.h"

//——————————————————————————————————————————————————————————————————————————————————————————
