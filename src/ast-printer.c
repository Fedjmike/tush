#include "ast-printer.h"

#include <stdio.h>

#include "common.h"
#include "type.h"
#include "ast.h"
#include "sym.h"

typedef struct printerCtx {
    int depth;
} printerCtx;

static printf_t* printer_outf (printerCtx* ctx) {
    putnchar(' ', ctx->depth*2 + 1);
    return printf;
}

static void printer (printerCtx* ctx, const ast* node);

/*==== ====*/

static void printChildren (printerCtx* ctx, const ast* node) {
    for_vector (ast* child, node->children, {
        printer(ctx, child);
    })
}

static void printLR (printerCtx* ctx, const ast* node) {
    if (node->l)
        printer(ctx, node->l);

    if (node->r)
        printer(ctx, node->r);
}

static void printer (printerCtx* ctx, const ast* node) {
    printer_outf(ctx)("+ %s\n", astKindGetStr(node->kind));
    ctx->depth++;

    switch (node->kind) {
    case astUnitLit:
        printer_outf(ctx)("unit: ()\n");
        break;

    case astIntLit:
        printer_outf(ctx)("int: %ld\n", node->literal.integer);
        break;

    case astBoolLit:
        printer_outf(ctx)("bool: %s\n", node->literal.truth ? "true" : "false");
        break;

    case astStrLit:
        printer_outf(ctx)("\"%s\"\n", node->literal.str);
        break;

    case astFileLit:
        printer_outf(ctx)("file: %s\n", node->literal.str);
        break;

    case astGlobLit:
        printer_outf(ctx)("glob: %s\n", node->literal.str);
        break;

    case astSymbol:
    case astLet:
        printer_outf(ctx)("symbol: %s\n",   !node->symbol ? "<no symbol attached>"
                                          : !node->symbol->name ? "<unnamed symbol>"
                                          : node->symbol->name);
        break;

    case astListLit:
    case astTupleLit:
    case astFnLit:
    case astFnApp:
    case astBOP:
    case astInvalid:
        break;

    case astKindNo:
        errprintf("Dummy AST kind, KindNo, found in the wild\n");
    }

    printChildren(ctx, node);
    printLR(ctx, node);

    if (node->op != opNull)
        printer_outf(ctx)("op: %s\n", opKindGetStr(node->op));

    if (node->dt)
        printer_outf(ctx)("type: %s\n", typeGetStr(node->dt));

    ctx->depth--;
}

void printAST (const ast* node) {
    printerCtx ctx = {
        .depth = 0
    };

    printer(&ctx, node);
}
