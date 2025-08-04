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
        }
        return 'int';
    }

    static constexpr LUT is_id("abcdefghijklmnopqrstuvqxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_");
    static constexpr LUT still_id("abcdefghijklmnopqrstuvqxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

    if (is_id(*it))
    {
        u8 *start = it;
        while (still_id(*it))
        {
            it++;
        }
        u8 *end = it;
        value = ids.lookup(start, end);

        if (value < keywords.size() && keywords[value] > -1)
        {
            return keywords[value];
        }
        return 'id';
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
    auto init_keyword = [&](std::string_view name, int value)
    {
        i32 id = ids[name];
        while (keywords.size() <= id)
        {
            keywords.push_back(-1);
            keywords[id] = value;
        };
    };
    init_keyword("let", 'let');
    init_keyword("break", 'brk');
    init_keyword("continue", 'cont');
    init_keyword("return", 'ret');
    init_keyword("loop", 'loop');
    init_keyword("if", 'if');
    init_keyword("else", 'else');
}