// LEXER.h
#ifndef LEXER_H
#define LEXER_H
#include <cstdint>
using u8 = uint8_t;

struct Lexer
{
    Lexer(u8 *it_) : it(it_) { head = lex(); }
    int operator*();
    void operator++();

private:
    int lex();
    int lex_comment();
    u8 *it;
    int head;
};

#endif