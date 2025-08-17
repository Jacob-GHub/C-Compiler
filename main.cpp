#include "includes.h"
#include "compiler.h"

static u8 test_case[] =
    {
#embed "temp.txt"
        , 0};

int main(void)
{
    llvm::Compiler{(u8 *)test_case};
    return 0;
}