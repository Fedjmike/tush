#include "ast-printer.h"

#include <stdio.h>

#include "type.h"
#include "ast.h"
#include "sym.h"

typedef struct printerCtx {
    int depth;
} printerCtx;

static printf_t* printer_outf (printerCtx* ctx) {
    putchar(' ');

    for (int i = 0; i < ctx->depth; i++)
        printf("  ");

    return printf;
}

static void printer (printerCtx* ctx, const ast* node);

static void printFnApp (printerCtx* ctx, const ast* node) {
    printer_outf(ctx)("arg(s):\n");

    for (int i = 0; i < node->children.length; i++) {
        ast* arg = vectorGet(node->children, i);
        printer(ctx, arg);
    }

    printer_outf(ctx)("fn:\n");
    printer(ctx, node->l);
}

static void printStrLit (printerCtx* ctx, const ast* node) {
    printer_outf(ctx)("\"%s\"\n", node->literal.str);
}

static void printSymbolLit (printerCtx* ctx, const ast* node) {
    printer_outf(ctx)("\"%s\"\n",   !node->literal.symbol ? "<no symbol attached>"
                                  : !node->literal.symbol->name ? "<unnamed symbol>"
                                  : node->literal.symbol->name);
}

static void printer (printerCtx* ctx, const ast* node) {
    printer_outf(ctx)("+ %s\n", astKindGetStr(node->kind));
    ctx->depth++;

    (void (*[astKindNo])(printerCtx*, const ast*)) {
        [astFnApp] = printFnApp,
        [astStrLit] = printStrLit,
        [astSymbolLit] = printSymbolLit
    }[node->kind](ctx, node);

    if (node->dt)
        printer_outf(ctx)("dt: %s\n", typeGetStr(node->dt));

    ctx->depth--;
}

void printAST (const ast* node) {
    printerCtx ctx = {
        .depth = 0
    };

    printer(&ctx, node);
}
