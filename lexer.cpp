#include "lexer.h"
#include "lut.h"
#include <iostream>
#include <cstdarg>

#define ABORT(...)                                  \
    do                                              \
    {                                               \
        std::cerr << "D: ";                         \
        std::cerr << __VA_ARGS__;                   \
        std::cerr << " , Line: " << __LINE__        \
                  << " File: " << __FILE__ << "\n"; \
        __builtin_trap();                           \
    } while (0)

IDManager Lexer::ids;

int Lexer::lex()
{
    init_keywords();
    static constexpr LUT is_ws(" \n\t\r\v\f");
    while (is_ws(*it))
    {
        line += *it++ == '\n';
    }

    static constexpr LUT is_mono(";~^*%(){}[]+-,", true);
    if (is_mono(*it))
    {
        return *it++;
    }

    static constexpr LUT is_digit("0123456789");
    if (is_digit(*it))
    {
        std::cerr << "Lexing int starting with: " << *it << "\n";
        value = 0;
        while (is_digit(*it))
        {
            if (__builtin_mul_overflow(value, 10, &value))
            {
                ABORT("D");
            }
            if (__builtin_add_overflow(value, *it++ - '0', &value))
            {
                ABORT("D:<");
            }
            std::cerr << "Lexing the entire: " << *it << "\n";
        }
        return 'int';
    }

    static constexpr LUT is_id("abcdefghijklmnopqrstuvqxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_");
    static constexpr LUT still_id("abcdefghijklmnopqrstuvqxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

    if (is_id(*it))
    {
        std::cerr << "Lexing id starting with: " << *it << "\n";

        u8 *start = it;
        while (still_id(*it))
        {
            it++;
        }
        u8 *end = it;
        value = ids.lookup(start, end);
        std::cerr << "value: " << value << "\n";
        if (value < keywords.size() && keywords[value] > -1)
        {
            std::cout << "keyword:" << keywords[value];
            return keywords[value];
        }
        std::cout << "identifier found";
        return 'id';
    }
    switch (*it)
    {
        std::cerr << "Lexing switch case starting with: " << *it << "\n";
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
    }
    case '|':
    {
        if (it[1] == '|')
        {
            it += 2;
            return '||';
        }
        else
        {
            return *it++;
        }
        break;
    }
    case '&':
    {
        if (it[1] == '&')
        {
            it += 2;
            return '&&';
        }
        else
        {
            return *it++;
        }
        break;
    }
    case '=':
    {
        if (it[1] == '=')
        {
            it += 2;
            return '==';
        }
        else
        {
            return *it++;
        }
        break;
    }
    case '!':
    {
        if (it[1] == '=')
        {
            it += 2;
            return '!=';
        }
        else
        {
            return *it++;
        }
        break;
    }
    case '<':
    {
        if (it[1] == '<')
        {
            it += 2;
            return '<<';
        }
        if (it[1] == '=')
        {
            it += 2;
            return '<=';
        }
        else
        {
            return *it++;
        }
        break;
    }
    case '>':
    {
        if (it[1] == '>')
        {
            it += 2;
            return '>>';
        }
        if (it[1] == '=')
        {
            it += 2;
            return '>=';
        }
        else
        {
            return *it++;
        }
        break;
    }
    default:
    {
        ABORT("Lexer line: " << line << (char)*it << " ," << (int)*it << " ?\n");
    }
    break;
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

void Lexer::init_keywords()
{
    std::cerr << "initing keywords" << "\n";

    auto init_keyword = [&](std::string_view name, int value)
    {
        i32 id = ids[name];
        if (keywords.size() <= id)
        {
            keywords.resize(id + 1, -1);
        }
        keywords[id] = value;
    };
    init_keyword("let", 'let');
    init_keyword("break", 'brk');
    init_keyword("continue", 'cont');
    init_keyword("return", 'ret');
    init_keyword("loop", 'loop');
    init_keyword("if", 'if');
    init_keyword("else", 'else');

    std::cerr << "=== Keyword table ===\n";
    for (size_t i = 0; i < keywords.size(); ++i)
    {
        std::cerr << i << ": " << keywords[i] << "\n";
    }
}