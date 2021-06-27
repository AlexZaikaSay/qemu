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


#include "hw/i8080/profboard.h"


static void profboard_machine_init(MachineState *machine)
{
    // ProfBoardMachineState *ms = PROFBOARD_MACHINE(machine);
    // ProfBoardMachineClass *amc = PROFBOARD_MACHINE_GET_CLASS(machine);
}

static void profboard_machine_class_init(ObjectClass *oc,
        void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    // ProfBoardMachineClass *amc = PROFBOARD_MACHINE_CLASS(oc);

    mc->init        = profboard_machine_init;
    mc->desc        = "Proficient PC";
    mc->is_default  = true;
    
    mc->max_cpus    = 1;
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
