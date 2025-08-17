#ifndef PARSER_H
#define PARSER_H

#include "includes.h"
#include "lexer.h"

template <class... Variants>

struct Var : public variant<Variants...>
{
    using std::variant<Variants...>::variant;
    template <class... Ts>
    struct overloaded : Ts...

    {
        using Ts::operator()...;
    };

    template <typename... Visitors>
    auto operator()(Visitors &&...v)
    {
        return std::visit(overloaded{std::forward<Visitors>(v)...}, *this);
    }
};

struct Expr;
struct Stmt;

struct IntLiteral
{
    i32 body;
};

struct MathOp
{
    int op;
    vector<Expr> body;
};

struct Expr : public Var<IntLiteral, MathOp>
{
    using Var<IntLiteral, MathOp>::Var;
};

struct Block
{
    vector<Stmt> body;
};

struct Break
{
};

struct Continue
{
};

struct Loop
{
    Block body;
};

struct While
{
    vector<Expr> cond;
    Block body;
};

struct If
{
    vector<Expr> cond;
    Block body;
    optional<Block> else_body;
};

struct Nop // when walking the AST this allows us to avoid finding null, we instead find Nop
{
};

struct Stmt : public Var<Block, Break, Continue, Loop, If, Nop, Expr> // stmt is a variant inehriting from our Var which lets us do operator overloading
{
    using Var<Block, Break, Continue, Loop, If, Nop, Expr>::Var;
};

struct Program
{
    vector<Stmt> body;
};

struct Parser
{
    Parser(u8 *test_case) : lex{test_case} {}
    Program operator()() { return parse_program(); }

private:
    Lexer lex;
    Block parse_block();
    Program parse_program();
    Stmt parse_stmt();
    Expr parse_expr();
    i32 expect(int token);
    Expr parse_or();
    Expr parse_rel();
    Expr parse_unary();
    Expr parse_primary();
    Expr parse_add();
    Expr parse_mul();
    Expr parse_and();
};

#endif