#include "lexer.h"
#include "lut.h"

int Lexer::operator*() { return head; }

void Lexer::operator++() { head = lex(); }

int Lexer::lex()
{
    static constexpr LUT is_ws(" \n\t\r\v\f");
    while (is_ws(*it))
    {
        it++;
    }

    switch (*it)
    {
    case '/':
    {
        if (it[1] == '/')
        {
            return lex_comment();
        }
        else
        {
            return *it++;
        }
        break;
    default:
    {
        return *it++;
    }
    break;
    }
    }
    if (*it == '\0')
        return -1;
    return *it++;
}

int Lexer::lex_comment()
{
    it += 2;
    static constexpr LUT is_new_line("\n", true);
    while (!is_new_line(*it))
    {
        it++;
    }
    return lex();
}
