#ifndef INCLUDES_H
#define INCLUDES_H

// llvm stuff
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

// c++ libs
#include <iostream>
#include <cassert>
#include <climits>
#include <vector>
#include <variant>

using namespace std;
using i32 = int;
using u8 = unsigned char;
static_assert(sizeof(u8) == 1);
static_assert(CHAR_BIT == 8);

// abort
#define ABORT(...)                                  \
    do                                              \
    {                                               \
        std::cerr << "D: ";                         \
        std::cerr << __VA_ARGS__;                   \
        std::cerr << " , Line: " << __LINE__        \
                  << " File: " << __FILE__ << "\n"; \
        __builtin_trap();                           \
    } while (0)

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

#endif