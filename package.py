#!/usr/bin/env python3
from dataclasses import dataclass
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from typing import List, Tuple, Dict, Generator, Union

import os, sys

# A set of sections that we want to include in the image
INCLUDE_THESE_SECTIONS = set(('.text', '.stack', '.bss', '.sdata', '.sbss', '.data', '.stack'))


# sector size of the img file in bytes
SECTOR_SIZE = 512

# start address
MEM_START = 0x100

# process control block struct name
KERNEL_BINARY_TABLE = 'binary_table'
KERNEL_BINARY_TABLE_ENTRY_SIZE = 4 * 4 # loaded_binary struct size (4 integers)

def overlaps(p1, l1, p2, l2) -> bool:
    return (p1 <= p2 and p1 + l1 > p2) or (p2 <= p1 and p2 + l2 > p1)

class Section:
    name: str
    start: int
    size: int
    data: bytes

    def __init__(self, sec):
        self.name = sec.name
        self.start = sec.header.sh_addr
        if sec.name in ('.text', '.data', '.sdata'):
            self.data = sec.data()
        else:
            self.data = bytes(sec.header.sh_size)
        self.size = sec.header.sh_size
        assert self.size == len(self.data)

    def __repr__(self) -> str:
        return "Section[{}]:{}:{}\n".format(self.name, self.start, self.size)

    def __len__(self):
        return self.size

class Bin:
    name: str
    secs: List[Section]
    symtab: Dict[str, int]
    entry: int
    start: int

    def __init__(self, name) -> Generator[Section, None, None]:
        self.name = name
        self.secs = list()
        with open(self.name, 'rb') as f:
            elf = ELFFile(f)
            if not elf.header.e_machine == 'EM_RISCV':
                raise Exception("Not a RISC-V elf file!")

            self.entry = elf.header.e_entry

            for sec in elf.iter_sections():
                if sec.name in INCLUDE_THESE_SECTIONS:
                    self.secs.append(Section(sec) )
                if isinstance(sec, SymbolTableSection):
                    self.symtab = {
                        sym.name: sym.entry.st_value for sym in sec.iter_symbols() if sym.name
                    }
            
            self.secs = sorted(self.secs, key=lambda sec: sec.start)
            self.start = self.secs[0].start

    def __iter__(self):
        for x in self.secs:
            yield x

    def size(self):
        return sum(sec.size for sec in self)

class MemImageCreator:
    data: bytes
    patches: List[Tuple[int, bytes]]

    def __init__(self):
        self.data = b''
        self.patches = list()

    def seek(self, pos):
        if len(self.data) > pos:
            raise Exception("seeking already passed position!")
        if len(self.data) == pos:
            return
        print(f"  - zeros {pos-len(self.data):8x} {len(self.data):x}:{pos:x}")
        self.put(bytes(pos - len(self.data)))
        assert len(self.data) == pos
    
    def align(self, bound):
        if len(self.data) % bound != 0:
            self.put(bytes(bound - (len(self.data) % bound)))
        assert len(self.data) % bound == 0
        
    def put(self, stuff: bytes) -> int:
        pos = len(self.data)
        self.data += stuff
        return pos

    def putBin(self, bin: Bin) -> int:
        pos = len(self.data)
        for sec in bin:
            img_pos = pos + sec.start - bin.start
            self.seek(img_pos)
            print(f"  - section {sec.name:<6} {img_pos:x}:{img_pos + sec.size:x}")
            self.put(sec.data)
        return pos

    def patch(self, pos, bytes):
        for ppos, pbytes in self.patches:
            if overlaps(ppos, len(pbytes), pos, len(bytes)):
                raise Exception("cant patch same area twice!")
        self.patches.append((pos, bytes))

    def write(self, fname):
        """
        write to a file
        """
        pos = 0
        print(f"writing binary image to {fname}")
        with open(fname, 'wb') as f:
            for patch_start, patch_data in sorted(self.patches, key=lambda e: e[0]):
                if pos < patch_start:
                    filler = patch_start - pos
                    f.write(self.data[pos : pos + filler])
                    print(f" - data  {pos:x}:{pos+filler:x}")
                    pos += filler
                assert pos == patch_start
                f.write(patch_data)
                print(f" - patch {pos:x}:{pos+len(patch_data):x}")
                pos += len(patch_data)
            if pos < len(self.data):
                print(f" - data  {pos:x}:{len(self.data):x}")
                f.write(self.data[pos : len(self.data)])
            if len(self.data) % SECTOR_SIZE != 0:
                print(f" - zeros {len(self.data):x}:{(SECTOR_SIZE - (len(self.data) % SECTOR_SIZE))+len(self.data):x}")
                f.write(bytes(SECTOR_SIZE - (len(self.data) % SECTOR_SIZE)))
        # done!

def package(kernel: str, binaries: List[str], out: str):
    """
    create an image
    """
    img = MemImageCreator()
    
    # process kernel
    img.seek(MEM_START)
    kernel = Bin(kernel)
    bin_table_addr = kernel.symtab.get(KERNEL_BINARY_TABLE, 0) - kernel.start + MEM_START
    print(f"kernel binary loaded, binary table located at: {bin_table_addr:x} (symtab addr {kernel.symtab.get(KERNEL_BINARY_TABLE, '??'):x})")


    img.putBin(kernel)

    binid = 0
    for bin_name in binaries:
        img.align(8) # align to eight bytes
        bin = Bin(bin_name)
        print(f"adding binary \"{bin.name}\"")
        start = img.putBin(bin)
        addr = bin_table_addr + (binid * KERNEL_BINARY_TABLE_ENTRY_SIZE)
        img.patch(addr, pcb_patch(binid+1, bin.entry - bin.start + start, start, start + bin.size()))
        binid += 1
        print(f"           binary    image")
        print(f"  entry:   {bin.entry:>6x}   {bin.entry - bin.start + start:>6x}")
        print(f"  start:   {bin.start:>6x}   {start:>6x}")

    img.write(out)


def pcb_patch(binid: int, entrypoint: int, start: int, end: int):
    return b''.join(num.to_bytes(4, 'little') for num in (binid, entrypoint, start, end))


    


if __name__ == '__main__':
    if '--help' in sys.argv or len(sys.argv) == 1:
        print_help()
    else:
        package(sys.argv[1], sys.argv[2:-1], sys.argv[-1])
