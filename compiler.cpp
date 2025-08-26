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
        for (auto &s : p.body)
        {
            gen_stmt(s);
        }
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
            gen_block,

            [&](Let &let)
            {
                let.body(
                    [&](Variable &v)
                    {
                        AllocaInst *alloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, ""); // createAlloca(type,size,name) -> nullptr means its just a variable (scalar)
                        symbols.push(v.name, alloca, false);
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
                                Value *sizeVal = ConstantInt::get(builder.getInt32Ty(), lit.body);
                                AllocaInst *alloca = builder.CreateAlloca(builder.getInt32Ty(), sizeVal, "");
                                symbols.push(arr.name, alloca, true);
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
                        {
                            ABORT("TRIED TO ASSIGN ARR AS VAR");
                        }
                        Value *index = gen_expr(arr.size[0]);
                        Value *gep = builder.CreateGEP(builder.getInt32Ty(), symbol.alloca, index); // getelementptr, calculates the mem addr of arr[index] (arg type, base ptr, index)
                        builder.CreateStore(rhs, gep);                                              // creates store instr of rhs into gep
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
            [&](Nop &stmt) {},
            [&](Expr &expr)
            {
                //      CreateCall takes (callee (func*), args to the func (Value*), calltmp - optional name for the result)
                builder.CreateCall(functions["printf"], {get_fmt("%d\n"), gen_expr(expr)});
            });
    };
    Value *Compiler::gen_expr(Expr &e)
    {
        return e(
            [&](Variable &v) -> Value *
            {
                auto symbol = symbols[v.name];
                if (symbol.is_array)
                {
                    ABORT("NOT ALLOWING PTR MATH");
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
                Value *gep = builder.CreateGEP(builder.getInt32Ty(), symbol.alloca, index);
                return builder.CreateLoad(builder.getInt32Ty(), gep);
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