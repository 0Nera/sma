#!/bin/bash

find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) -print0 | xargs -0 clang-format -i -style=file

gcc -O0 src/sma.c -o sma
gcc -O0 src/compiler.c -o compiler

chmod +X sma
chmod +X compiler
