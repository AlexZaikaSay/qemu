/*
 * i8080 gdb server stub
 *
 * Copyright (c) 2021 Alex Zaika
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
#include "qemu-common.h"
#include "exec/gdbstub.h"


int i8080_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;
    uint16_t af;

    switch (n) {
    case 0:
        af = env->a & 0xff;
        af <<= 8;
        af |= env->flags & 0xff;
        return gdb_get_reg16(mem_buf, af);
    case 1 ... 4:
        return gdb_get_reg16(mem_buf, env->regs[n - 1]);
    case 5:
        return gdb_get_reg16(mem_buf, env->pc);
    default:
        return gdb_get_reg16(mem_buf, 0);
    }
    return 0;
}

int i8080_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    return 0;
}