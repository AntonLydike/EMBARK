# EMBARK: An Educational and Modifiable BAsic RISC-V Kernel

EMBARK is a small kernel, designed for educational projects. It has very limited scope and is designed to be extensible.




## The toolchain:

I am using the [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain), configured with `--with-arch=rv32im --disable-linux --disable-gdb --disable-multilib` and built using `make -j <number of threads>`.

## The Makefile:

You can build the kernel using `make kernel`. Make sure the toolchain is in your path!
