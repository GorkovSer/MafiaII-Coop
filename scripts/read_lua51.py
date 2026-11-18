"""Bounded static Lua 5.1 chunk reader. Never loads a Lua VM or executes code.

Format references: https://www.lua.org/source/5.1/lundump.c.html and
https://www.lua.org/source/5.1/lopcodes.h.html . Only the observed PC layout
(little endian, int/size_t/instruction/float all 4 bytes) is supported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

HEADER = bytes.fromhex('1b4c75615100010404040400')
OPCODES = ('MOVE LOADK LOADBOOL LOADNIL GETUPVAL GETGLOBAL GETTABLE SETGLOBAL '
           'SETUPVAL SETTABLE NEWTABLE SELF ADD SUB MUL DIV MOD POW UNM NOT LEN '
           'CONCAT JMP EQ LT LE TEST TESTSET CALL TAILCALL RETURN FORLOOP '
           'FORPREP TFORLOOP SETLIST CLOSE CLOSURE VARARG').split()


class ChunkError(ValueError):
    pass


class Reader:
    def __init__(self, data):
        if len(data) > 8 * 1024 * 1024:
            raise ChunkError('Chunk exceeds static-analysis budget')
        self.data, self.at = data, 0
        self.functions, self.instructions = 0, 0

    def take(self, size):
        if size < 0 or size > len(self.data) - self.at:
            raise ChunkError('Truncated chunk')
        value = self.data[self.at:self.at+size]
        self.at += size
        return value

    def integer(self):
        value = struct.unpack('<i', self.take(4))[0]
        if not 0 <= value <= 200000:
            raise ChunkError('Integer outside analysis budget')
        return value

    def string(self):
        size = self.integer()
        if not size:
            return None
        data = self.take(size)
        if data[-1] != 0:
            raise ChunkError('Missing string terminator')
        return data[:-1].decode('utf-8', errors='replace')

    def function(self, depth=0):
        self.functions += 1
        if depth > 64 or self.functions > 4096:
            raise ChunkError('Function nesting/count budget exceeded')
        offset = self.at
        source = self.string()
        first, last = self.integer(), self.integer()
        upvalues, parameters, vararg, stack = self.take(4)
        count = self.integer()
        self.instructions += count
        if self.instructions > 200000:
            raise ChunkError('Instruction budget exceeded')
        code = list(struct.unpack('<' + 'I' * count, self.take(count * 4)))
        constants = []
        for _ in range(self.integer()):
            kind = self.take(1)[0]
            if kind == 0:
                value = None
            elif kind == 1:
                value = bool(self.take(1)[0])
            elif kind == 3:
                value = struct.unpack('<f', self.take(4))[0]
            elif kind == 4:
                value = self.string()
            else:
                raise ChunkError('Unsupported constant tag')
            constants.append(value)
        children = [self.function(depth + 1) for _ in range(self.integer())]
        lines = self.integer()
        self.take(lines * 4)
        for _ in range(self.integer()):
            self.string()
            self.integer()
            self.integer()
        for _ in range(self.integer()):
            self.string()
        return dict(offset=offset, source=source, first_line=first, last_line=last,
                    upvalues=upvalues, parameters=parameters, vararg=vararg,
                    stack=stack, code=code, constants=constants, children=children)


def read_chunk(data):
    reader = Reader(data)
    if reader.take(len(HEADER)) != HEADER:
        raise ChunkError('Unsupported Lua header/layout')
    root = reader.function()
    if reader.at != len(data):
        raise ChunkError('Trailing bytes after chunk')
    return root


def decode(word):
    op = word & 63
    if op >= len(OPCODES):
        raise ChunkError('Unsupported opcode')
    return dict(op=OPCODES[op], a=(word >> 6) & 255, b=(word >> 23) & 511,
                c=(word >> 14) & 511, bx=word >> 14, sbx=(word >> 14)-131071)


def watch_report(data, watch):
    """Report references only; no claims about execution or control flow."""
    root = read_chunk(data)
    matches = []

    def visit(node, path):
        watched = {i: value for i, value in enumerate(node['constants'])
                   if isinstance(value, str) and value in watch}
        hits = []
        pc = 0
        while pc < len(node['code']):
            word = node['code'][pc]
            op = word & 63
            if op >= len(OPCODES):
                raise ChunkError('Unsupported opcode')
            if op in (1, 5, 7):
                refs = [word >> 14]
            elif op in (6, 9, 11):
                fields = [(word >> 14) & 511]
                if op == 9:
                    fields.append((word >> 23) & 511)
                refs = [field-256 for field in fields
                        if field >= 256]
            else:
                refs = []
            for index in refs:
                if index >= len(node['constants']):
                    raise ChunkError('Constant reference outside prototype')
                if op in (5, 7) and not isinstance(node['constants'][index], str):
                    raise ChunkError('Global name is not a string')
                if index in watched:
                    hits.append(dict(pc=pc, opcode=OPCODES[op], name=watched[index]))
            skip = 0
            if op == 34 and ((word >> 14) & 511) == 0:
                skip = 1  # SETLIST's following word is data, not an opcode.
            if op == 36:
                child = word >> 14
                if child >= len(node['children']):
                    raise ChunkError('Invalid CLOSURE prototype index')
                skip = node['children'][child]['upvalues']
            if pc + skip >= len(node['code']):
                raise ChunkError('Truncated instruction extension/bindings')
            pc += 1 + skip
        if hits:
            matches.append(dict(function=path, chunk_offset=node['offset'],
                                first_line=node['first_line'], last_line=node['last_line'], hits=hits))
        for i, child in enumerate(node['children']):
            visit(child, path + '/' + str(i))

    visit(root, '0')
    return dict(schema=1, sha256=hashlib.sha256(data).hexdigest(),
                executed=False, reachability_verified=False, matches=matches)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('chunk', type=Path)
    parser.add_argument('--watch', nargs='+', default=['missionNumber', 'missionPart', 'SaveGame'])
    args = parser.parse_args()
    try:
        with args.chunk.open('rb') as source:
            data = source.read(8 * 1024 * 1024 + 1)
        print(json.dumps(watch_report(data, set(args.watch)), indent=2))
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f'Static chunk read refused: {error}\n')
