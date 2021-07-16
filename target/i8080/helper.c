/*
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
#include "exec/exec-all.h"
#include "qemu/qemu-print.h"


static void i8080_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;

    const char *typename = object_class_get_name(oc);
    qemu_printf("  %s\n", typename);
}

void i8080_cpu_list(void)
{
    GSList *list = object_class_get_list_sorted(TYPE_I8080_CPU, false);
    qemu_printf("Available CPUs:\n");
    g_slist_foreach(list, i8080_cpu_list_entry, NULL);
    g_slist_free(list);
}

static int get_physical_address(CPUI8080State *env, hwaddr *physical,
                                int *prot, target_ulong address,
                                MMUAccessType access_type, int mmu_idx)
{
    *physical = address & 0xFFFF;
    *prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;

    return 0;
}

static void raise_mmu_exception(CPUI8080State *env, target_ulong address,
                                int rw, int tlb_error)
{
}

bool i8080_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                          MMUAccessType rw, int mmu_idx,
                          bool probe, uintptr_t retaddr)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;
    hwaddr physical;
    int prot;
    int ret = 0;

    rw &= MMU_DATA_STORE;
    ret = get_physical_address(env, &physical, &prot,
                               address, rw, mmu_idx);

    qemu_log_mask(CPU_LOG_MMU, "%s address=" TARGET_FMT_lx " ret %d physical "
                  TARGET_FMT_plx " prot %d\n",
                  __func__, (target_ulong)address, ret, physical, prot);

    if (!ret) {
        tlb_set_page(cs, address & TARGET_PAGE_MASK,
                     physical & TARGET_PAGE_MASK, prot | PAGE_EXEC,
                     mmu_idx, TARGET_PAGE_SIZE);
        return true;
    } else {
        assert(ret < 0);
        if (probe) {
            return false;
        }
        raise_mmu_exception(env, address, rw, ret);
        cpu_loop_exit_restore(cs, retaddr);
    }

}

hwaddr i8080_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    I8080CPU *cpu = I8080_CPU(cs);
    hwaddr phys_addr;
    int prot;
    int mmu_idx = cpu_mmu_index(&cpu->env, false);

    if (get_physical_address(&cpu->env, &phys_addr, &prot, addr,
                             MMU_DATA_LOAD, mmu_idx)) {
        return -1;
    }
    return phys_addr;
}