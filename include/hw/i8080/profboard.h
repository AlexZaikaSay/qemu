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
#include "qapi/error.h"
#include "hw/boards.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qom/object.h"
#include "target/i8080/cpu.h"


#define TYPE_PROFBOARD_MACHINE MACHINE_TYPE_NAME("proficient")
typedef struct ProfBoardMachineState ProfBoardMachineState;
typedef struct ProfBoardMachineClass ProfBoardMachineClass;
DECLARE_OBJ_CHECKERS(ProfBoardMachineState, ProfBoardMachineClass,
                     PROFBOARD_MACHINE, TYPE_PROFBOARD_MACHINE)

#define TYPE_PROFBOARD_VGA "prof_vga"
OBJECT_DECLARE_SIMPLE_TYPE(ProfBoardVGAState, PROFBOARD_VGA)

#define TYPE_PROFBOARD_I8255 "prof_i8255"
OBJECT_DECLARE_SIMPLE_TYPE(ProfBoardI8255State, PROFBOARD_I8255)

struct ProfBoardMachineState {
    MachineState parent;
};

struct ProfBoardMachineClass {
    MachineClass parent_obj;

    const char *name;
    const char *desc;
};

struct ProfBoardVGAState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion video_mem;
    QemuConsole *con;
};

enum I8255Mode
{
    BSR_MODE    = 0x00,
    IO_MODE     = 0x80,
};

enum I8255ModeIO
{
    GROUP_A_MODE_SELECTION_0     = 0x00,
    GROUP_A_MODE_SELECTION_1     = 0x20,
    GROUP_A_MODE_SELECTION_2     = 0x40, /* or 0x60 */
    GROUP_A_MODE_SELECTION_ALL   = 0x60, 
    GROUP_A_DIR_OUTPUT           = 0x00,
    GROUP_A_DIR_INPUT            = 0x10,
    GROUP_B_MODE_SELECTION_0     = 0x00,
    GROUP_B_MODE_SELECTION_1     = 0x04,
    GROUP_B_DIR_OUTPUT           = 0x00,
    GROUP_B_DIR_INPUT            = 0x02,
    GROUP_C_UP_DIR_OUTPUT        = 0x00,
    GROUP_C_UP_DIR_INPUT         = 0x08,
    GROUP_C_LO_DIR_OUTPUT        = 0x00,
    GROUP_C_LO_DIR_INPUT         = 0x01,
};

#define MODE_MASK(val) (val & 0x80) 
#define IO_MODE_SET(val, mode) ((val & mode))

struct ProfBoardI8255State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion regs;

    uint8_t mode;
    uint8_t group_a_mode;
    uint8_t group_b_mode;
    uint8_t group_a_dir;
    uint8_t group_b_dir;
    uint8_t group_c_up_dir;
    uint8_t group_c_lo_dir;
};