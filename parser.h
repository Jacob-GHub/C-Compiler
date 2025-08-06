
#include "includes.h"

template <class... Variants>
struct Var : public variant<Variants>
{
    using std::variant<Variants>::variant;
    template <class... Ts>
    struct overloaded : Ts...

    {
        using Ts::operator()...;
    };
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    template <typename... Visitors>
    auto operator()(Visitors &&...v) -> decltype(std::visit(overloaded{std::forward<Visitors>(v)...}, *this))
    {
        return std::visit(overloaded{std::forward<Visitors>(v)...}, *this);
    }
};

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

struct Stmt
{
    Expr body;
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
    Program parse_program()
    {
        Program p;
        while (*lex)
        {
            p.body.push_back(parse_stmt());
        }
        return p;
    }

    Stmt parse_stmt()
    {
        Stmt s;
        s.body = parse_expr();
        expect(';');
        return s;
    }

    Expr parse_expr()
    {
        return parse_or();
    }

    i32 expect(int token)
    {
        if (token != *lex) [[Unlikely]]
        {
            ABORT("bruh");
        }
        return lex.get_value();
    }

    Expr parse_or()
    {
        Expr lhs = parse_and();
        while (*lex == '||')
        {
            ++lex;
            Expr rhs = parse_and();
            // lhs = MathOp{'||', {move(lhs), move(rhs)}};
            lhs = Expr{MathOp{'||', {std::move(lhs), std::move(rhs)}}};
        }
        return lhs;
    }
    Expr parse_rel()
    {
        Expr lhs = parse_add();
        while (1)
        {
            switch (*lex)
            {
            case '<':
            case '>':
            case '<=':
            case '>=':
            case '==':
            case '!=':
            {
                int op = *lex;
                ++lex;
                Expr rhs = parse_add();
                lhs = MathOp(op, {move(lhs), move(rhs)});
            }
            break;
            default:
            {
                return lhs;
            }
            break;
            }
        }
    }
    Expr parse_unary()
    {
        while (1)
        {
            switch (*lex)
            {
            case '+':
            case '-':
            case '!':
            case '~':
            {
                int op = *lex;
                ++lex;
                return MathOp{op, {parse_unary()}};
            }
            break;
            default:
            {
                return parse_primary();
            }
            break;
            }
        }
    }
    Expr parse_primary()
    {
        if ('(' == *lex)
        {
            ++lex;
            Expr res = parse_expr();
            expect(')');
            return res;
        }
        return IntLiteral(expect('int'));
    }
    Expr parse_add()
    {
        Expr lhs = parse_unary();
        while (*lex == '+' || *lex == '-')
        {
            int op = *lex;
            ++lex;
            Expr rhs = parse_unary();
            lhs = MathOp{op, {std::move(lhs), std::move(rhs)}};
        }
        return lhs;
    }
    Expr parse_and()
    {
        Expr lhs = parse_rel();
        while (*lex == '&') // Adjust this if you support '&&' as a token
        {
            ++lex;
            Expr rhs = parse_rel();
            lhs = MathOp{'&', {std::move(lhs), std::move(rhs)}};
        }
        return lhs;
    }
};
