# C-Compiler

A compiler for a subset of C, written from scratch in C++. It takes C source
code and runs it through the full classic compiler pipeline: lexing, parsing,
AST construction, and code generation to produce LLVM IR.

Built to understand how a high-level language actually becomes something a
machine can run, without leaning on a compiler framework.

---

## Features

- **Full front-to-back pipeline** source code in, target binaries out.
- **Lexer** that tokenizes C source into a typed token stream.
- **Recursive-descent parser** that builds an abstract syntax tree (AST).
- **Code generation** that walks the AST and emits LLVM IR.
- **Language support:** `int`, `var` types, functions, `if`/`else`, `while`/`for` loops, arithmetic and logical
  operators, local variables, recursion.
- **Tested against 30+ programs** with GoogleTest, and checked for memory leaks
  with Valgrind.

## Tech stack

| Area | Tooling |
| --- | --- |
| Language | C++ 17 |
| Build | [CMake / Make] |
| Testing | GoogleTest |
| Memory checking | Valgrind |
| Target output | LLVM IR |

## How it works

The compiler runs source through four stages:

```
  C source
     │
     ▼
 ┌─────────┐    tokens     ┌─────────┐     AST      ┌──────────────┐   LLVM IR
 │  Lexer  │ ───────────▶  │ Parser  │ ──────────▶  │  Code Gen    │ ──────────▶
 └─────────┘               └─────────┘              └──────────────┘
```

1. **Lexer** — scans the raw text and groups characters into tokens (keywords,
   identifiers, literals, operators, punctuation).
2. **Parser** — consumes the token stream with recursive descent and builds an
   AST that captures the program's structure (expressions, statements,
   functions).
3. **Semantic analysis** — Checks proper scope and type checking.
4. **Code generation** — walks the AST and emits LLVM IR, which assembles with
   `gcc`.

## Build

```bash
# with CMake
git clone https://github.com/Jacob-GHub/C-Compiler.git
cd C-Compiler
cmake -B build
cmake --build build
```


## License

[MIT — add a LICENSE file so the badge and the repo's About section show it.]
