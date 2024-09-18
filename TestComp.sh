#!/bin/bash

if [[ "$2" == "-nc" ]]; then
    echo "Skipping Compiler"
else
    clear
    ./build/bin/Compiler -i ./test/SimpleExit.uc -o test2
fi

if [[ "$1" == "-e" ]]; then
    ./build/bin/assembler ./test2.asm a
    ./build/bin/emulator ./test2.bytes -D
fi

if [[ "$1" == "-a" ]]; then
    ./build/bin/assembler ./test2.asm a
fi