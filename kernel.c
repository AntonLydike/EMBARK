#include "kernel.h"
#include "kinclude/ktypes.h"
#include "kinclude/ecall.h"
#include "kinclude/sched.h"
#include "kinclude/io.h"
#include "kinclude/malloc.h"
#include "kinclude/csr.h"

void read_binary_table();
void setup_mem_protection();

// this array is populated when the memory image is built, therefore it should
// resign in a section which is not overwritten with zeros on startup
loaded_binary binary_table[PACKAGED_BINARY_COUNT] __attribute__ ((section(".data")));

// access the memset function defined in boot.S
extern void memset(unsigned int, void*, void*);

// access linker symbols:
extern byte* _end, _ftext;

extern void init()
{
    dbgln("Kernel started!", 15);
    // setup phsycial memory protection
    setup_mem_protection();
    // initialize scheduler
    scheudler_init();
    // initialize tabel for associating ecall codes with their handlers
    init_ecall_table();
    // read supplied binaries, this will call malloc_init with the memory layout
    // then it will create a new process for each loaded binary
    read_binary_table();
    // give control to the scheudler and start runnign user programs
    scheduler_run_next();
}

void read_binary_table()
{
    char msg[28] = "found bin with id 0 at pos 0";

    struct malloc_info info = {
        .allocate_memory_end    = (void*) END_OF_USABLE_MEM,
        .allocate_memory_start  = (void*) 0
    };

    // calculate the end of loaded binaries
    for (int i = 0; i < PACKAGED_BINARY_COUNT; i++) {
        if (binary_table[i].binid == 0)
            break;

        if (DEBUGGING) {
            // print message
            msg[18] = (char) binary_table[i].binid + '0';
            msg[27] = (char) i + '0';
            dbgln(msg, 28);
        }

        info.allocate_memory_start = binary_table[i].bounds[1];
    }

    // initialize malloc
    malloc_init(&info);

    for (int i = 0; i < PACKAGED_BINARY_COUNT; i++) {
        if (binary_table[i].binid == 0)
            break;

        // create a new process for each binary found
        // it should have around 4kb stack
        optional_pcbptr res = create_new_process(binary_table + i);
        if (has_error(res)) {
            dbgln("Error creating initial process!", 31);
        }
    }
}

void setup_mem_protection()
{
    // this pmp config uses Top-of-Range mode - read more in the privileged spec p.49
    // we disallow all access to 0x0-0x100 from user and machine mode
    // and all access to 0x100-kernel_end from user mode
    // to do this, we must first calculate the kernel bin length
    uint32 kernel_bin_len = ((uint32) &_end) - ((uint32) &_ftext);

    CSR_WRITE(CSR_PMPADDR, 0x100);
    CSR_WRITE(CSR_PMPADDR + 1, 0x100 + kernel_bin_len);

    // this contains two pmp configs:
    // fields:    L    A  RWX
    // pmpcfg0: 0b1_00_01_000 <- disallow RWX from U and M mode
    // pmpcfg1: 0b0_00_01_000 <- disallow RWX from U mode
    // the resulint number is 0b0000100010001000, hex 0x888
    CSR_WRITE(CSR_PMPCFG, 0x888);
}
