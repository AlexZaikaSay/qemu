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

typedef struct i8080_def_t i8080_def_t;

typedef struct CPUI8080State CPUI8080State;
struct CPUI8080State {
    
    /* QEMU */
    int error_code;
    uint32_t hflags;    /* CPU State */

    /* Internal CPU feature flags.  */
    uint64_t features;

    const i8080_def_t *cpu_model;
    void *irq[8];
    struct QEMUTimer *timer; /* Internal timer */
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

typedef CPUI8080State CPUArchState;
typedef I8080CPU ArchCPU;

static inline int cpu_mmu_index(CPUI8080State *env, bool ifetch)
{
    return 0;
}

#include "exec/cpu-all.h"

static inline void cpu_get_tb_cpu_state(CPUI8080State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    // TODO: return flags
    *pc = 0;
    *cs_base = 0;
    *flags = 0;
}

#define I8080_CPU_TYPE_SUFFIX "-" TYPE_I8080_CPU
#define I8080_CPU_TYPE_NAME(model) model I8080_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_I8080_CPU

#endif /* I8080_CPU_H */
