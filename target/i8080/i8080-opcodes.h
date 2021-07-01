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

#define MASK_8_REG(opcode)               (0x7 & opcode)
#define MASK_16_REG(opcode)              (0x3 & opcode)
#define MASK_IS_UNCOND(opcode)           (0x1 & opcode)
#define MASK_REG_PAIR(regnum)            (0x3 & (regnum >> 1))
#define MASK_LO_8_REG(regnum)            (0x1 & regnum)
#define MASK_MOV_IMM_8_REG(opcode)       (MASK_8_REG(opcode >> 3))
#define MASK_OP_16_REG(opcode)           (MASK_16_REG(opcode >> 4))

enum {
    OPC_MOV_BC_REF_A                                    = 0x02,
    OPC_MOV_DE_REF_A                                    = 0x12,
    OPC_MOV_A_BC_REF                                    = 0x0a,
    OPC_MOV_A_DE_REF                                    = 0x1a,    
    OPC_MOV_BC_IMM                                      = 0x01,
    OPC_MOV_DE_IMM                                      = 0x11,
    OPC_MOV_HL_IMM                                      = 0x21,
    OPC_MOV_SP_IMM                                      = 0x31,
    OPC_INC_BC                                          = 0x03,
    OPC_INC_DE                                          = 0x13,
    OPC_INC_HL                                          = 0x23,
    OPC_INC_SP                                          = 0x33,
    OPC_DEC_BC                                          = 0x0b,
    OPC_DEC_DE                                          = 0x1b,
    OPC_DEC_HL                                          = 0x2b,
    OPC_DEC_SP                                          = 0x3b,
    OPC_MOV_B_IMM                                       = 0x06,
    OPC_MOV_C_IMM                                       = 0x0e,
    OPC_MOV_D_IMM                                       = 0x16,
    OPC_MOV_E_IMM                                       = 0x1e,
    OPC_MOV_H_IMM                                       = 0x26,
    OPC_MOV_L_IMM                                       = 0x2e,
    OPC_MOV_A_IMM                                       = 0x3e,
    OPC_MOV_B_B                                         = 0x40,
    OPC_MOV_B_C                                         = 0x41,
    OPC_MOV_B_D                                         = 0x42,
    OPC_MOV_B_E                                         = 0x43,
    OPC_MOV_B_H                                         = 0x44,
    OPC_MOV_B_L                                         = 0x45,
    OPC_MOV_B_HL_REF                                    = 0x46,
    OPC_MOV_B_A                                         = 0x47,
    OPC_MOV_C_B                                         = 0x48,
    OPC_MOV_C_C                                         = 0x49,
    OPC_MOV_C_D                                         = 0x4a,
    OPC_MOV_C_E                                         = 0x4b,
    OPC_MOV_C_H                                         = 0x4c,
    OPC_MOV_C_L                                         = 0x4d,
    OPC_MOV_C_HL_REF                                    = 0x4e,
    OPC_MOV_C_A                                         = 0x4f,
    OPC_MOV_D_B                                         = 0x50,
    OPC_MOV_D_C                                         = 0x51,
    OPC_MOV_D_D                                         = 0x52,
    OPC_MOV_D_E                                         = 0x53,
    OPC_MOV_D_H                                         = 0x54,
    OPC_MOV_D_L                                         = 0x55,
    OPC_MOV_D_HL_REF                                    = 0x56,
    OPC_MOV_D_A                                         = 0x57,
    OPC_MOV_E_B                                         = 0x58,
    OPC_MOV_E_C                                         = 0x59,
    OPC_MOV_E_D                                         = 0x5a,
    OPC_MOV_E_E                                         = 0x5b,
    OPC_MOV_E_H                                         = 0x5c,
    OPC_MOV_E_L                                         = 0x5d,
    OPC_MOV_E_HL_REF                                    = 0x5e,
    OPC_MOV_E_A                                         = 0x5f,
    OPC_MOV_H_B                                         = 0x60,
    OPC_MOV_H_C                                         = 0x61,
    OPC_MOV_H_D                                         = 0x62,
    OPC_MOV_H_E                                         = 0x63,
    OPC_MOV_H_H                                         = 0x64,
    OPC_MOV_H_L                                         = 0x65,
    OPC_MOV_H_HL_REF                                    = 0x66,
    OPC_MOV_H_A                                         = 0x67,
    OPC_MOV_L_B                                         = 0x68,
    OPC_MOV_L_C                                         = 0x69,
    OPC_MOV_L_D                                         = 0x6a,
    OPC_MOV_L_E                                         = 0x6b,
    OPC_MOV_L_H                                         = 0x6c,
    OPC_MOV_L_L                                         = 0x6d,
    OPC_MOV_L_HL_REF                                    = 0x6e,
    OPC_MOV_L_A                                         = 0x6f,
    OPC_MOV_HL_REF_B                                    = 0x70,
    OPC_MOV_HL_REF_C                                    = 0x71,
    OPC_MOV_HL_REF_D                                    = 0x72,
    OPC_MOV_HL_REF_E                                    = 0x73,
    OPC_MOV_HL_REF_H                                    = 0x74,
    OPC_MOV_HL_REF_L                                    = 0x75,
    OPC_MOV_HL_REF_HL_REF                               = 0x76,
    OPC_MOV_HL_REF_A                                    = 0x77,
    OPC_MOV_A_B                                         = 0x78,
    OPC_MOV_A_C                                         = 0x79,
    OPC_MOV_A_D                                         = 0x7a,
    OPC_MOV_A_E                                         = 0x7b,
    OPC_MOV_A_H                                         = 0x7c,
    OPC_MOV_A_L                                         = 0x7d,
    OPC_MOV_A_HL_REF                                    = 0x7e,
    OPC_MOV_A_A                                         = 0x7f,    
    OPC_ST_A                                            = 0x32,
    OPC_JMP_IMM                                         = 0xc3,
    OPC_CALL_IMM                                        = 0xcd,
};


#endif /* TARGET_I8080_I8080_OPCODES_H */
