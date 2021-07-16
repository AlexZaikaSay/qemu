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
#include "ui/console.h"

#define ROM_FILE "root.rom"

static void profboard_machine_init(MachineState *machine)
{
    Error *error_fatal;
    I8080_CPU(cpu_create(machine->cpu_type));
    const char* bios_name = machine->firmware ? machine->firmware : ROM_FILE;

    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *user_ram = g_new(MemoryRegion, 1);
    
    MemoryRegion *rom = g_new(MemoryRegion, 1);
    MemoryRegion *regs_ram = g_new(MemoryRegion, 1);

    memory_region_init_ram(user_ram, NULL, "user.ram",  36 * KiB, &error_fatal);
    memory_region_init_rom(rom, NULL, "rom.rom", 14 * KiB, &error_fatal);
    memory_region_init_ram(regs_ram, NULL, "regs.ram", 2 * KiB, &error_fatal);


    memory_region_add_subregion(sysmem, 0x0000, user_ram);
    
    memory_region_add_subregion(sysmem, 0xc000, rom);
    memory_region_add_subregion(sysmem, 0xf800, regs_ram);

    if (load_image_mr(bios_name, rom) < 2 * KiB)
            error_report("Failed to load firmware '%s'.", bios_name);

    ProfBoardVGAState* vga = PROFBOARD_VGA(qdev_new(TYPE_PROFBOARD_VGA));

    sysbus_realize_and_unref(SYS_BUS_DEVICE(vga), &error_fatal);
    
    memory_region_init_ram(&vga->iomem, NULL, "screen.ram", 12 * KiB, &error_fatal);
    memory_region_add_subregion(sysmem, 0x9000, &vga->iomem);

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

static void profboard_vga_init(Object *obj)
{
}

static void vga_refresh(void *opaque)
{
    int x, y, b, c;
    ProfBoardVGAState *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->con);
    uint8_t *ptr = memory_region_get_ram_ptr(&s->iomem);
    uint32_t *screen = surface_data(surface);

    int counter = 0;

    for (y = 0; y < 256; y++) {
        for (x = 0; x < 48; x++) {
            uint8_t data = ptr[counter++];
            for (b = 0; b < 8; ++b) {
                c = (data >> ( b)) & 0x01;
                screen[b + x * 8 + y * 384] = c ? 0xffffff : 0x0000f0;
            }
        }
    }
    dpy_gfx_update(s->con, 0, 0, 384, 256);
}

static void vga_invalidate(void *opaque)
{
}

static const GraphicHwOps profboard_vga_console_ops = {
    .invalidate  = vga_invalidate,
    .gfx_update  = vga_refresh,
};

static void profboard_vga_realize(DeviceState *dev, Error **errp)
{
    ProfBoardVGAState *s = PROFBOARD_VGA(dev);
    s->con = graphic_console_init(dev, 0, &profboard_vga_console_ops, s);
    qemu_console_resize(s->con, 384, 256);
}

static void profboard_vga_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->realize = profboard_vga_realize;
}

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
    }, {
        .name          = TYPE_PROFBOARD_VGA,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(ProfBoardVGAState),
        .instance_init = profboard_vga_init,
        .class_init    = profboard_vga_class_init,
    },
};

DEFINE_TYPES(profboard_machine_types)
