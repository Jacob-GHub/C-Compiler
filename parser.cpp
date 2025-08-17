#include "parser.h"

// turns {...} section of code into a block for cfg
Block Parser::parse_block()
{
    expect('{');
    vector<Stmt> body;
    while (*lex && *lex != '}')
    {
        body.push_back(parse_stmt());
    }
    expect('}');
    return Block{move(body)};
}
//
Program Parser::parse_program()
{
    Program p;
    while (*lex)
    {
        p.body.push_back(parse_stmt());
    }
    return p;
}

Stmt Parser::parse_stmt()
{
    switch (*lex)
    {
    case 'brk':
    {
        ++lex;
        expect(';');
        // std::cerr << "DEBUG: Returning from parse_stmt with next token: "
        //           << tok2str(*lex) << "\n";

        return Break{};
    }
    break;
    case 'cont':
    {
        ++lex;
        expect(';');
        // std::cerr << "DEBUG: Returning from parse_stmt with next token: "
        //           << tok2str(*lex) << "\n";

        return Continue{};
    }
    break;
    case 'loop':
    {
        // std::cerr << "found loop: "
        //           << tok2str(*lex) << "\n";
        ++lex;
        auto blk = parse_block();
        return Loop{move(blk)};
    }
    case 'if':
    {
        ++lex;
        Expr cond = parse_expr();
        Block body = parse_block();
        optional<Block> else_body;
        if (*lex == 'else')
        {
            ++lex;
            else_body = parse_block();
        }
        // std::cerr << "DEBUG: Returning from parse_stmt with next token: "
        //           << tok2str(*lex) << "\n";

        return If{{move(cond)}, move(body), move(else_body)};
    }
    break;
    case '{':
    {
        // std::cerr << "DEBUG: Returning from parse_stmt with next token: "
        //           << tok2str(*lex) << "\n";

        return parse_block();
    }
    break;
    case ';':
    {
        // std::cerr << "DEBUG: Returning from parse_stmt with next token: "
        //           << tok2str(*lex) << "\n";

        ++lex;
        return Nop{};
    }
    break;
    case 'let':
    {
        ++lex;
        return Let{parse_variable()};
    }
    break;
    default:
    {
        auto lhs = parse_expr();
        if (*lex == '=')
        {
            ++lex;
            auto rhs = parse_expr();
            expect(';');
            return Assign{move(lhs), move(rhs)};
        }
        expect(';');
        return lhs;
    }
    break;
    }
}

Expr Parser::parse_expr()
{
    return parse_or();
}

i32 Parser::expect(int token)
{
    if (token != *lex) [[Unlikely]]
    {
        std::cerr << "Expected token: " << tok2str(token)
                  << ", got: " << tok2str(*lex);
        ABORT("bruh");
    }
    i32 res = lex.get_value();
    ++lex;
    return res;
}

Expr Parser::parse_or()
{
    Expr lhs = parse_and();
    while (*lex == '||')
    {
        ++lex;
        Expr rhs = parse_and();
        // lhs = Expr{MathOp{'||', {std::move(lhs), std::move(rhs)}}};
        lhs = MathOp{'||', {move(lhs), move(rhs)}};
    }
    return lhs;
}

Expr Parser::parse_rel()
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
            // lhs = Expr{MathOp{op, {std::move(lhs), std::move(rhs)}}};
            lhs = MathOp{op, {move(lhs), move(rhs)}};
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
Expr Parser::parse_unary()
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
Expr Parser::parse_primary()
{
    if ('(' == *lex)
    {
        ++lex;
        Expr res = parse_expr();
        expect(')');
        return res;
    }
    if ('id' == *lex)
    {
        return parse_variable();
    }
    return IntLiteral{expect('int')};
}
Expr Parser::parse_add()
{
    Expr lhs = parse_mul();
    while (1)
    {
        switch (*lex)
        {
        case '+':
        case '-':
        case '^':
        case '|':
        {
            // std::cerr << "Parsing operator " << (char)*lex << "\n";
            int op = *lex;
            ++lex;
            Expr rhs = parse_mul();
            lhs = MathOp{op, {std::move(lhs), std::move(rhs)}};
            break;
        }
        default:
            return lhs;
        }
    }
}

Expr Parser::parse_mul()
{
    Expr lhs = parse_unary();
    while (1)
    {
        switch (*lex)
        {
        case '*':
        case '/':
        case '%':
        case '<<':
        case '>>':
        case '&':
        {
            // std::cerr << "[parse_mul] operator: " << tok2str(*lex) << "\n";
            int op = *lex;
            ++lex;
            Expr rhs = parse_unary();
            lhs = MathOp{op, {std::move(lhs), std::move(rhs)}};
            break;
        }
        default:
            // std::cerr << "[parse_mul] next token before return: " << tok2str(*lex) << "\n";
            return lhs;
        }
    }
}

Expr Parser::parse_and()
{
    Expr lhs = parse_rel();
    while (*lex == '&&')
    {
        ++lex;
        Expr rhs = parse_rel();

        lhs = MathOp{'&&', {std::move(lhs), std::move(rhs)}};
    }
    return lhs;
}
Expr Parser::parse_variable()
{
    i32 name = expect('id');
    if (*lex == '[')
    {
        ++lex;
        auto size = parse_expr();
        expect(']');
        return Array{name, {move(size)}};
    }
    return Variable{name};
}