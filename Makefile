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
CFLAGS=-I$(KLIBDIR) -O3 -MD
KERNEL_CFLAGS=-nostdlib -T linker.ld
ARCH = rv32im

### Build configuration:

# uncomment to build with only the rv32i standard
CFLAGS += -D__risc_no_ext=1
ARCH = rv32i

# configure if mtime is memory-mapped or inside a CSR:
# replace 0xFF11FF22FF33 with the correct address
CFLAGS += -DTIMECMP_IN_MEMORY=1 -DTIMECMP_MEM_ADDR=0xFF11FF22


### End configuration
CFLAGS += -march=$(ARCH)

# dependencies that need to be built:
_DEPS = ecall.c csr.c mutex.c sched.c

# dependencies as object files:
_OBJ = ecall.o mutex.o sched.o boot.o csr.o


DEPS  = $(patsubst %,$(KLIBDIR)/%,$(_DEPS))
OBJ  = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/boot.o: $(KLIBDIR)/boot.S
	$(CC) -c -o $@ $(KLIBDIR)/boot.S $(CFLAGS) -D__assembly=1 

$(ODIR)/%.o: $(KLIBDIR)/%.c $(KLIBDIR)/%.h
	$(CC) -c -o $@ $(KLIBDIR)/$*.c $(CFLAGS)

kernel: $(OBJ)
	mkdir -p $(TARGET)
	$(CC) -o $(TARGET)/$@ $^ kernel.c $(CFLAGS) $(KERNEL_CFLAGS)

kernel-dump: kernel
	$(OBJDUMP) -SFlDf $(TARGET)/kernel > $(TARGET)/kernel-objects

.PHONY: clean

clean:
	rm -rf $(ODIR) *~ $(KLIBDIR)/*~ $(TARGET)

-include $(OBJ:.o=.d)