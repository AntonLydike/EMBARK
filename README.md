# EMBARK: An Educational and Modifiable BAsic RISC-V Kernel

EMBARK is a small kernel, designed for educational projects. It has very limited scope and is designed to be extensible.




## The toolchain:

I am using the [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain), configured with `--with-arch=rv32im --disable-linux --disable-gdb --disable-multilib` and built using `make -j <number of threads>`.

## The Makefile:

You can build the kernel using `make kernel`. Make sure the toolchain is in your path!


## Packaging a kernel image with user programs

You can use the `package.py` script to package a kernel and multiple user binaries into a single `img` file.

Debugging information is also emitted, it's a json formatted file called `<name>.img.dbg`.

To generate such an image, run `python3 package.py out/kernel <user bin 1> <usr bin 2> ... output/path/memory.img`. You can edit the script to change various variables. They somewhat well documented.