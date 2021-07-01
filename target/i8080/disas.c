
/*
 *  i8080 disassembler.
 *
 *  Copyright (c) 2021 Alex Zaika
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "disas/dis-asm.h"

typedef void (*print_func)(uint8_t *, disassemble_info *, const char *);

static void print_nn(uint8_t *buf, disassemble_info *info, const char *txt)
{
    int nn = *(uint16_t*)(buf + 1);
    info->fprintf_func (info->stream, txt, nn);
}

static void print_n(uint8_t *buf, disassemble_info *info, const char *txt)
{
    int nn = *(buf + 1);
    info->fprintf_func (info->stream, txt, nn);
}

static uint32_t sizes[256] = 
{
    0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}; 

static const char * inst_str[256] = 
{ 
    0, "ld bc,0x%04x", 0, 0, 0, 0, "ld b,0x%02x", 0, 0, 0, 0, 0, 0, 0, "ld c,0x%02x", 0,
    0, "ld de,0x%04x", 0, 0, 0, 0, "ld d,0x%02x", 0, 0, 0, 0, 0, 0, 0, "ld e,0x%02x", 0,
    0, "ld hl,0x%04x", 0, 0, 0, 0, "ld h,0x%02x", 0, 0, 0, 0, 0, 0, 0, "ld l,0x%02x", 0,
    0, "ld sp,0x%04x", "st (0x%04x),a", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "ld a,0x%02x", 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, "jmp 0x%04x", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static print_func print_funcs[256] = 
{ 
    0, print_nn, 0, 0, 0, 0, 0, print_n, 0, 0, 0, 0, 0, 0, print_n, 0,
    0, print_nn, 0, 0, 0, 0, 0, print_n, 0, 0, 0, 0, 0, 0, print_n, 0,
    0, print_nn, 0, 0, 0, 0, 0, print_n, 0, 0, 0, 0, 0, 0, print_n, 0,
    0, print_nn, print_nn, 0, 0, 0, 0, print_n, 0, 0, 0, 0, 0, 0, print_n, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, print_nn, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int print_insn_i8080(bfd_vma addr, disassemble_info *info)
{
    int n = info->buffer_length;
    uint8_t *buf = g_malloc(n);
    uint8_t *ptr = buf;

    info->read_memory_func(addr, buf, n, info);

    while (n > 0) {
        uint32_t op = *ptr;
        assert(sizes[op]);
        assert(inst_str[op]);
        assert(print_funcs[op]);
        print_funcs[op](ptr, info, inst_str[op]);
        addr += sizes[op];
        info->fprintf_func(info->stream, "\n0x" TARGET_FMT_lx ":  ", (target_ulong)addr);
        n -= sizes[op];
        ptr += sizes[op];
    }

    g_free(buf);
    return info->buffer_length;
}