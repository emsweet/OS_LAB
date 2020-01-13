all:
	nasm -f	elf32 my_print.asm
	g++ -o fat -m32 1.cpp my_print.o
	./fat
