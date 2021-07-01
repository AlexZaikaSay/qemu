/*
 * Proficient System emulation.
 *
 * Copyright (c) 2021 Alex Zaika
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/loader.h"
#include "hw/i8080/profboard.h"

#define ROM_FILE "root.rom"

static void profboard_machine_init(MachineState *machine)
{   
    I8080_CPU(cpu_create(machine->cpu_type));
    const char* bios_name = machine->firmware ? machine->firmware : ROM_FILE;

    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *user_ram = g_new(MemoryRegion, 1);
    MemoryRegion *screen_ram = g_new(MemoryRegion, 1);
    MemoryRegion *rom = g_new(MemoryRegion, 1);
    MemoryRegion *regs_ram = g_new(MemoryRegion, 1);

    memory_region_init_ram(user_ram, NULL, "user.ram",  36 * KiB, &error_fatal);
    memory_region_init_ram(screen_ram, NULL, "screen.ram", 12 * KiB, &error_fatal);
    memory_region_init_rom(rom, NULL, "rom.rom", 14 * KiB, &error_fatal);
    memory_region_init_ram(regs_ram, NULL, "regs.ram", 2 * KiB, &error_fatal);


    memory_region_add_subregion(sysmem, 0x0000, user_ram);
    memory_region_add_subregion(sysmem, 0x9000, screen_ram);
    memory_region_add_subregion(sysmem, 0xc000, rom);
    memory_region_add_subregion(sysmem, 0xf800, regs_ram);



    if (load_image_mr(bios_name, rom) < 2 * KiB)
            error_report("Failed to load firmware '%s'.", bios_name);

}

static void profboard_machine_class_init(ObjectClass *oc,
        void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->init              = profboard_machine_init;
    mc->desc              = "Proficient PC";
    mc->is_default        = true;
    
    mc->default_cpu_type  = TYPE_I8080_CPU;
    mc->max_cpus          = 1;
};

static const TypeInfo profboard_machine_types[] = {
    {
        .name           = MACHINE_TYPE_NAME("prof"),
        .parent         = TYPE_PROFBOARD_MACHINE,
        .class_init     = profboard_machine_class_init,
    }, {
        .name           = TYPE_PROFBOARD_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(ProfBoardMachineState),
        .class_size     = sizeof(ProfBoardMachineClass),
        .abstract       = true,
    },
};

DEFINE_TYPES(profboard_machine_types)
