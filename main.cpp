#include <iostream>
#include <cassert>
#include <climits>
#include "lexer.h"

using u8 = unsigned char;
static_assert(sizeof(u8) == 1);
static_assert(CHAR_BIT == 8);

static u8 test_case[] =
    {
#embed "temp.txt"
        , 0};

int main(void)
{
    Lexer lex{test_case};
    while (*lex)
    {
        std::cerr << (char)*lex << std::endl;
        ++lex;
    }
    return 0;
}