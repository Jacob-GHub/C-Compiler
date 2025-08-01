.SILENT:

CLANG_PATH=/opt/homebrew/opt/llvm/bin/clang++

test: 
	clear
	$(CLANG_PATH) -std=c++23 main.cpp lexer.cpp -Wno-c23-extensions
	./a.out
	rm a.out

clean:
	rm -f temp.h temp.elf

# CLANG_PATH=/opt/homebrew/opt/llvm/bin/clang++

# test: temp.txt
# 	xxd -i temp.txt > embedded_temp.h
# 	$(CLANG_PATH) -std=c++23 -O3 main.cpp -o temp.elf
# 	clear
# 	./temp.elf
# 	rm temp.elf

# build_llvm:
# 	git clone --depth=1 https://github.com/llvm/llvm-project.git; \
# 	cd llvm-project/llvm; \
# 	mkdir -p build && cd build; \
# 	mkdir -p install; \
# 	cmake .. \
# 		-G Ninja \
# 		-DCMAKE_C_FLAGS="-O3 -ffast-math -mtune=native -march=native -I/opt/homebrew/include -L/opt/homebrew/lib -ljemalloc" \
# 		-DCMAKE_CXX_FLAGS="-O3 -fno-rtti -ffast-math -mtune=native -march=native -I/opt/homebrew/include -L/opt/homebrew/lib -ljemalloc" \
# 		-DCMAKE_BUILD_TYPE="Release" \
# 		-DCMAKE_INSTALL_PREFIX=install \
# 		-DLLVM_ENABLE_PROJECTS="clang;lld;clang-tools-extra" \
# 		-DLLVM_ENABLE_RUNTIMES="compiler-rt;libunwind;libcxx;libcxxabi"; \
# 	caffeinate -s -i ninja; \
# 	ninja install; \
# 	cd ../../../; \
# 	mv llvm-project/llvm/build/install llvm; \
# 	rm -r llvm-project


# tools:
# 	sudo apt install build-essential cmake git curl python3 python3-pip libjemalloc-dev. #include "llvm/IR/IRBuilder.h"