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
    putchar(' ');

    for (int i = 0; i < ctx->depth; i++)
        printf("  ");

    return printf;
}

static void printer (printerCtx* ctx, const ast* node);

/*==== ====*/

static void printChildren (printerCtx* ctx, const ast* node) {
    for (int i = 0; i < node->children.length; i++) {
        ast* child = vectorGet(node->children, i);
        printer(ctx, child);
    }
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
    case astStrLit:
        printer_outf(ctx)("\"%s\"\n", node->literal.str);
        break;

    case astFileLit:
        printer_outf(ctx)("file: %s\n", node->literal.str);
        break;

    case astSymbol:
        printer_outf(ctx)("\"%s\"\n",   !node->symbol ? "<no symbol attached>"
                                      : !node->symbol->name ? "<unnamed symbol>"
                                      : node->symbol->name);
        break;

    default:
        break;
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
