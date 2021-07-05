/*
 *  i8080 emulation for qemu: main CPU struct.
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

#ifndef I8080_CPU_H
#define I8080_CPU_H

#include "cpu-qom.h"
#include "exec/cpu-defs.h"

int i8080_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n);
int i8080_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n);
void i8080_cpu_dump_state(CPUState *cpu, FILE *f, int flags);
hwaddr i8080_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
void i8080_tcg_init(void);
bool i8080_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                          MMUAccessType access_type, int mmu_idx,
                          bool probe, uintptr_t retaddr);

typedef struct i8080_def_t i8080_def_t;

#define CPU_NB_REGS 4

enum {
    C_FLAG      = 0,
    P_FLAG      = 2,
    A_FLAG      = 4,  
    Z_FLAG      = 6,
    S_FLAG      = 7,

    C_MASK      = (1 << C_FLAG),
    P_MASK      = (1 << P_FLAG),
    A_MASK      = (1 << A_FLAG),  
    Z_MASK      = (1 << Z_FLAG),
    S_MASK      = (1 << S_FLAG),

    DEF_FLAGS   = 0x02, 
};

#define MASK_OP_COND(opcode)             (MASK_COND(opcode >> 2))

#define GET_FLAG(flags, off)             ((flags >> off) & 0x01)

#define GET_C_FLAG(flags)                    GET_FLAG(flags, C_FLAG)
#define GET_P_FLAG(flags)                    GET_FLAG(flags, P_FLAG)
#define GET_A_FLAG(flags)                    GET_FLAG(flags, A_FLAG)  
#define GET_Z_FLAG(flags)                    GET_FLAG(flags, Z_FLAG)
#define GET_S_FLAG(flags)                    GET_FLAG(flags, S_FLAG)

enum {
    COND_NZ                                          = 0x00,
    COND_Z                                           = 0x01,
    COND_NC                                          = 0x02,
    COND_C                                           = 0x03,
    COND_PO                                          = 0x04,
    COND_PE                                          = 0x05,
    COND_P                                           = 0x06,
    COND_M                                           = 0x07,
};

typedef struct CPUI8080State CPUI8080State;
struct CPUI8080State {
    
    /* i8080 registers */
    target_ulong a;
    target_ulong regs[CPU_NB_REGS];
    target_ulong pc;

    /* i8080 flags */
    target_ulong flags;
};

/**
 * I8080CPU:
 * @env: #CPUI8080State
 *
 * A i8080 CPU.
 */
struct I8080CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUI8080State env;
};

int print_insn_i8080(bfd_vma addr, disassemble_info *info);

typedef CPUI8080State CPUArchState;
typedef I8080CPU ArchCPU;

static inline int cpu_mmu_index(CPUI8080State *env, bool ifetch)
{
    return 0;
}

void i8080_cpu_list(void);

#define cpu_list i8080_cpu_list

#include "exec/cpu-all.h"

static inline void cpu_get_tb_cpu_state(CPUI8080State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->pc;
    *cs_base = 0;
    *flags = 0;
}

#define I8080_CPU_TYPE_SUFFIX "-" TYPE_I8080_CPU
#define I8080_CPU_TYPE_NAME(model) model I8080_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_I8080_CPU

#endif /* I8080_CPU_H */
