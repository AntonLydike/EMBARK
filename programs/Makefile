
CC = riscv32-unknown-elf-gcc
CFLAGS = -nostdlib -mcmodel=medany -Wall -Wextra -pedantic-errors -T ../linker.ld

simple:
	$(CC) $(CFLAGS) -o simple simple.c

spawn:
	$(CC) $(CFLAGS) -o spawn spawn.c

thread:
	$(CC) $(CFLAGS) -o threads threads.c

all: simple spawn thread

