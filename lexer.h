// LEXER.h
#ifndef LEXER_H
#define LEXER_H
#include <cstdint>
#include "idmanager.h"
using u8 = uint8_t;
using i32 = int32_t;

struct Lexer
{
    Lexer(u8 *it_) : it(it_) { head = lex(); }
    int operator*() { return head; }
    void operator++() { head = lex(); }
    i32 get_value() { return value; }
    void init_keywords();
    static IDManager ids;

private:
    int lex();
    int lex_comment();
    u8 *it;
    int head;
    size_t line = 0;
    i32 value;
    std::vector<i32> keywords;
};

#endif