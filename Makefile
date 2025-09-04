LLVM_DIR=/opt/homebrew/opt/llvm
CLANG=$(LLVM_DIR)/bin/clang++
CPPFLAGS=-I$(LLVM_DIR)/include
LDFLAGS=-L$(LLVM_DIR)/lib
LLVM_FLAGS=$(shell $(LLVM_DIR)/bin/llvm-config --cxxflags --ldflags --libs bitwriter core support)
# SANITIZER=-g -fsanitize=address
CXXFLAGS=-std=c++23 -Wno-c23-extensions -w -Wno-deprecated-declarations # <- suppress warnings

SRCS=main.cpp lexer.cpp idmanager.cpp parser.cpp compiler.cpp
OBJS=$(SRCS:.cpp=.o)
TARGET=a.out

test: $(TARGET)
	./$(TARGET)
	$(LLVM_DIR)/bin/clang main.bc -o main
	./main
	rm -f $(TARGET) main.bc

$(TARGET): $(OBJS)
	$(CLANG) $(OBJS) $(CPPFLAGS) $(LDFLAGS) $(LLVM_FLAGS) -lz -o $@

clean:
	rm -f $(OBJS) $(TARGET) main.elf main.bc
