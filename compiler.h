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
    };
};