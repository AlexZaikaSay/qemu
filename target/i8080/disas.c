
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
    info->fprintf_func(info->stream, txt, nn);
}

static void print_n(uint8_t *buf, disassemble_info *info, const char *txt)
{
    int nn = *(buf + 1);
    info->fprintf_func(info->stream, txt, nn);
}

static void print(uint8_t *buf, disassemble_info *info, const char *txt)
{
    info->fprintf_func(info->stream, txt, 0);
}

static uint32_t sizes[256] = 
{
    0, 3, 1, 1, 1, 1, 2, 1, 0, 0, 1, 1, 1, 1, 2, 1,
    0, 3, 1, 1, 1, 1, 2, 1, 0, 0, 1, 1, 1, 1, 2, 1,
    0, 3, 3, 1, 1, 1, 2, 0, 0, 0, 3, 1, 1, 1, 2, 0,
    0, 3, 3, 1, 1, 1, 2, 0, 0, 0, 3, 1, 1, 1, 2, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 3, 3, 0, 1, 2, 0, 1, 1, 3, 0, 0, 3, 2, 0,
    1, 0, 3, 0, 0, 1, 2, 0, 1, 0, 3, 0, 0, 0, 2, 0,
    1, 0, 3, 0, 0, 1, 2, 0, 1, 1, 3, 0, 0, 0, 2, 0,
    1, 0, 3, 0, 0, 1, 2, 0, 1, 0, 3, 0, 0, 0, 2, 0,
}; 

static const char * inst_str[256] = 
{ 
    0, "mov bc,0x%04x", "mov (bc),a", "inc bc", "inc b", "dec b", "mov b,0x%02x", "rol", 0, 0, "mov a,(bc)", "dec bc", "inc c", "dec c", "mov c,0x%02x", "ror",
    0, "mov de,0x%04x", "mov (de),a", "inc de", "inc d", "dec d", "mov d,0x%02x", "rlc", 0, 0, "mov a,(de)", "dec de", "inc e", "dec e", "mov e,0x%02x", "rrc",
    0, "mov hl,0x%04x", "mov (0x%04x),hl", "inc hl", "inc h", "dec h", "mov h,0x%02x", 0, 0, 0, "mov hl,(0x%04x)", "dec hl", "inc l", "dec l", "mov l,0x%02x", 0,
    0, "mov sp,0x%04x", "mov (0x%04x),a", "inc sp", "inc (hl)", "dec (hl)", "mov (hl),0x%02x", 0, 0, 0, "mov a,(0x%04x)", "dec sp", "inc a", "dec a", "mov a,0x%02x", 0,
    "mov b,b", "mov b,c", "mov b,d", "mov b,e", "mov b,h", "mov b,l", "mov b,(hl)", "mov b,a", "mov c,b", "mov c,c", "mov c,d", "mov c,e", "mov c,h", "mov c,l", "mov c,(hl)", "mov c,a",
    "mov d,b", "mov d,c", "mov d,d", "mov d,e", "mov d,h", "mov d,l", "mov d,(hl)", "mov d,a", "mov e,b", "mov e,c", "mov e,d", "mov e,e", "mov e,h", "mov e,l", "mov e,(hl)", "mov e,a",
    "mov h,b", "mov h,c", "mov h,d", "mov h,e", "mov h,h", "mov h,l", "mov h,(hl)", "mov h,a", "mov l,b", "mov l,c", "mov l,d", "mov l,e", "mov l,h", "mov l,l", "mov l,(hl)", "mov l,a",
    "mov (hl),b", "mov (hl),c", "mov (hl),d", "mov (hl),e", "mov (hl),h", "mov (hl),l", "mov (hl),(hl)", "mov (hl),a", "mov a,b", "mov a,c", "mov a,d", "mov a,e", "mov a,h", "mov a,l", "mov a,(hl)", "mov a,a",
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, "cmp b", "cmp c", "cmp d", "cmp e", "cmp h", "cmp l", "cmp (hl)", "cmp a",
    "ret nz", 0, "jnz 0x%04x", "jmp 0x%04x", 0, "push bc", "add 0x%02x", 0, "ret z", "ret", "jz 0x%04x", 0, 0, "call 0x%04x", "adc 0x%02x", 0,
    "ret nc", 0, "jnc 0x%04x", 0, 0, "push de", "sub 0x%02x", 0, "ret c", 0, "jc 0x%04x", 0, 0, 0, "sbc 0x%02x", 0,
    "ret np", 0, "jnp 0x%04x", 0, 0, "push hl", "and 0x%02x", 0, "ret p", "jmp (hl)", "jp 0x%04x", 0, 0, 0, "xor 0x%02x", 0,
    "ret ns", 0, "jns 0x%04x", 0, 0, "push af", "or 0x%02x", 0, "ret s", 0, "js 0x%04x", 0, 0, 0, "cmp 0x%02x", 0,
};

static print_func print_funcs[256] = 
{ 
    0, print_nn, print, print, print, print, print_n, print, 0, 0, print, 0, print, print, print_n, print,
    0, print_nn, print, print, print, print, print_n, print, 0, 0, print, 0, print, print, print_n, print,
    0, print_nn, print_nn, print, print, print, 0, print_n, 0, 0, print_nn, 0, print, print, print_n, 0,
    0, print_nn, print_nn, print, print, print, print_n, print_n, 0, 0, print_nn, 0, print, print, print_n, 0,
    print, print, print, print, print, print, print, print, print, print, print, print, print, print, print, print,
    print, print, print, print, print, print, print, print, print, print, print, print, print, print, print, print,
    print, print, print, print, print, print, print, print, print, print, print, print, print, print, print, print,
    print, print, print, print, print, print, print, print, print, print, print, print, print, print, print, print,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, print, print, print, print, print, print, print, print,
    print, 0, print_nn, print_nn, 0, print, print_n, 0, print, print, print_nn, 0, 0, print_nn, print_n, 0,
    print, 0, print_nn, 0, 0, print, print_n, 0, print, 0, print_nn, 0, 0, 0, print_n, 0,
    print, 0, print_nn, 0, 0, print, print_n, 0, print, print, print_nn, 0, 0, 0, print_n, 0,
    print, 0, print_nn, 0, 0, print, print_n, 0, print, 0, print_nn, 0, 0, 0, print_n, 0,
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