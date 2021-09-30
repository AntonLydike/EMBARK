# small makefile for compiling the kernel

### Build configuration:
# process and binary count
PROCESS_COUNT = 8
PACKAGED_BINARY_COUNT = 8

# Define the maximum number of binaries packaged with the kernel
PACKAGED_BINARY_COUNT = 4

# Comment this out if you don't have any text IO device memory mapped
CFLAGS += -DTEXT_IO_ADDR=0xff0000 -DTEXT_IO_BUFLEN=64

# If you want to build without any extension, you can uncomment the next line
#CFLAGS += -D__risc_no_ext=1
# also change this to represent your target RISC-V architecture and extensions
ARCH = rv32im

# Configure if mtime is memory-mapped or inside a CSR:
# replace 0xFF11FF22FF33 with the correct address
#CFLAGS += -DTIMECMP_IN_MEMORY=1 -DTIMECMP_MEM_ADDR=0xFF11FF22

# Set this to the first out-of-bounds memory address
END_OF_USABLE_MEM=0xff0000

### End configuration

# kernel lib dir
KLIBDIR=kinclude
# object file dir
ODIR=obj
# target dir
TARGET=out

GCC_PREF=riscv32-unknown-elf-

CC=$(GCC_PREF)gcc
OBJDUMP=$(GCC_PREF)objdump
CFLAGS+=-I$(KLIBDIR) -MD -mcmodel=medany -Wall -Wextra -pedantic-errors -Wno-builtin-declaration-mismatch -march=$(ARCH)
KERNEL_CFLAGS=-nostdlib -T linker.ld
CFLAGS+=-DPROCESS_COUNT=$(PROCESS_COUNT) -DPACKAGED_BINARY_COUNT=$(PACKAGED_BINARY_COUNT) -DEND_OF_USABLE_MEM=$(END_OF_USABLE_MEM)

# dependencies that need to be built:
_DEPS = ecall.c csr.c sched.c io.c malloc.c

# dependencies as object files:
_OBJ = ecall.o sched.o boot.o csr.o io.o malloc.o


DEPS  = $(patsubst %,$(KLIBDIR)/%,$(_DEPS))
OBJ  = $(patsubst %,$(ODIR)/%,$(_OBJ))


.PHONY: clean
.PHONY: directories
.PHONY: kernel

directories:
	mkdir -p $(ODIR) $(TARGET)

$(ODIR)/boot.o: $(KLIBDIR)/boot.S
	$(CC) -c -o $@ $(KLIBDIR)/boot.S $(CFLAGS) -D__assembly

$(ODIR)/%.o: $(KLIBDIR)/%.c $(KLIBDIR)/%.h
	$(CC) -c -o $@ $(KLIBDIR)/$*.c $(CFLAGS)

_kernel: $(OBJ)
	$(CC) -o $(TARGET)/kernel $^ kernel.c $(CFLAGS) $(KERNEL_CFLAGS)

kernel: directories _kernel

kernel-dump: kernel
	$(OBJDUMP) -SFldft $(TARGET)/kernel > $(TARGET)/kernel-objects

all: kernel-dump


clean:
	rm -rf $(ODIR) *~ $(KLIBDIR)/*~ $(TARGET)

-include $(OBJ:.o=.d)