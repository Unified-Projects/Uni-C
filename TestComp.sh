clear
# ./build/bin/Compiler -i ./test/SimpleExit.uc -o test2
./build/bin/assembler ./test2.asm a
./build/bin/emulator ./test2.bytes -D