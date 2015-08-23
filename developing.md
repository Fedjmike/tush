Developer's guide
=================

Source structure
----------------

`sh.c`: Contains `main()` and the high-level shell stuff: compilation, REPL.

---

Modules which compiler implement passes:

`lexer.h`: The lexer and inline implementations, which turns a string of code into a list of tokens. These are consumed by the parser.

`parser*.[ch]`: The parser, which takes a lexer holding the program and gives a Abstract Syntax Tree (AST).

`analyzer.[ch]`: The semantic analyzer, which adds `type` information to the AST and checks the semantics of the given program.

`runner.[ch]`: The runner, which takes a program in the form of a typed AST and interprets it, returning a runtime `value`.

`display.[ch]`: Prints user-friendly representations of a `value`, using its `type`. Tables, grids etc.

---

Language data structures:

`token.h` / struct `token`: A single token.

`ast*.[ch]` / struct `ast`: The Abstract Syntax Tree (AST), a structured representation of the code given to the compiler.

`type*.[ch]` / struct `type`: The interface for describing language datatypes.

`sym.[ch]` / struct `sym`: The symbol table. Symbols are the named objects (variables, functions etc.) accessible in the language.

`value.[ch]` / struct `value`: The runtime values produced by programs. Allocated with the Boehm garbage collector.

---

Functions for:

`paths.[ch]`: Handling paths. For example: getting an absolute path, performing tilde contraction (`/home/user/<...>` to `~/<...>`) and others.

`invoke.[ch]`: Invoking external programs.

`terminal.[ch]`:  Controlling to the terminal output.

---

Miscellaneous:

`builtins.[ch]`: Some built-in Tush functions.

`dirctx.h`: A structure, `dirCtx`, which tracks the context the shell is currently in: search paths and the working directory.

`forward.h`: Forward declarations of a bunch of types.

`common.h`: Very miscellaneous definitions.

Code standards
--------------

To be written.
