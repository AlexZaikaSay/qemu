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

#ifndef QEMU_I8080_CPU_QOM_H
#define QEMU_I8080_CPU_QOM_H

#include "hw/core/cpu.h"
#include "qom/object.h"


#define TYPE_I8080_CPU "i8080-cpu"

OBJECT_DECLARE_TYPE(I8080CPU, I8080CPUClass,
                    I8080_CPU)

struct I8080CPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    DeviceReset parent_reset;
};


#endif /* QEMU_I8080_CPU_QOM_H */
