#!/usr/bin/env python3
from dataclasses import dataclass
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from typing import List, Tuple, Dict, Generator, Union

import os, sys
import json

## Configuration: 

# A set of sections that we want to include in the image
INCLUDE_THESE_SECTIONS = set((
    '.text', '.stack', '.bss', '.sdata', 
    '.sbss', '.data', '.stack', '.init', 
    '.fini', '.preinit_array', '.init_array', 
    '.fini_array', '.rodata'
))
# these sections are empty, so we don't want to read the elf here
EMPTY_SECTIONS = set((
    '.bss', '.sbss', '.stack'
))

# sector size of the img file in bytes
SECTOR_SIZE = 512

# start address
MEM_START = 0x100

# process control block struct name
KERNEL_BINARY_TABLE = 'binary_table'
# loaded_binary struct size (4 integers)
KERNEL_BINARY_TABLE_ENTRY_SIZE = 4 * 4 

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
        if sec.name not in EMPTY_SECTIONS:
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

    def __init__(self, name):
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
    """
    Interface for writing the img file
    """
    data: bytes
    patches: List[Tuple[int, bytes]]
    dbg_nfo: Dict

    def __init__(self):
        self.data = b''
        self.patches = list()
        self.dbg_nfo = {
            'sections': dict(),
            'symbols': dict()
        }

    def seek(self, pos):
        if len(self.data) > pos:
            raise Exception("seeking already passed position!")
        if len(self.data) == pos:
            return
        print(f"  - zeros {pos-len(self.data):8x} {len(self.data):x}:{pos:x}")
        self.put(bytes(pos - len(self.data)), '', '.empty')
        assert len(self.data) == pos
    
    def align(self, bound):
        if len(self.data) % bound != 0:
            self.put(bytes(bound - (len(self.data) % bound)), '', '.empty')
        assert len(self.data) % bound == 0
        
    def put(self, stuff: bytes, parent: str, name: str) -> int:
        pos = len(self.data)
        self.data += stuff
        self.dbg_nfo['sections'][pos] = parent + ':' + name
        return pos

    def putBin(self, bin: Bin) -> int:
        bin_start = len(self.data)
        for sec in bin:
            img_pos = bin_start + sec.start - bin.start
            self.seek(img_pos)
            print(f"  - section {sec.name:<6} {img_pos:x}:{img_pos + sec.size:x}")
            self.put(sec.data, bin.name, sec.name)
        self.dbg_nfo['symbols'][bin.name] = {
            name: bin_start + val - bin.start
            for name, val in bin.symtab.items()
            if val != 0
        }
        return bin_start

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
        print(f"writing debug info to {fname}.dbg")
        with open(fname + '.dbg', 'w') as f:
            json.dump(self.dbg_nfo, f, indent = 2)


def package(kernel: str, binaries: List[str], out: str):
    """
    Main logic for creating the image file
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
    """
    Creates the binary data to populate the KERNEL_BINARY_TABLE structs
    """
    return b''.join(num.to_bytes(4, 'little') for num in (binid, entrypoint, start, end))


if __name__ == '__main__':
    if '--help' in sys.argv or len(sys.argv) == 1:
        print_help()
    else:
        print(f"creating image {sys.argv[-1]}")
        package(sys.argv[1], sys.argv[2:-1], sys.argv[-1])
