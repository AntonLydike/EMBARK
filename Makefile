# simple makefile for this kernel project
# This is BAD. it should be reworked before anyone else uses this.

# kernel lib dir
KLIBDIR=kinclude
# object file dir
ODIR=obj
# target dir
TARGET=out

GCC_PREF=riscv32-unknown-elf-

CC=$(GCC_PREF)gcc
OBJDUMP=$(GCC_PREF)objdump
CFLAGS=-I$(KLIBDIR) -march=rv32im -O3
KERNEL_CFLAGS=-nostdlib

# dependencies that need to be built:
_DEPS = ecall.c csr.c mutex.c sched.c

# dependencies as object files:
_OBJ = ecall.o mutex.o sched.o boot.o csr.o


DEPS  = $(patsubst %,$(KLIBDIR)/%,$(_DEPS))
OBJ  = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/boot.o:
	$(CC) -c -o $@ $(KLIBDIR)/boot.S $(CFLAGS)

$(ODIR)/%.o:
	$(CC) -c -o $@ $(KLIBDIR)/$*.c $(CFLAGS)

kernel: $(OBJ)
	mkdir -p $(TARGET)
	$(CC) -o $(TARGET)/$@ $^ kernel.c $(CFLAGS) $(KERNEL_CFLAGS)

kernel-dump: kernel
	$(OBJDUMP) -SFlDf $(TARGET)/kernel > $(TARGET)/kernel-objects

.PHONY: clean

clean:
	rm -rf $(ODIR) *~ $(KLIBDIR)/*~ $(TARGET)