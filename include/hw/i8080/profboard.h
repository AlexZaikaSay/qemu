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

    MemoryRegion iomem;
    QemuConsole *con;
    uint8_t video_ram[0];
};