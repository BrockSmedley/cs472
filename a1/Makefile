prime:
	gcc -std=c11 -o prime ./prime_number.c
x86:
	gcc -fno-asynchronous-unwind-tables -std=c11 -Og -S -o ./prime_number_x86.s ./prime_number.c
mips:
	mips64-linux-gnu-gcc -fno-asynchronous-unwind-tables -std=c11 -Og -S -o ./prime_number_mips.s ./prime_number.c
clean:
	rm prime prime_number_x86.s
