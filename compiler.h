#include "includes.h"
#include "parser.h"

namespace llvm
{

    struct Compiler
    {
        ~Compiler()
        {
            if constexpr (1)
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
        void gen_function(Func &f);
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
            struct Symbol // represents a decl'd var
            {
                Value *alloca; // ptr to IR alloca instructions
                bool is_array;
            };

            SymbolTable()
            {
                ++(*this); // SymbolTable Constructor, just adds a scope
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
                tables.back()[name] = {alloca, is_array}; // pushes symbol to back of tables
            }

            Symbol &operator[](i32 name)
            {
                for (size_t i = tables.size() - 1; i != (size_t)-1; --i)
                {
                    if (tables[i].contains(name))
                    {
                        return tables[i][name]; // returns Symbol
                    }
                }
                ABORT("SYMBOL TABLE DIDNT FIND THE NAME " << name);
            }

        private:
            vector<unordered_map<i32, Symbol>> tables; // each scope is a map entry (name:Symbol), symbol holds value and whethere it's an array or not, Tables is just the stack of scopes
        } symbols;                                     // creates a global instance of SymbolTable named symbols
    };
};