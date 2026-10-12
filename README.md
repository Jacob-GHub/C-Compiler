# C Compiler

A compiler for a subset of C, written from scratch in C++. It takes C source and
runs it through the full classic pipeline — lexing, parsing, AST construction,
and LLVM IR code generation — all the way down to a native binary.

Built to understand how a high-level language actually becomes something a machine
can run, using LLVM for the backend so the focus stays on the front end and code
generation.

---

## Features

- **Full source-to-binary pipeline** — C source in, native executable out.
- **Pull-based lexer** that tokenizes on demand and resolves keywords against an
  identifier table before the parser ever sees a token.
- **Recursive-descent parser** implementing a complete operator-precedence
  hierarchy, one function per precedence level.
- **`std::variant`-based AST** with visitor-pattern traversal — a closed set of
  node types with value semantics, so a missing case is a compile error instead
  of a runtime surprise.
- **LLVM IR code generation** via the IRBuilder API, walking the AST top-down and
  building `Value*`s bottom-up.
- **Control flow** — conditionals and loops lowered into explicit LLVM basic
  blocks with branch terminators and break/continue target stacks.
- **Short-circuit `&&` and `||`** generated with PHI nodes, skipping the right
  operand once the result is decided.
- **Scoped symbol table** — a scope stack bridging declarations to later
  references, with a bidirectional string-interning table that reduces identifier
  lookup to constant time.
- **Variables in memory** — locals mapped onto LLVM `alloca` and `GEP` addressing,
  separating stack arrays from decayed pointers.
- **Tested** against 30+ programs with GoogleTest, with Valgrind gating memory
  safety on Linux builds.

## Tech stack

| Area | Tooling |
| --- | --- |
| Language | C++17 (`std::variant` AST) |
| Backend | LLVM (IRBuilder API) |
| Build | CMake |
| Testing | GoogleTest |
| Memory checking | Valgrind |
| Platform | Linux |

## How it works

```
  C source
     │
     ▼
 ┌─────────┐   tokens    ┌──────────┐    AST     ┌───────────┐   LLVM IR   ┌────────┐  native
 │  Lexer  │ ─────────▶  │  Parser  │ ────────▶  │  Code Gen │ ─────────▶  │  LLVM  │ ────────▶
 └─────────┘             └──────────┘            └───────────┘             └────────┘  binary
```

1. **Lexer** — a pull-based token stream the parser drives with peek and advance.
   Keywords are disambiguated from identifiers here, before parsing.
2. **Parser** — recursive descent, with each precedence level parsing the level
   below it and looping on its own operators. Produces an owned AST as nested
   `std::variant` values (no heap-linked node graph).
3. **Code generation** — a recursive walk (`gen_prog` → `gen_function` →
   `gen_stmt` → `gen_expr`) that mirrors the AST shape and emits LLVM IR through
   the IRBuilder. The symbol table connects declarations to the loads and stores
   that reference them.
4. **LLVM** — takes the generated IR the rest of the way to a native binary.

> Note: the pipeline emits IR directly; it does not run optimization passes.

## Build


```bash
git clone https://github.com/Jacob-GHub/C-Compiler.git
cd C-Compiler
cmake -B build
cmake --build build
```

Requires LLVM installed and discoverable by CMake (`find_package(LLVM)`).

## Usage

```bash
# Compile a C file down to a native binary
./build/ccompiler path/to/program.c -o program    
./program
```

## Running the tests

```bash
cd build
ctest        
```

30+ test programs run under GoogleTest, with Valgrind confirming leak-free
execution on Linux.

## What I learned / possible next steps

- Using `std::variant` plus `std::visit` for the AST means forgetting to handle a
  node type fails at compile time, not at runtime.
- Generating short-circuit `&&`/`||` meant learning how LLVM encodes control flow,
  since the IR has no native short-circuit operators — hence the basic blocks and
  PHI nodes.
- Possible extensions: more of the C standard, better diagnostics with
  line/column info, or an optimization pass over the generated IR.

## License

[MIT]
