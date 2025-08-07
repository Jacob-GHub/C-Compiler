#include <iostream>
#include <cassert>
#include <climits>
#include "lexer.h"
#include <vector>
#include <variant>
using i32 = int;
using namespace std;
using u8 = unsigned char;
static_assert(sizeof(u8) == 1);
static_assert(CHAR_BIT == 8);
#define ABORT(...)                                  \
    do                                              \
    {                                               \
        std::cerr << "D: ";                         \
        std::cerr << __VA_ARGS__;                   \
        std::cerr << " , Line: " << __LINE__        \
                  << " File: " << __FILE__ << "\n"; \
        __builtin_trap();                           \
    } while (0)
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/NoFolder.h"

// template <typename... Ts>
// struct Variant : public variant<Ts...>
// {
//     using variant<Ts...>::variant; // <-- inherit contrsutcors
//     template <typename... Fn>
//     struct Fns : public Fn...
//     {
//         using Fn::operator()...;
//     };
//     template <typename... Fn>
//     auto operator()(Fn &&...fn)
//     {
//         return visit(Fns{std::forward<Fns>(fn)...}, *this);
//     }
// };
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

//              STD::VARIANT
// std::variant lets a variable hold one value out of a fixed set of types
// basically think if we make a variant and pass in <int,std::string>
// then we can assign this variant to an int value or string value
// basically it makes it pythonic

//              TEMPLATES
// templates allow you to write generic code that works with any type without repeating yourself
// for example template <typename T>   <-- lets you do T add(T a, t b), where T is return type and param type
// it's kind of like doing using u8 = unsigned char, but its taying T can = any type

//              VISIT
// std::visit -> Takes a callable(like a lambda) and a std::variant
// from here it invokes the lambda with the current value inside the variant, based on its active type.

//              GOAL
// We want to use lambdas as a way to take in a node in our AST such as expr, num, etc
// That node will essentially be the argument to a lambda and then it will have the corresponding logic

// So we want to eventually be able to do this:
//  variant<int,double> v = (5)
// v( [&](int& x){ std::cout<<"int"<<endl; }, [&](double& s){ std::cout<<"string"<<endl; })
// so depending on what the variant is, we want a certain code to run

//

struct IntLiteral
{
    i32 body;
};
struct Expr; // forward declare

struct MathOp
{
    int op;
    vector<Expr> body;
    // MathOp(int op, vector<Expr> body) : op(op), body(std::move(body)) {}
};

template <class... Variants>
struct Var : public variant<Variants...>
{
    using std::variant<Variants...>::variant;
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
        i32 res = lex.get_value();
        ++lex;
        return res;
    }

    Expr parse_or()
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

    Expr parse_mul()
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
                int op = *lex;
                ++lex;
                Expr rhs = parse_unary();
                lhs = MathOp{op, {std::move(lhs), std::move(rhs)}};
                break;
            }
            default:
                return lhs;
            }
        }
    }

    Expr parse_and()
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
};
namespace llvm
{

    struct Compiler
    {
        Compiler(u8 *text) : prog(Parser{text}()),
                             ctx(),                //<-- the llvm context
                             mod("main.cpp", ctx), // <-- creares main.cpp module
                             builder(ctx)
        {
            mod.setTargetTriple(Triple(sys::getDefaultTargetTriple()).str());
            setup(); // declares functions like main and printf
            gen_prog(prog);
        }
        void gen_prog(Program &p)
        {
            for (auto &s : p.body)
            {
                gen_stmt(s);
            }
        }
        void 
        ~Compiler()
        {
            builder.CreateRet(builder.getInt32(0));
            if constexpr (true)
            {
                mod.print(outs(), 0);
            }
            if (verifyModule(mod, &errs()))
            {
                ABORT("MODULE VERIFICATION FAILED");
            }
            error_code error_opening_file;
            raw_fd_ostream file("main.bc", error_opening_file);
            if (error_opening_file)
            {
                ABORT("ERROR OPENING FILE" << error_opening_file);
            }
            WriteBitcodeToFile(mod, file);
        }

    private:
        Program prog;                                // <-- parsed representation of the input
        LLVMContext ctx;                             // <--- The llvm context (shared env for types,consts,etc)
        Module mod;                                  // <--The llvm IR module, the generated code unit
        IRBuilder<NoFolder> builder;                 // helps construct llvm instructions easily
        unordered_map<string, Function *> functions; //<-- map stores function names and their llvm representations
        void setup()
        {
            { // defining the function type for our IR that we are about to put IR code and BB's inside
                // returns an i32, has no variables, is not variadic (variable num of args)
                FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), {}, false);

                // creates the main function for our IR representation
                // sig is the function type we defined the previous line
                // external linkage means this code is visible globally
                // mod is the llvm module where this function is being inserted
                functions["main"] = Function::Create(sig, GlobalValue::ExternalLinkage, "main", mod);

                // creates a basic block inside the main function called entry
                // keep in mind that a basic block is A sequence of LLVM instructions with no control flow jumps in or out except at the end.
                // Every function needs at least one basic block
                // this is where you will actually emit code
                BasicBlock *block = BasicBlock::Create(ctx, "entry", functions["main"]);
                // tells the IRBuilder to start emitting instructions inside the "entry" block
                // this is because the builder doesn't know where to input instructions unless you specif
                builder.SetInsertPoint(block);
            }
            {
                // getPtrTy() -> means this function is expecting an i8* as an argument
                //  basically our printf is expecting to print a string of i8 chars
                FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), {builder.getPtrTy()}, true);
                functions["printf"] = Function::Create(sig, GlobalValue::ExternalLinkage, "printf", mod);
            }
        }
    };
}

int main(void)
{
    // Lexer lex{test_case};
    // lex.init_keywords();

    // while (*lex)
    // {
    //     if (*lex == 'int')
    //     {
    //         std::cerr << lex.get_value() << "\n";
    //     }
    //     else if (*lex == 'id')
    //     {
    //         std::cerr << lex.ids[lex.get_value()] << "\n";
    //     }
    //     else
    //     {
    //         std::cerr << tok2str(*lex) << "\n";
    //     }
    //     ++lex;
    // }
    // return 0;
    llvm::Compiler{(u8 *)test_case};
    // Program prog = Parser(test_case)();
    return 0;
}