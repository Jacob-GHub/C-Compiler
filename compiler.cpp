#include "compiler.h"

namespace llvm
{
    Value *Compiler::i1toi32(Value *i1)
    {
        return builder.CreateZExt(i1, builder.getInt32Ty());
    }
    Value *Compiler::i32toi1(Value *i32)
    {
        return builder.CreateICmpNE(i32, builder.getInt32(0));
    }
    void Compiler::gen_prog(Program &p)
    {
        for (auto &f : p.body)
        {
            gen_function(f);
        }
    }
    void Compiler::gen_function(Func &f)
    {
        string name = string(Lexer::ids[f.name]);

        vector<Type *> parameter_types;
        vector<i32> parameter_names;

        for (auto &param_expr : f.params)
        {
            param_expr(
                [&](Variable &v)
                {
                    parameter_types.push_back(builder.getInt32Ty());
                    parameter_names.push_back(v.name);
                },
                [&](Array &arr)
                {
                    parameter_types.push_back(builder.getInt32Ty()->getPointerTo()); // int*
                    parameter_names.push_back(arr.name);
                },
                [&](auto &)
                {
                    ABORT("PARSER MADE ERR WITH PARAM TYPE");
                });
        }
        FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), parameter_types, false); // signature requires return type, param types, varargs
        functions[name] = Function::Create(sig, GlobalValue::ExternalLinkage, name, mod);     // map name : llvm::function -> req's a (signature, global visibility, string name, module to place into )
        BasicBlock *entry_block = BasicBlock::Create(ctx, "entry", functions[name]);          // entry block for our function, placed into our function
        builder.SetInsertPoint(entry_block);                                                  // point our builder to our entry_block

        ++symbols; // add a new scope
        {
            size_t i = 0;
            for (Argument &arg : functions[name]->args()) // our params need to be in scope so loop through them
            {
                if (arg.getType() == builder.getInt32Ty()) // if the param is a var
                {
                    AllocaInst *alloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, ""); // allocate stack mem for a var inside the funcs stack grame, allocation is for an i32, nullptr (not an arr), name for instr
                    builder.CreateStore(&arg, alloca);                                            //   store the func's argument value in the local stack
                    symbols.push(parameter_names[i], alloca, nullptr, false, false);              // push this to our symbol table scope
                }
                else
                {
                    symbols.push(parameter_names[i], nullptr, &arg, true, true);
                }
                ++i;
            }
        }
        for (auto &s : f.body.body)
        {
            gen_stmt(s);
        }
        --symbols;
    }
    void Compiler::gen_stmt(Stmt &s)
    {
        if (builder.GetInsertBlock()->getTerminator())
        {
            return;
        }
        auto gen_block = [&](Block &block)
        {
            ++symbols;
            for (auto &s : block.body)
            {
                gen_stmt(s);
            }
            --symbols;
        };

        s(
            [&](Expr &expr)
            {
                //      CreateCall takes (callee (func*), args to the func (Value*), calltmp - optional name for the result)
                // builder.CreateCall(functions["printf"], {get_fmt("%d\n"), gen_expr(expr)});
                gen_expr(expr);
            },
            gen_block,
            [&](Return &ret)
            {
                builder.CreateRet(gen_expr(*ret.value));
            },
            [&](Let &let)
            {
                let.body(
                    [&](Variable &v)
                    {
                        AllocaInst *alloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, ""); // createAlloca(type,size,name) -> nullptr means its just a variable (scalar)
                        symbols.push(v.name, alloca, nullptr, false, false);
                    },
                    [&](Array &arr)
                    {
                        arr.size[0](
                            [&](IntLiteral &lit)
                            {
                                std::cerr << "Allocating array with size: " << lit.body << "\n";
                                if (lit.body <= 0)
                                {
                                    ABORT("Invalid array size: must be positive");
                                }
                                // Create an LLVM array type [N x i32] and allocate it on the stack.
                                // This produces `alloca [N x i32]` instead of `alloca i32, i32 N`.
                                Value *n = ConstantInt::get(builder.getInt32Ty(), lit.body);
                                ArrayType *arrTy = ArrayType::get(builder.getInt32Ty(), lit.body);
                                AllocaInst *alloca = builder.CreateAlloca(arrTy, nullptr, ""); // allocate the array object
                                symbols.push(arr.name, alloca, nullptr, true, false);
                            },
                            [&](auto &)
                            {
                                ABORT("TRIED TO DECL ARR WITH NON INTLITERAL SIZE");
                            });
                    },
                    [&](auto &)
                    {
                        ABORT("Let tried to decl non var or arr type?");
                    }

                );
            },

            [&](Assign &asn)
            {
                asn.lhs(
                    [&](Variable &v)
                    {
                        Value *rhs = gen_expr(asn.rhs);
                        auto symbol = symbols[v.name];
                        if (symbol.is_array)
                        {
                            ABORT("TRIED TO ASN VAR AS ARR");
                        }
                        builder.CreateStore(rhs, symbol.alloca); // generate llvm store Instruction: writes rhs into vars mem slot
                    },
                    [&](Array &arr)
                    {
                        Value *rhs = gen_expr(asn.rhs);
                        auto symbol = symbols[arr.name];
                        if (!symbol.is_array)
                            ABORT("TRIED TO ASSIGN ARR AS VAR");

                        Value *index = gen_expr(arr.size[0]);
                        Value *base = symbol.is_param ? symbol.value : symbol.alloca;
                        if (!base)
                            ABORT("ARRAY BASE PTR IS NULL");

                        Value *elemPtr = nullptr;
                        if (symbol.is_param)
                        {
                            // param: i32* base
                            elemPtr = builder.CreateGEP(builder.getInt32Ty(), base, {index});
                        }
                        else
                        {
                            // local array object: alloca [N x i32] -> gep [N x i32]*, 0, index
                            Value *zero = builder.getInt32(0);
                            Type *allocatedTy = cast<AllocaInst>(symbol.alloca)->getAllocatedType();
                            elemPtr = builder.CreateGEP(allocatedTy, symbol.alloca, {zero, index});
                        }

                        builder.CreateStore(rhs, elemPtr);
                    },
                    [&](auto &)
                    {
                        ABORT("ASSIGNING TO NON VAR OR ARR");
                    });
            },
            [&](Break &stmt)
            {
                if (!break_stack.size()) // break stack holds a stack of merge blocks to jump to after breaking
                {
                    ABORT("BREAK_STACK EMPTY");
                }
                builder.CreateBr(break_stack.back()); // create branch instr in IR to jump to top of the stack
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

                if (!builder.GetInsertBlock()->getTerminator()) // if loop body didnt terminate, branch back to top (inf loop)
                {
                    builder.CreateBr(loop_block);
                }
                builder.SetInsertPoint(merge_block); // move IR cursor to merge block (after the loop)

                continue_stack.pop_back();
                break_stack.pop_back();
            },
            [&](If &stmt)
            {
                BasicBlock *cur_block = builder.GetInsertBlock();                           // current block
                BasicBlock *if_block = BasicBlock::Create(ctx, "", cur_block->getParent()); // creating a new basic block in the current function
                BasicBlock *else_block = BasicBlock::Create(ctx, "", cur_block->getParent());
                BasicBlock *merge_block = BasicBlock::Create(ctx, "", cur_block->getParent());

                Value *cond = i32toi1(gen_expr(stmt.cond[0])); // gen_expr generates IR code for what we have in our cond vector (vector<Expr> cond)
                builder.CreateCondBr(cond, if_block, else_block);

                builder.SetInsertPoint(if_block);
                gen_block(stmt.body);
                if (!builder.GetInsertBlock()->getTerminator()) // if we dont find a terminator -> create branch to merge_block
                {
                    builder.CreateBr(merge_block);
                }
                builder.SetInsertPoint(else_block);
                if (stmt.else_body) // else_body is optional but eixstence doesnt need to be rerefed
                {
                    gen_block(*stmt.else_body); // since else_body is optional, we need to deref it
                }
                if (!builder.GetInsertBlock()->getTerminator()) // if theres still not a terminator, createBr to merge_block
                {
                    builder.CreateBr(merge_block);
                }
                builder.SetInsertPoint(merge_block);
            },
            [&](Nop &stmt) {});
    };
    Value *Compiler::gen_expr(Expr &e)
    {
        return e(
            [&](FnCall &call) -> Value *
            {
                Function *fn = functions[string(Lexer::ids[call.name])]; // functions is a map (string : Function *)
                if (!fn)
                {
                    ABORT("TRIED TO CALL UNDECLARED FUNCTION");
                }
                vector<Value *> args;
                for (auto &arg : call.args)
                {
                    args.push_back(gen_expr(arg)); // storing the values of the args (variable, Array)
                }
                return builder.CreateCall(fn, args);
            },
            [&](Variable &v) -> Value *
            {
                auto symbol = symbols[v.name];
                if (symbol.is_array)
                {
                    if (symbol.is_param)
                    {
                        // param is already i32*
                        return symbol.value;
                    }
                    else
                    {
                        // local array: alloca i32, size
                        Value *zero = builder.getInt32(0);
                        // symbol.alloca->getAllocatedType() should be ArrayType*
                        Type *allocatedTy = cast<AllocaInst>(symbol.alloca)->getAllocatedType();
                        return builder.CreateGEP(allocatedTy, symbol.alloca, {zero, zero});
                    }
                }

                return builder.CreateLoad(builder.getInt32Ty(), symbol.alloca);
            },
            [&](Array &arr) -> Value *
            {
                auto symbol = symbols[arr.name];
                if (!symbol.is_array)
                {
                    ABORT("NO INDEX OP OVERLOADS");
                }
                Value *index = gen_expr(arr.size[0]);
                Value *base = symbol.is_param ? symbol.value : symbol.alloca;
                if (!base)
                    ABORT("ARRAY BASE PTR IS NULL");

                Value *elemPtr = nullptr;
                if (symbol.is_param)
                {
                    // parameter is i32* (pointer to first element) -> single-index GEP on i32*
                    elemPtr = builder.CreateGEP(builder.getInt32Ty(), base, {index});
                }
                else
                {
                    // local array: alloca [N x i32] -> two-index GEP: 0, index
                    Value *zero = builder.getInt32(0);
                    Type *allocatedTy = cast<AllocaInst>(symbol.alloca)->getAllocatedType();
                    elemPtr = builder.CreateGEP(allocatedTy, symbol.alloca, {zero, index});
                }

                return builder.CreateLoad(builder.getInt32Ty(), elemPtr);
            },
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
    };
    Value *Compiler::get_fmt(const char *fmt) // value is the llvm ir value for a global string
    {
        Value *&res = formats[fmt]; // caching
        if (!res)                   // if it wasnt in the cache, make it
        {
            res = builder.CreateGlobalString(fmt, "", 0, &mod);
        }
        return res;
    }
    void Compiler::setup()
    {
        {
            // getPtrTy() -> means this function is expecting an i8* as an argument
            //  basically our printf is expecting to print a string of i8 chars
            FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), {builder.getPtrTy()}, true);
            functions["printf"] = Function::Create(sig, GlobalValue::ExternalLinkage, "printf", mod);
        }
        {
            //_______write(number)_______
            FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), builder.getInt32Ty(), true); // (return type, param type, var)
            functions["write"] = Function::Create(sig, GlobalValue::InternalLinkage, "write", mod);

            BasicBlock *bb = BasicBlock::Create(ctx, "entry", functions["write"]);
            builder.SetInsertPoint(bb);

            builder.CreateCall(functions["printf"], {get_fmt("%d"), functions["write"]->getArg(0)});
            builder.CreateRet(builder.getInt32(0));
        }
        // ____ putch(number) ____
        {
            FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), builder.getInt32Ty(), false);
            functions["putch"] = Function::Create(sig, GlobalValue::InternalLinkage, "putch", mod);
            BasicBlock *bb = BasicBlock::Create(ctx, "entry", functions["putch"]);
            builder.SetInsertPoint(bb);

            builder.CreateCall(functions["printf"], {get_fmt("%c"), functions["putch"]->getArg(0)});
            builder.CreateRet(builder.getInt32(0));
        }
        // ____scanf_____
        {
            FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), {builder.getPtrTy()}, true);
            functions["scanf"] = Function::Create(sig, GlobalValue::ExternalLinkage, "scanf", mod);
        }

        // ____ read() ____

        {
            FunctionType *sig = FunctionType::get(Type::getInt32Ty(ctx), {}, false);
            functions["read"] = Function::Create(sig, GlobalValue::InternalLinkage, "read", mod);
            BasicBlock *bb = BasicBlock::Create(ctx, "entry", functions["read"]);
            builder.SetInsertPoint(bb);

            Value *ptr = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "");
            builder.CreateCall(functions["scanf"], {get_fmt("%d"), ptr});
            Value *value = builder.CreateLoad(builder.getInt32Ty(), ptr);
            builder.CreateRet(value);
        }
    }
};