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

#ifndef TARGET_I8080_I8080_OPCODES_H
#define TARGET_I8080_I8080_OPCODES_H

#define MASK_JUMP_IS_UNCOND(opcode) (0x1 & opcode)
#define MASK_REG_PAIR(regnum) (0x3 & (regnum >> 1))
#define MASK_LO_8_REG(regnum) (0x1 & regnum)
#define MASK_LD_IMM_8_REG(opcode) (0x7 & (opcode >> 3))
#define MASK_LD_IMM_16_REG(opcode) (0x3 & (opcode >> 4))

enum {
    OPC_LD_BC_IMM                                       = 0x01,
    OPC_LD_DE_IMM                                       = 0x11,
    OPC_LD_HL_IMM                                       = 0x21,
    OPC_LD_SP_IMM                                       = 0x31,
    OPC_LD_B_IMM                                        = 0x06,
    OPC_LD_C_IMM                                        = 0x0e,
    OPC_LD_D_IMM                                        = 0x16,
    OPC_LD_E_IMM                                        = 0x1e,
    OPC_LD_H_IMM                                        = 0x26,
    OPC_LD_L_IMM                                        = 0x2e,
    OPC_LD_A_IMM                                        = 0x3e,
    OPC_ST_A                                            = 0x32,
    OPC_JMP_IMM                                         = 0xc3,
};


#endif /* TARGET_I8080_I8080_OPCODES_H */
