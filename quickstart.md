Basic Expressions
-----------------

```haskell
"hello world" :: Str
true :: Bool
false :: Bool
5 :: Int
```

Barewords:

```haskell
$ five
five :: File
$ let five = 5
$ five
5 :: Int
$ five.*
[] :: [File]
```

- A bareword is a file literal, unless it has been declared as a variable with `let`.
- A bareword that contains wildcards is a glob literal, typed as a list of files.

Function literals:

```haskell
$ let f = \x y -> y
$ :type f
'a -> 'b -> 'b
```

- To be written.

The four containers: tuples, lists, records and dictionaries
------------------------------------------------------------

```haskell
$ (1, [])
(1, []) :: (Int, ['a])
```

               |  Heterogeneous  |  Variable length  
--------------:|:---------------:|:-----------------:
 Integer keys  |      Tuple      |       List        
 Complex keys  |     Record*     |    Dictionary*    

Why isn't there a heterogeneous, variable length container? That would require runtime type information, which is currently beyond the scope of Tush.

\* Not implemented yet.

Function/program invocation
---------------------------

To be written.

Operators
---------

```haskell
(++) :: ['a] -> ['a] -> ['a]
```

- Joins two lists. Strings are not (yet) lists.

```haskell
(|), (|:)
```

- To be written.

Variables
---------

```haskell
let <var> = <expr>
```

`let` defines a variable, shadowing any previous variables of the same name.

Some built-in functions
-----------------------

```haskell
size :: File -> Int
```

- Returns the size, in bytes, of a file.

---

```haskell
zipf :: ('a -> 'b) -> 'b -> ('a, 'b)
```

- Zips up a function result with the argument the produced it.
- An example:
```haskell
$ let sz = size zipf
$ type.h sz
(2434, type.h) :: (Int, File)
$ type* | sz
    848  type-internal.h
   9693  type-unify.c   
  11546  type.c         
   2434  type.h         
 :: [(Int, File)]
```

---

```haskell
sort :: [(Int, 'b)] -> [(Int, 'b)]
```

- Sorts a table by its first column in ascending order.
- Currently it only sorts on `Int`. In the future it will be more general.
- Continuing the example above:
```haskell
$ type* | sz | sort
    848  type-internal.h
   2434  type.h         
   9693  type-unify.c   
  11546  type.c         
 :: [(Int, File)]
```

---

```haskell
sum :: [Int] -> Int
```

- Sums a list.

```haskell
$ type* | size | sum
24521 :: Int
```

REPL commands
-------------

```
:type <expr>
:ast <expr>
:cd <dir>
```

These are special commands available from the prompt, all of which take expressions. They are not part of the language and therefore can't be used within other expressions.

- `:type` displays the type of an expression without evaluating it.
- `:ast` displays the Abstract Syntax Tree of an expression with types, again without evaluating it.
- `:cd` changes the working directory to the the result of the expression given, which must be of type `File`. It does not evaluate it if otherwise.

`cd` is not part of the language because it's the directory equivalent of `goto`. The only reason to indefinitely enter a directory is when at the prompt, where one might not know how long they want to stay there. See `into` below for a structured way to change directory.

A word on tokens
----------------

```
$ [*.[ch]]
main.c  main.h
 :: [File]
```

Note how the outer brackets are tokens of their own, opening and closing a list, while the inner brackets are part of a glob. After the opening bracket, the lexer waits for a closing bracket before moving onto the next token.

The basic rules for tokens are:

- Whitespace, parentheses (`()`) and backticks (`` ` ``) delimit universally.
- If a token starts with an opening bracket (`[{`) or `!` then it immediately ends there.
- Closing brackets (`]}`) are delimiters, unless they match a bracket in a glob.
- Commas are also delimiters, but will be escaped by an open glob bracket.
- Nothing else delimits.

This is designed to do the reasonable and intuitive thing.

- `[name1, name2, name3]` is a list of files, the commas and brackets are not part of the filenames.
- `*.{cpp,h}` is an alternation glob.

Not implemented yet
===================

Containers
----------

```
$ :type {field: <expr>}
{field: <type of expr>}
$ :type [key: value]
<type of key> <type of value> Dict
```

Control flow
------------

```
if <expr>:
elif <expr>:
else:
```
```
for <var>, <iterable>:
```
```
switch <expr>
case <pattern>:
```

Directory control
-----------------

```
into <dir>:
```
```
make-into <dir>:
```

Functions
---------

<!--```
\<args..> -> <expr>
```-->

```
<fn> <args..>:
    <stmts..>
```

Macros
------

```
<macro>! <args..>:
```
