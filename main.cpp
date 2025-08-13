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

    template <typename... Visitors>
    auto operator()(Visitors &&...v)
    {
        return std::visit(overloaded{std::forward<Visitors>(v)...}, *this);
    }
};
struct Expr : public Var<IntLiteral, MathOp>
{
    using Var<IntLiteral, MathOp>::Var;
};

struct Stmt;
struct Block
{
    vector<Stmt> body;
};
struct Break
{
};
struct Continue
{
};
struct Loop
{
    Block body;
};
struct If
{
    vector<Expr> cond;
    Block body;
    optional<Block> else_body;
};
struct Nop // when walking the AST this allows us to avoid finding null, we instead find Nop
{
};

struct Stmt : public Var<Block, Break, Continue, Loop, If, Nop, Expr> // stmt is a variant inehriting from our Var which lets us do operator overloading
{
    using Var<Block, Break, Continue, Loop, If, Nop, Expr>::Var;
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

    // turns {...} section of code into a block for cfg
    Block parse_block()
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
        switch (*lex)
        {
        case 'brk':
        {
            ++lex;
            expect(';');
            std::cerr << "DEBUG: Returning from parse_stmt with next token: "
                      << tok2str(*lex) << "\n";

            return Break{};
        }
        break;
        case 'cont':
        {
            ++lex;
            expect(';');
            std::cerr << "DEBUG: Returning from parse_stmt with next token: "
                      << tok2str(*lex) << "\n";

            return Continue{};
        }
        break;
        case 'loop':
        {
            std::cerr << "found loop: "
                      << tok2str(*lex) << "\n";
            ++lex;
            auto blk = parse_block();
            return Loop{move(blk)};
        }
        break;
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
            std::cerr << "DEBUG: Returning from parse_stmt with next token: "
                      << tok2str(*lex) << "\n";

            return If{{move(cond)}, move(body), move(else_body)};
        }
        break;
        case '{':
        {
            std::cerr << "DEBUG: Returning from parse_stmt with next token: "
                      << tok2str(*lex) << "\n";

            return parse_block();
        }
        break;
        case ';':
        {
            std::cerr << "DEBUG: Returning from parse_stmt with next token: "
                      << tok2str(*lex) << "\n";

            ++lex;
            return Nop{};
        }
        break;
        default:
        {
            std::cerr << "DEFAULT EXPR " << tok2str(*lex) << "\n";
            auto lhs = parse_expr();
            std::cerr << " token after expr: " << tok2str(*lex) << "\n";
            expect(';');
            return lhs;
        }
        break;
        }
    }

    Expr parse_expr()
    {
        return parse_or();
    }

    i32 expect(int token)
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
        return IntLiteral{expect('int')};
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
                std::cerr << "Parsing operator " << (char)*lex << "\n";
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
                std::cerr << "[parse_mul] operator: " << tok2str(*lex) << "\n";
                int op = *lex;
                ++lex;
                Expr rhs = parse_unary();
                lhs = MathOp{op, {std::move(lhs), std::move(rhs)}};
                break;
            }
            default:
                std::cerr << "[parse_mul] next token before return: " << tok2str(*lex) << "\n";
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
                             ctx(),                //<-- Creates an empty llvm::LLVMContext.
                                                   // This stores all LLVM IR "global" state for a single compilation unit.
                             mod("main.cpp", ctx), // <-- Creates an llvm::Module named "main.cpp" associated with that context.
                                                   // A module is like a container for all IR code, functions, and global variables.
                             builder(ctx)
        {
            llvm::Triple triple(sys::getDefaultTargetTriple());
            mod.setTargetTriple(triple.str());
            // mod.setTargetTriple(Triple(sys::getDefaultTargetTriple()));
            setup(); // declares functions like main and printf
            gen_prog(prog);
        }
        Value *i1toi32(Value *i1)
        {
            return builder.CreateZExt(i1, builder.getInt32Ty());
        }
        Value *i32toi1(Value *i32)
        {
            return builder.CreateICmpNE(i32, builder.getInt32(0));
        }
        void gen_prog(Program &p)
        {
            for (auto &s : p.body)
            {
                gen_stmt(s);
            }
        }
        void gen_stmt(Stmt &s)
        {
            if (builder.GetInsertBlock()->getTerminator())
            {
                return;
            }
            auto gen_block = [&](Block &block)
            {
                for (auto &s : block.body)
                {
                    gen_stmt(s);
                }
            };

            s(
                gen_block,
                [&](Break &stmt)
                {
                    if (!break_stack.size())
                    {
                        ABORT("BREAK_STACK EMPTY");
                    }
                    builder.CreateBr(break_stack.back());
                    return;
                },
                [&](Continue &stmt)
                {
                    if (!continue_stack.size())
                    {
                        ABORT("continue_STACK EMPTY");
                    }
                    builder.CreateBr(continue_stack.back());
                    return;
                },
                [&](Loop &stmt)
                {
                    BasicBlock *cur_block = builder.GetInsertBlock();
                    BasicBlock *loop_block = BasicBlock::Create(ctx, "", cur_block->getParent());
                    BasicBlock *merge_block = BasicBlock::Create(ctx, "", cur_block->getParent());

                    continue_stack.push_back(loop_block);
                    break_stack.push_back(merge_block);

                    builder.CreateBr(loop_block);
                    builder.SetInsertPoint(loop_block);
                    gen_block(stmt.body);

                    if (!builder.GetInsertBlock()->getTerminator())
                    {
                        builder.CreateBr(loop_block);
                    }
                    builder.SetInsertPoint(merge_block);

                    continue_stack.pop_back();
                    break_stack.pop_back();
                },
                [&](If &stmt)
                {
                    BasicBlock *cur_block = builder.GetInsertBlock();
                    BasicBlock *if_block = BasicBlock::Create(ctx, "", cur_block->getParent());
                    BasicBlock *else_block = BasicBlock::Create(ctx, "", cur_block->getParent());
                    BasicBlock *merge_block = BasicBlock::Create(ctx, "", cur_block->getParent());

                    Value *cond = i32toi1(gen_expr(stmt.cond[0]));
                    builder.CreateCondBr(cond, if_block, else_block);

                    builder.SetInsertPoint(if_block);
                    gen_block(stmt.body);
                    if (!builder.GetInsertBlock()->getTerminator())
                    {
                        builder.CreateBr(merge_block);
                    }
                    builder.SetInsertPoint(else_block);
                    if (stmt.else_body)
                    {
                        gen_block(*stmt.else_body);
                    }
                    if (!builder.GetInsertBlock()->getTerminator())
                    {
                        builder.CreateBr(merge_block);
                    }
                    builder.SetInsertPoint(merge_block);
                },
                [&](Nop &stmt) {},
                [&](Expr &expr)
                {
                    //      CreateCall takes (callee (func*), args to the func (Value*), calltmp - optional name for the result)
                    builder.CreateCall(functions["printf"], {get_fmt("%d\n"), gen_expr(expr)});
                });
        }
        Value *gen_expr(Expr &e)
        {
            return e(
                [&](IntLiteral &lit) -> Value *
                { return builder.getInt32(lit.body); },
                [&](MathOp &op) -> Value *
                {
                    if (op.op == '||' || op.op == '&&')
                    {

                        Value *lhs = gen_expr(op.body[0]);                                                       // gen left op code recursively
                        BasicBlock *current_block = builder.GetInsertBlock();                                    // current position in IR
                        BasicBlock *second_eval_block = BasicBlock::Create(ctx, "", current_block->getParent()); // code evaling rhs
                        BasicBlock *merge_block = BasicBlock::Create(ctx, "", current_block->getParent());       // code where both branches meet again

                        Value *lhs_as_i1 = i32toi1(lhs); // llvms bool type is i1, thus we switch to i1 from our lhs value
                        if (op.op == '||')
                        {
                            builder.CreateCondBr(lhs_as_i1, merge_block, second_eval_block);
                            // if lhs_as_i1 is true -> first arg -> jump to merge_block
                            // else jump to second_eval_block
                        }
                        else
                        {
                            builder.CreateCondBr(lhs_as_i1, second_eval_block, merge_block);
                        }

                        builder.SetInsertPoint(second_eval_block); // any new instr's will go in 2nd eval block
                        Value *rhs_as_i1 = i32toi1(gen_expr(op.body[1]));
                        second_eval_block = builder.GetInsertBlock();
                        builder.CreateBr(merge_block); // once we're done with rhs jump to merge_block

                        builder.SetInsertPoint(merge_block);
                        PHINode *phi = builder.CreatePHI(builder.getInt1Ty(), 2); // this value could coem from any path, choose the right one
                        phi->addIncoming(lhs_as_i1, current_block);               // if control came from curr_block ->lhs_as_i1
                        phi->addIncoming(rhs_as_i1, second_eval_block);

                        return i1toi32(phi);
                    }
                    vector<Value *> args;
                    for (Expr &ex : op.body)
                    {
                        args.push_back(gen_expr(ex));
                    }

                    switch (op.op)
                    {
                    case '~':
                        if (args.size() < 1)
                            ABORT("Operator '~' expects 1 operand.");
                        return builder.CreateNot(args[0]);

                    case '!':
                        if (args.size() < 1)
                            ABORT("Operator '!' expects 1 operands.");
                        return i1toi32(builder.CreateICmpEQ(args[0], builder.getInt32(0)));

                    case '<<':
                        if (args.size() < 2)
                            ABORT("Operator '<<' expects 2 operands.");
                        return builder.CreateShl(args[0], args[1]);

                    case '>>':
                        if (args.size() < 2)
                            ABORT("Operator '>>' expects 2 operands.");
                        return builder.CreateAShr(args[0], args[1]);

                    case '&':
                        if (args.size() < 2)
                            ABORT("Operator '&' expects 2 operands.");
                        return builder.CreateAnd(args[0], args[1]);

                    case '*':
                        if (args.size() < 2)
                            ABORT("Operator '*' expects 2 operands.");
                        return builder.CreateMul(args[0], args[1]);

                    case '/':
                        if (args.size() < 2)
                            ABORT("Operator '/' expects 2 operands.");
                        return builder.CreateSDiv(args[0], args[1]);

                    case '%':
                        if (args.size() < 2)
                            ABORT("Operator '%' expects 2 operands.");
                        return builder.CreateSRem(args[0], args[1]);

                    case '+':
                        if (args.size() == 1)
                        {
                            return args[0];
                        }
                        else if (args.size() >= 2)
                        {
                            return builder.CreateAdd(args[0], args[1]);
                        }
                        else
                        {
                            ABORT("Operator '+' expects 1 or 2 operands.");
                        }

                    case '-':
                        if (args.size() == 1)
                        {
                            return builder.CreateSub(builder.getInt32(0), args[0]);
                        }
                        else if (args.size() >= 2)
                        {
                            return builder.CreateSub(args[0], args[1]);
                        }
                        else
                        {
                            ABORT("Operator '-' expects 1 or 2 operands.");
                        }

                    case '^':
                        if (args.size() < 2)
                            ABORT("Operator '^' expects 2 operands.");
                        return builder.CreateXor(args[0], args[1]);

                    case '|':
                        if (args.size() < 2)
                            ABORT("Operator '|' expects 2 operands.");
                        return builder.CreateOr(args[0], args[1]);

                    case '>':
                        if (args.size() < 2)
                            ABORT("Operator '>' expects 2 operands.");
                        return i1toi32(builder.CreateICmpSGT(args[0], args[1]));

                    case '<':
                        if (args.size() < 2)
                            ABORT("Operator '<' expects 2 operands.");
                        return i1toi32(builder.CreateICmpSLT(args[0], args[1]));

                    case '<=':
                        if (args.size() < 2)
                            ABORT("Operator '<=' expects 2 operands.");
                        return i1toi32(builder.CreateICmpSLE(args[0], args[1]));

                    case '>=':
                        if (args.size() < 2)
                            ABORT("Operator '>=' expects 2 operands.");
                        return i1toi32(builder.CreateICmpSGE(args[0], args[1]));

                    case '==':
                        if (args.size() < 2)
                            ABORT("Operator '==' expects 2 operands.");
                        return i1toi32(builder.CreateICmpEQ(args[0], args[1]));

                    case '!=':
                        if (args.size() < 2)
                            ABORT("Operator '!=' expects 2 operands.");
                        return i1toi32(builder.CreateICmpNE(args[0], args[1]));

                    default:
                        ABORT("unhandled mathop? " << tok2str(op.op));
                    }
                },

                [&](auto &)
                { ABORT("UNHANDLED EXPR?"); });
        }

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
        unordered_map<string, Value *> formats;

        vector<BasicBlock *> continue_stack, break_stack;

        Value *get_fmt(const char *fmt) // value is the llvm ir value for a global string
        {
            Value *&res = formats[fmt]; // caching
            if (!res)                   // if it wasnt in the cache, make it
            {
                res = builder.CreateGlobalString(fmt, "", 0, &mod);
            }
            return res;
        }
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