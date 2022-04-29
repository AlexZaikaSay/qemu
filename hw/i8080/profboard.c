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
#include "exec/log.h"

#define ROM_FILE "root.rom"

#define KEY_ROWS 6
#define KEY_COLS 12

static uint8_t keys[6][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}; 

static uint8_t hp_key = 1;

enum I8255Regs
{
    PortA           = 0,
    PortB           = 1,
    PortC           = 2,
    ControlPort     = 3,
};

static uint8_t pa_col_indicies[] = { 4, 5, 6, 7, 8, 9, 10, 11 }; 

static uint8_t pb_row_indicies[] = { 0, 1, 2, 3, 4, 5 }; 

static void profboard_put_key(void *opaque, int keycode)
{
    uint8_t push = (keycode & 0x80) ? 0 : 1;
    switch (keycode) {
    case 0x39:  /* space */
        keys[5][6] = push;
        //return;
    }
    qemu_log("undefined profboard_put_key keycode = 0x%04x "
            "not implemented\n", keycode);
}

static void profboard_keyboard_init(void)
{
    qemu_add_kbd_event_handler(profboard_put_key, NULL);
}

static uint64_t get_port_a_state(void)
{
    int i = 0;
    int key_row = 0;
    uint64_t result = 0;
    for (; i < sizeof(pa_col_indicies); ++i) {
        for (key_row = 0; key_row < KEY_ROWS; ++key_row) {
            if (keys[key_row][pa_col_indicies[i]] == 1) {
                result |= 1;
                break;
            }
        }
        result <<= 1;
    }
    result >>= 1;
    return result;
}

static uint64_t get_port_b_state(void)
{
    int i = 0;
    int key_col = 0;
    uint64_t result = 0;
    uint8_t tape_in = 1;
    for (; i < sizeof(pb_row_indicies); ++i) {
        for (key_col = 0; key_col < KEY_COLS; ++key_col) {
            if (keys[pb_row_indicies[i]][key_col] == 1) {
                result |= 1;
                break;
            }
        }
        result <<= 1;
    }
    /* pb1 */
    if (hp_key) 
        result |= 1;
    result <<= 1;

    /* pb0 */
    if (tape_in) 
        result |= 1;

    return result;
}

static uint64_t profboard_i8255_regs_read(void *opaque, hwaddr addr, unsigned width)
{
    ProfBoardI8255State *state = opaque;
    
    switch (addr) {
    case PortA:
        if (state->group_a_mode == GROUP_A_MODE_SELECTION_0 &&
            state->group_a_dir == GROUP_A_DIR_INPUT)
            return get_port_a_state();
        break;
    case PortB:
        if (state->group_b_mode == GROUP_B_MODE_SELECTION_0 &&
            state->group_b_dir == GROUP_B_DIR_INPUT)
            return get_port_b_state();
        break;
    }
    
    qemu_log("undefined profboard_regs_read addr = 0x%04x, width = 0x%02x, "
            "not implemented\n", (uint16_t)state->regs.addr + (uint16_t)addr, (uint8_t)width);
    
    abort();
}

static void i8255_set_bsr_mode(ProfBoardI8255State *state, uint64_t val)
{
    qemu_log("undefined i8255_set_bsr_mode val = 0x%02x, "
        "not implemented\n", (uint8_t)val);
    abort();
}

static void i8255_set_io_mode(ProfBoardI8255State *state, uint64_t val)
{
    state->group_a_mode   = IO_MODE_SET(val, GROUP_A_MODE_SELECTION_ALL);
    state->group_b_mode   = IO_MODE_SET(val, GROUP_B_MODE_SELECTION_1);
    state->group_a_dir    = IO_MODE_SET(val, GROUP_A_DIR_INPUT);
    state->group_b_dir    = IO_MODE_SET(val, GROUP_B_DIR_INPUT);
    state->group_c_up_dir = IO_MODE_SET(val, GROUP_C_UP_DIR_INPUT);    
    state->group_c_lo_dir = IO_MODE_SET(val, GROUP_C_LO_DIR_INPUT);
}


static void profboard_i8255_regs_write(void *opaque, hwaddr addr, uint64_t val,
                              unsigned width)
{
    ProfBoardI8255State *state = opaque;
    
    switch (addr) {
    case ControlPort:
        state->mode = MODE_MASK(val);
        switch (state->mode) {
        case IO_MODE:
            i8255_set_io_mode(state, val);
            break;
        case BSR_MODE:
            i8255_set_bsr_mode(state, val);
            break;
        }
        return;
    }

    qemu_log("undefined profboard_regs_write addr = 0x%04x, val = 0x%02x, "
            "not implemented\n", (uint16_t)state->regs.addr + (uint16_t)addr, (uint8_t)val);

    abort();
}

static struct MemoryRegionOps profboard_i8255_regs = {
    .read = profboard_i8255_regs_read,
    .write = profboard_i8255_regs_write,
    .impl.min_access_size = 1,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void profboard_machine_init(MachineState *machine)
{
    Error *error_fatal;
    I8080_CPU(cpu_create(machine->cpu_type));
    const char* bios_name = machine->firmware ? machine->firmware : ROM_FILE;

    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *user_ram = g_new(MemoryRegion, 1);
    MemoryRegion *rom = g_new(MemoryRegion, 1);

    memory_region_init_ram(user_ram, NULL, "user.ram",  36 * KiB, &error_fatal);
    memory_region_init_rom(rom, NULL, "rom.rom", 14 * KiB, &error_fatal);

    memory_region_add_subregion(sysmem, 0x0000, user_ram);
    memory_region_add_subregion(sysmem, 0xc000, rom);

    if (load_image_mr(bios_name, rom) < 2 * KiB)
            error_report("Failed to load firmware '%s'.", bios_name);

    ProfBoardVGAState* vga = PROFBOARD_VGA(qdev_new(TYPE_PROFBOARD_VGA));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(vga), &error_fatal);
    
    memory_region_init_ram(&vga->video_mem, NULL, "screen.ram", 12 * KiB, &error_fatal);
    memory_region_add_subregion(sysmem, 0x9000, &vga->video_mem);

    ProfBoardI8255State* interface = PROFBOARD_I8255(qdev_new(TYPE_PROFBOARD_I8255));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(interface), &error_fatal);

    memory_region_init_io(&interface->regs, NULL, &profboard_i8255_regs, interface, "i8255regs.ram", 4);
    memory_region_add_subregion(sysmem, 0xff00, &interface->regs);

    profboard_keyboard_init();
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

static void profboard_i8255_init(Object *obj)
{
}

static void vga_refresh(void *opaque)
{
    int x, y;
    ProfBoardVGAState *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->con);
    uint8_t *ptr = memory_region_get_ram_ptr(&s->video_mem);
    uint32_t *screen = surface_data(surface);

    for (y = 0; y < 256; y++) {
        for (x = 0; x < 384; x++) {
            // TODO: optimize it infuture
            uint8_t data = ptr[((x / 8) << 8) + y];
            uint8_t r = (x % 8);
            uint8_t mask = 1 << (7 - r);
            uint8_t c = data & mask;
            screen[x + y * 384] = c ? 0xffffff : 0x0000f0;
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

static void profboard_i8255_class_init(ObjectClass *oc, void *data)
{
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
    }, {
        .name          = TYPE_PROFBOARD_I8255,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(ProfBoardI8255State),
        .instance_init = profboard_i8255_init,
        .class_init    = profboard_i8255_class_init,
    },
};

DEFINE_TYPES(profboard_machine_types)
