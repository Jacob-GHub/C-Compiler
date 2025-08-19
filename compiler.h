#include "includes.h"
#include "parser.h"

namespace llvm
{

    struct Compiler
    {
        ~Compiler()
        {
            builder.CreateRet(builder.getInt32(0));
            if constexpr (true)
            {
                mod.print(outs(), 0);
            }
            if (verifyModule(mod, &errs())) // check out later
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
        Compiler(u8 *text) : prog(Parser{text}()),
                             ctx(),                //<-- Creates an empty llvm::LLVMContext.
                             mod("main.cpp", ctx), // <-- Creates an llvm::Module named "main.cpp" associated with that context.
                             builder(ctx)
        {
            llvm::Triple triple(sys::getDefaultTargetTriple());
            mod.setTargetTriple(triple.str());
            setup(); // declares functions like main and printf
            gen_prog(prog);
        }

        Value *i1toi32(Value *i1);
        Value *i32toi1(Value *i32);
        void gen_prog(Program &p);
        void gen_stmt(Stmt &s);
        Value *gen_expr(Expr &e);

    private:
        Program prog;                                // <-- parsed representation of the input
        LLVMContext ctx;                             // <--- The llvm context (shared env for types,consts,etc)
        Module mod;                                  // <--The llvm IR module, the generated code unit
        IRBuilder<NoFolder> builder;                 // helps construct llvm instructions easily
        unordered_map<string, Function *> functions; //<-- map stores function names and their llvm representations
        unordered_map<string, Value *> formats;
        vector<BasicBlock *> continue_stack, break_stack;
        void setup();
        Value *get_fmt(const char *fmt); // value is the llvm ir value for a global string

        struct SymbolTable
        {
            struct Symbol
            {
                Value *alloca;
                bool is_array;
            };

            SymbolTable()
            {
                ++(*this);
            }

            void operator++()
            {
                tables.push_back({}); // pushing an empty scope
            }
            void operator--()
            {
                if (tables.empty())
                {
                    ABORT("[symtab] pop on empty table");
                }
                tables.pop_back();
            }

            void push(i32 name, Value *alloca, bool is_array)
            {
                // if (tables.back().contains(name))
                // {
                //     ABORT("DUP NAME: " << name);
                // }
                tables.back()[name] = {alloca, is_array};
            }

            Symbol &operator[](i32 name)
            {
                for (auto it = tables.rbegin(); it != tables.rend(); ++it)
                {
                    auto &scope = *it;
                    if (scope.contains(name))
                        return scope[name];
                }
                ABORT("SYMBOL TABLE DIDNT FIND THE NAME " << name);
            }

        private:
            vector<unordered_map<i32, Symbol>> tables;
        } symbols;
    };
};