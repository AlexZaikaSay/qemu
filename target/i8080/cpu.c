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


#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "exec/exec-all.h"

static gchar *i8080_gdb_arch_name(CPUState *cs)
{
    return g_strdup("i8080");
}

static void i8080_cpu_set_pc(CPUState *cs, vaddr value)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;

    env->pc = value & ~(target_ulong)1;
}

static void i8080_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    info->print_insn = print_insn_i8080;
}

static void i8080_cpu_synchronize_from_tb(CPUState *cs,
                                            const TranslationBlock *tb)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;

    env->pc = tb->pc;
}

static void i8080_cpu_state_reset(CPUI8080State *env)
{
    /* Reset Regs to Default Value */
    env->pc = 0xc000;
}

static void i8080_cpu_reset(DeviceState *dev)
{
    CPUState *s = CPU(dev);
    I8080CPU *cpu = I8080_CPU(s);
    I8080CPUClass *icc = I8080_CPU_GET_CLASS(cpu);
    CPUI8080State *env = &cpu->env;

    icc->parent_reset(dev);

    i8080_cpu_state_reset(env);
}

static void i8080_cpu_init(Object *obj)
{
    I8080CPU *cpu = I8080_CPU(obj);

    cpu_set_cpustate_pointers(cpu);
}

static ObjectClass *i8080_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;
    char *typename;

    typename = g_strdup_printf(I8080_CPU_TYPE_NAME("%s"), cpu_model);
    oc = object_class_by_name(typename);
    g_free(typename);
    if (!oc || !object_class_dynamic_cast(oc, TYPE_I8080_CPU) ||
        object_class_is_abstract(oc)) {
        return NULL;
    }
    return oc;
}

static void i8080_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    I8080CPUClass *icc = I8080_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    cpu_reset(cs);
    qemu_init_vcpu(cs);

    icc->parent_realize(dev, errp);
}

static bool i8080_cpu_has_work(CPUState *cs)
{
    return true;
}

#include "hw/core/sysemu-cpu-ops.h"

static const struct SysemuCPUOps i8080_sysemu_ops = {
    .get_phys_page_debug = i8080_cpu_get_phys_page_debug,
};

#include "hw/core/tcg-cpu-ops.h"

static const struct TCGCPUOps i8080_tcg_ops = {
    .initialize = i8080_tcg_init,
    .synchronize_from_tb = i8080_cpu_synchronize_from_tb,
    .tlb_fill = i8080_cpu_tlb_fill,
};

static void i8080_cpu_class_init(ObjectClass *c, void *data)
{
    I8080CPUClass *icc = I8080_CPU_CLASS(c);
    CPUClass *cc = CPU_CLASS(c);
    DeviceClass *dc = DEVICE_CLASS(c);

    device_class_set_parent_realize(dc, i8080_cpu_realizefn,
                                &icc->parent_realize);

    device_class_set_parent_reset(dc, i8080_cpu_reset, &icc->parent_reset);
    cc->class_by_name = i8080_cpu_class_by_name;
    cc->has_work = i8080_cpu_has_work;

    cc->gdb_read_register = i8080_cpu_gdb_read_register;
    cc->gdb_write_register = i8080_cpu_gdb_write_register;
    cc->gdb_num_core_regs = 9;
    cc->gdb_arch_name = i8080_gdb_arch_name;

    cc->dump_state = i8080_cpu_dump_state;
    cc->set_pc = i8080_cpu_set_pc;
    cc->sysemu_ops = &i8080_sysemu_ops;
    cc->tcg_ops = &i8080_tcg_ops;
    cc->disas_set_info = i8080_disas_set_info;
 }

static const TypeInfo i8080_cpu_type_infos[] = {
    {
        .name = TYPE_I8080_CPU,
        .parent = TYPE_CPU,
        .instance_size = sizeof(I8080CPU),
        .instance_init = i8080_cpu_init,
        .class_size = sizeof(I8080CPUClass),
        .class_init = i8080_cpu_class_init,
    },
};

DEFINE_TYPES(i8080_cpu_type_infos)