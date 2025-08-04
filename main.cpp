#include <iostream>
#include <cassert>
#include <climits>
#include "lexer.h"
#include <variant>
using u8 = unsigned char;
static_assert(sizeof(u8) == 1);
static_assert(CHAR_BIT == 8);

template <typename... Ts>
struct Variant : public variant<Ts...>
{
    using variant<Ts...>::variant; // <-- inherit contrsutcors
    template <typename... Fn>
    struct Fns : public Fn...
    {
        using Fn::operator()...;
    };
    template <typename... Fn>
    auto operator()(Fn &&...fn)
    {
        return visit(Fns{std::forward<Fns>(fn)...}, *this);
    }
};
static u8 test_case[] =
    {
#embed "temp.txt"
        , 0};

static std::string tok2str(int tok)
{
    std::string s;
    for (int i = sizeof(int) - 1; i > -1; --i)
    {
        if (((char *)&tok)[i])
        {
            s += (((char *)&tok)[i]);
        }
    }
    return s;
}

int main(void)
{
    Lexer lex{test_case};
    lex.init_keywords();

    while (*lex)
    {
        if (*lex == 'int')
        {
            std::cerr << lex.get_value() << "\n";
        }
        else if (*lex == 'id')
        {
            std::cerr << lex.ids[lex.get_value()] << "\n";
        }
        else
        {
            std::cerr << tok2str(*lex) << "\n";
        }
        ++lex;
    }
    return 0;
}

struct Parser
{
    Parser(const char *text) : lexer{text}
    {
    }
    Program operator()({return parse_program(){
        ABORT("TODO")}});

private:
    Lexer lexer;
};

struct MyStruct
{
};
Variant<int, double> v = 5.7;

v(
    [&](int x)
    { std::cerr << "int: " << x << "\n"; },
    [&](double x)
    { std::cerr << "double: " << x << "\n"; });
