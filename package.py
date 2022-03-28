#!/usr/bin/env python3
from dataclasses import dataclass
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from typing import List, Tuple, Dict, Generator, Union, Set
from collections import defaultdict

import os, sys
import json

## Configuration:
# sector size of the img file in bytes
SECTOR_SIZE = 512

# start address
MEM_START = 0x100

# address where the userspace binaries should be located  (-1 to start directly after the kernel)
USR_BIN_START = -1

## end of config

# A set of sections that we want to include in the image
INCLUDE_THESE_SECTIONS = set((
    '.text', '.stack', '.bss', '.sdata', '.rdata', '.rodata'
    '.sbss', '.data', '.stack', '.init',
    '.fini', '.preinit_array', '.init_array',
    '.fini_array', '.rodata', '.thread_fini'
))
# these sections are empty, so we don't want to read the elf here
EMPTY_SECTIONS = set((
    '.bss', '.sbss', '.stack'
))

# this is the name of the global variable holding the list of loaded binaries
KERNEL_BINARY_TABLE = 'binary_table'
# loaded_binary struct size (4 integers)
KERNEL_BINARY_TABLE_ENTRY_SIZE = 4 * 4

# overwrite this function to generate the entries for the loaded binary list
def create_loaded_bin_struct(binid: int, entrypoint: int, start: int, end: int):
    """
    Creates the binary data to populate the KERNEL_BINARY_TABLE structs
    """
    return b''.join(num.to_bytes(4, 'little') for num in (binid, entrypoint, start, end))


def overlaps(p1, l1, p2, l2) -> bool:
    """
    check if the intervals (p1, p1+l1) and (p2, p2+l2) overlap
    """
    return (p1 <= p2 and p1 + l1 > p2) or (p2 <= p1 and p2 + l2 > p1)

class MemoryImageDebugInfos:
    """
    This defines the riscemu debug information format.

    See the riscemu project for more detail.
    """

    VERSION = '1'
    """
    Schema version
    """

    base: int = 0
    """
    The base address where the image starts. Defaults to zero.
    """

    sections: Dict[str, Dict[str, Tuple[int, int]]]
    """
    This dictionary maps a program and section to (start address, section length)
    """

    symbols: Dict[str, Dict[str, int]]
    """
    This dictionary maps a program and a symbol to a value
    """

    globals: Dict[str, Set[str]]
    """
    This dictionary contains the list of all global symbols of a given program
    """

    def __init__(self,
                 sections: Dict[str, Dict[str, Tuple[int, int]]],
                 symbols: Dict[str, Dict[str, int]],
                 globals: Dict[str, Set[str]],
                 base: int = 0
                 ):
        self.sections = sections
        self.symbols = symbols
        self.globals = globals
        self.base = base

    def serialize(self) -> str:
        def serialize(obj: any) -> str:
            if isinstance(obj, defaultdict):
                return dict(obj)
            if isinstance(obj, (set, tuple)):
                return list(obj)
            return "<<unserializable {}>>".format(getattr(obj, '__qualname__', '{unknown}'))

        return json.dumps(
            dict(
                sections=self.sections,
                symbols=self.symbols,
                globals=self.globals,
                base=self.base,
                VERSION=self.VERSION
            ),
            default=serialize,
            indent=2
        )

    def add_section(self, program: str, name: str, start: int, length: str):
        self.sectionss[program][name] = (start, length)

    def add_symbol(self, program: str, name: str, val: int):
        self.symbols[program][name] = val

    @classmethod
    def load(cls, serialized_str: str) -> 'MemoryImageDebugInfos':
        json_obj: dict = json.loads(serialized_str)

        if 'VERSION' not in json_obj:
            raise RuntimeError("Unknown MemoryImageDebugInfo version!")

        version: str = json_obj.pop('VERSION')

        # compare major version
        if version != cls.VERSION or version.split('.')[0] != cls.VERSION.split('.')[0]:
            raise RuntimeError(
                "Unknown MemoryImageDebugInfo version! This emulator expects version {}, debug info version {}".format(
                    cls.VERSION, version
                )
            )

        return MemoryImageDebugInfos(**json_obj)

    @classmethod
    def builder(cls) -> 'MemoryImageDebugInfos':
        return MemoryImageDebugInfos(
            defaultdict(dict), defaultdict(dict), defaultdict(set)
        )


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
    global_symbols: List[str]
    entry: int
    start: int

    def __init__(self, name):
        self.name = name
        self.secs = list()
        self.symtab = dict()
        self.global_symbols = list()

        with open(self.name, 'rb') as f:
            elf = ELFFile(f)
            if not elf.header.e_machine == 'EM_RISCV':
                raise Exception("Not a RISC-V elf file!")

            self.entry = elf.header.e_entry

            for sec in elf.iter_sections():
                if sec.name in INCLUDE_THESE_SECTIONS:
                    self.secs.append(Section(sec) )
                if isinstance(sec, SymbolTableSection):
                    for sym in sec.iter_symbols():
                        if not sym.name:
                            continue
                        
                        self.symtab[sym.name] = sym.entry.st_value

                        if sym.entry.st_info.bind == 'STB_GLOBAL':
                            self.global_symbols.append(sym.name)

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
    dbg_nfo: MemoryImageDebugInfos

    def __init__(self):
        self.data = b''
        self.patches = list()
        self.dbg_nfo = MemoryImageDebugInfos.builder()

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
        if parent:
            self.dbg_nfo.sections[parent][name] = (pos, len(stuff))
        return pos

    def putBin(self, bin: Bin) -> int:
        bin_start = len(self.data)
        for sec in bin:
            img_pos = bin_start + sec.start - bin.start
            self.seek(img_pos)
            print(f"  - section {sec.name:<6} {img_pos:x}:{img_pos + sec.size:x}")
            self.put(sec.data, bin.name, sec.name)
        self.dbg_nfo.symbols[bin.name] = {
            name: bin_start + val - bin.start
            for name, val in sorted(bin.symtab.items(), key=lambda x:x[1])
            if val != 0
        }
        self.dbg_nfo.globals[bin.name] = set(bin.global_symbols)
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
            f.write(self.dbg_nfo.serialize())


def package(kernel: str, binaries: List[str], out: str):
    """
    Main logic for creating the image file
    """
    img = MemImageCreator()

    # process kernel
    img.seek(MEM_START)
    kernel = Bin(kernel)
    kernel.name = 'kernel' # make sure kernel is marked kernel in debug symbols
    bin_table_addr = kernel.symtab.get(KERNEL_BINARY_TABLE, 0) - kernel.start + MEM_START
    print(f"kernel binary loaded, binary table located at: {bin_table_addr:x} (symtab addr {kernel.symtab.get(KERNEL_BINARY_TABLE, '??'):x})")


    img.putBin(kernel)

    if USR_BIN_START > 0:
        img.seek(USR_BIN_START)

    binid = 0
    for bin_name in binaries:
        img.align(8) # align to eight bytes
        bin = Bin(bin_name)
        print(f"adding binary \"{bin.name}\"")
        start = img.putBin(bin)
        addr = bin_table_addr + (binid * KERNEL_BINARY_TABLE_ENTRY_SIZE)
        img.patch(addr, create_loaded_bin_struct(binid+1, bin.entry - bin.start + start, start, start + bin.size()))
        binid += 1
        print(f"           binary    image")
        print(f"  entry:   {bin.entry:>6x}   {bin.entry - bin.start + start:>6x}")
        print(f"  start:   {bin.start:>6x}   {start:>6x}")

    img.write(out)


if __name__ == '__main__':
    if '--help' in sys.argv or len(sys.argv) == 1:
        print("package.py <kernel path> <user path> [<user path>...] <output path>\n\
\n\
Generate a memory image with the given kernel and userspace binaries.")
    else:
        print(f"creating image {sys.argv[-1]}")
        package(sys.argv[1], sys.argv[2:-1], sys.argv[-1])
