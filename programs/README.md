# EMBARK example programs

Note that these programs are of poor quality.

* `simple.c` does some arithmetic in a loop, occasionally prints something rather random (the sequence goes `90,90,90,2386,2386,ae86,520c,1cbdc,1cbde...`)
* `spawn.c` this thing spawn a new thread and exits when the thread overwrites a value.

## Compiling

The important thing when compiling user binaries are the following:

 * `-mcmodel=medany` makes all address loads pc-relative. This allows for relocating binaries without much effort. This only works, if the addresses are larger than signed 12 bit number, so make sure you programs are located far enough into memory. (the default linker script takes care of that)
 * `-T ../linker.ld` use the kernel linker script. This packs everything nice and close and sets the `__global_pointer$` etc up.

 If you don't want to worry, use the makefile `make all`.
