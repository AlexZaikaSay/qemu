/*
 *  i8080 emulation for qemu: main translation routines.
 *
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
#include "tcg/tcg-op.h"
#include "cpu.h"
#include "i8080-opcodes.h"
#include "exec/translator.h"
#include "exec/log.h"
#include "qemu/qemu-print.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"

typedef struct DisasContext {
    DisasContextBase base;
    /* current opcode */
    uint32_t opcode;
    /* address of next instruction */
    uint32_t pc_succ_insn;

    /* TCG local temps */
    TCGv T0;
} DisasContext;

/*
 * TCG registers
 */
static TCGv cpu_PC;

/* GPR registers */
static TCGv cpu_A;
static TCGv cpu_REGS[4];

static TCGv cpu_FLAGS;

static const char *regnames[] = {
      "bc"  , "de"  , "hl"  , "sp",
    };

static inline void gen_store_16_val(TCGv reg, target_ulong val)
{
    tcg_gen_movi_tl(reg, val);
}

void i8080_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;
    int i;

    qemu_fprintf(f, "pc: 0x%04x\n", env->pc);
    qemu_fprintf(f, " a: 0x%02x\n", env->a);
    for (i = 0 ; i < REG_MAX ; i++) 
        qemu_fprintf(f, "%s: 0x%04x\n", regnames[i], env->regs[i]);
    
    qemu_fprintf(f, "flags: s: %d, z: %d, c: %d, p: %d, a: %d\n", 
        GET_S_FLAG(env->flags),
        GET_Z_FLAG(env->flags),
        GET_C_FLAG(env->flags),
        GET_P_FLAG(env->flags),
        GET_A_FLAG(env->flags));

}

static void i8080_tr_init_disas_context(DisasContextBase *dcbase,
                                          CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    ctx->opcode = 0;
    ctx->T0 = tcg_temp_new();
}

static void i8080_tr_tb_start(DisasContextBase *db, CPUState *cpu)
{
}

static void i8080_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    tcg_gen_insn_start(ctx->base.pc_next);
}

static void gen_debug_insn(DisasContext *s, uint32_t pc, int excp)
{
    gen_store_16_val(cpu_PC, pc);
    gen_helper_debug(cpu_env);
    s->base.is_jmp = DISAS_NORETURN;
}

static bool i8080_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                      const CPUBreakpoint *bp)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    gen_debug_insn(dc, dc->base.pc_next, EXCP_DEBUG);
    dc->base.pc_next += 2;
    dc->base.is_jmp = DISAS_NORETURN;
    return true;
}

static inline void gen_store_8_val(uint32_t regnum, TCGv src)
{
    TCGv dest = cpu_REGS[MASK_REG_PAIR(regnum)];
    if (MASK_LO_8_REG(regnum)) 
        tcg_gen_deposit_tl(dest, dest, src, 0, 8);
    else
        tcg_gen_deposit_tl(dest, dest, src, 8, 8);
}

static inline void gen_load_8_val(uint32_t regnum, TCGv dest)
{
    TCGv src = cpu_REGS[MASK_REG_PAIR(regnum)];
    if (MASK_LO_8_REG(regnum)) 
        tcg_gen_extract_tl(dest, src, 0, 8);
    else
        tcg_gen_extract_tl(dest, src, 8, 8);
}

static void gen_cond(DisasContext *ctx, uint32_t cond, TCGLabel *label)
{
    switch(cond) {
    case COND_NZ:
        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_FLAGS, Z_FLAG, label);
        break;
    case COND_Z:
        tcg_gen_brcondi_i32(TCG_COND_NE, cpu_FLAGS, Z_FLAG, label);
        break;
    case COND_NC:
        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_FLAGS, C_FLAG, label);
        break;
    case COND_C:
        tcg_gen_brcondi_i32(TCG_COND_NE, cpu_FLAGS, C_FLAG, label);
        break;
    case COND_PO:
        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_FLAGS, P_FLAG, label);
        break;
    case COND_PE:
        tcg_gen_brcondi_i32(TCG_COND_NE, cpu_FLAGS, P_FLAG, label);
        break;
    case COND_P:
        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_FLAGS, S_FLAG, label);
        break;
    case COND_M:
        tcg_gen_brcondi_i32(TCG_COND_NE, cpu_FLAGS, S_FLAG, label);
        break;
    }
}

static inline bool use_goto_tb(DisasContext *ctx, target_ulong dest)
{
    if (unlikely(ctx->base.singlestep_enabled)) {
        return false;
    }
    return (ctx->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
}

static inline void gen_push(TCGv val)
{
    tcg_gen_subi_tl(cpu_REGS[REG_SP], cpu_REGS[REG_SP], 2);
    tcg_gen_qemu_st16(val, cpu_REGS[REG_SP], 0);
}

static inline void gen_pop(TCGv val)
{
    tcg_gen_qemu_ld16u(val, cpu_REGS[REG_SP], 0);
    tcg_gen_addi_tl(cpu_REGS[REG_SP], cpu_REGS[REG_SP], 2);
}

static inline void gen_goto_tb(DisasContext *ctx, int tb_num, target_ulong dest)
{
    if (use_goto_tb(ctx, dest))  {
        /* jump to same page: we can use a direct jump */
        tcg_gen_goto_tb(tb_num);
        gen_store_16_val(cpu_PC, dest);
        tcg_gen_exit_tb(ctx->base.tb, tb_num);
    } else {
        /* jump to another page */
        gen_store_16_val(cpu_PC, dest);
        tcg_gen_exit_tb(NULL, 0);
    }
}

static void gen_jmp(DisasContext *ctx, uint32_t abs)
{
    gen_goto_tb(ctx, 0, abs);
}

static void gen_jmp_hl(DisasContext *ctx)
{
    tcg_gen_mov_tl(cpu_PC, cpu_REGS[REG_HL]);
    tcg_gen_exit_tb(NULL, 0);
}

static void gen_call(DisasContext *ctx, uint32_t abs)
{
    tcg_gen_movi_tl(ctx->T0, ctx->pc_succ_insn);
    gen_push(ctx->T0);
    gen_goto_tb(ctx, 0, abs);
}

static void gen_ret(DisasContext *ctx)
{
    gen_pop(cpu_PC);
    tcg_gen_exit_tb(NULL, 0);
}

static void gen_jcc(DisasContext *ctx, uint32_t abs)
{
    uint32_t cond = MASK_OP_COND(ctx->opcode);
    TCGLabel *label = gen_new_label();

    gen_cond(ctx, cond, label);
    gen_goto_tb(ctx, 0, abs);
    gen_set_label(label);
    gen_goto_tb(ctx, 1, ctx->pc_succ_insn);
}

static void gen_ccc(DisasContext *ctx, uint32_t abs)
{
    uint32_t cond = MASK_OP_COND(ctx->opcode);
    TCGLabel *label = gen_new_label();
    gen_cond(ctx, cond, label);
    gen_goto_tb(ctx, 0, ctx->pc_succ_insn);
    gen_set_label(label);
    tcg_gen_movi_tl(ctx->T0, ctx->pc_succ_insn);
    gen_push(ctx->T0);
    gen_goto_tb(ctx, 1, abs);
}

static void gen_rcc(DisasContext *ctx)
{
    uint32_t cond = MASK_OP_COND(ctx->opcode);
    TCGLabel *label = gen_new_label();
    
    gen_cond(ctx, cond, label);
    gen_goto_tb(ctx, 0, ctx->pc_succ_insn);
    gen_set_label(label);
    gen_pop(cpu_PC);
    tcg_gen_exit_tb(NULL, 0);
}

static void decode_jmp(DisasContext *ctx, uint32_t abs)
{
    if (MASK_IS_UNCOND(ctx->opcode)) 
        gen_jmp(ctx, abs);
    else
        gen_jcc(ctx, abs);
    ctx->base.is_jmp = DISAS_NORETURN;
}

static void decode_jmp_hl(DisasContext *ctx)
{
    gen_jmp_hl(ctx);
    ctx->base.is_jmp = DISAS_NORETURN;
}

static void decode_call(DisasContext *ctx, uint32_t abs)
{
    if (MASK_IS_UNCOND(ctx->opcode)) 
        gen_call(ctx, abs);
    else
        gen_ccc(ctx, abs);
    ctx->base.is_jmp = DISAS_NORETURN;
}

static void decode_ret(DisasContext *ctx)
{
    if (MASK_IS_UNCOND(ctx->opcode)) 
        gen_ret(ctx);
    else
        gen_rcc(ctx);
    ctx->base.is_jmp = DISAS_NORETURN;
}

static void decode_mov_16_imm(DisasContext *ctx, uint32_t val)
{
    gen_store_16_val(cpu_REGS[MASK_OP_16_REG(ctx->opcode)], val);
}

static void decode_inc_16_r(DisasContext *ctx)
{
    TCGv dest = cpu_REGS[MASK_OP_16_REG(ctx->opcode)];
    tcg_gen_addi_tl(dest, dest, 1);
    tcg_gen_andi_tl(dest, dest, 0xffff);
}

static void decode_dec_16_r(DisasContext *ctx)
{
    TCGv dest = cpu_REGS[MASK_OP_16_REG(ctx->opcode)];
    tcg_gen_subi_tl(dest, dest, 1);
    tcg_gen_andi_tl(dest, dest, 0xffff);
}

static void gen_load_temp(DisasContext *ctx, uint32_t regnum)
{
    switch(regnum) {
    case REG_A:
        tcg_gen_mov_tl(ctx->T0, cpu_A);
        break;
    case MEM_REF:
        tcg_gen_qemu_ld8u(ctx->T0, cpu_REGS[REG_HL], 0);
        break;
    default:
        gen_load_8_val(regnum, ctx->T0);
        break;
    }
}

static void gen_store_temp(DisasContext *ctx, uint32_t regnum)
{
    switch(regnum) {
    case REG_A:
        tcg_gen_mov_tl(cpu_A, ctx->T0);
        break;
    case MEM_REF:
        tcg_gen_qemu_st8(ctx->T0, cpu_REGS[REG_HL], 0);
        break;
    default:
        gen_store_8_val(regnum, ctx->T0);
        break;
    }
}

static void decode_mov_8_imm(DisasContext *ctx, uint32_t val)
{
    uint32_t regnum = MASK_MOV_IMM_8_REG(ctx->opcode);
    tcg_gen_movi_tl(ctx->T0, val);
    gen_store_temp(ctx, regnum);
}

static void decode_mov_8_r(DisasContext *ctx, CPUI8080State *env)
{
    uint32_t regsrc = MASK_8_REG(ctx->opcode);
    uint32_t regdest = MASK_8_REG(ctx->opcode >> 3);
    gen_load_temp(ctx, regsrc);
    gen_store_temp(ctx, regdest);
}

static void decode_st_a(DisasContext *ctx, uint32_t addr)
{
    tcg_gen_movi_tl(ctx->T0, addr);
    tcg_gen_qemu_st8(cpu_A, ctx->T0, 0);
}

static void decode_ld_a(DisasContext *ctx, uint32_t addr)
{
    tcg_gen_movi_tl(ctx->T0, addr);
    tcg_gen_qemu_ld8u(cpu_A, ctx->T0, 0);
}

static void decode_st_hl(DisasContext *ctx, uint32_t addr)
{
    tcg_gen_movi_tl(ctx->T0, addr);
    tcg_gen_qemu_st16(cpu_REGS[REG_HL], ctx->T0, 0);
}

static void decode_ld_hl(DisasContext *ctx, uint32_t addr)
{
    tcg_gen_movi_tl(ctx->T0, addr);
    tcg_gen_qemu_ld16u(cpu_REGS[REG_HL], ctx->T0, 0);
}

static void gen_zsp_flags(TCGv dest)
{
    TCGv flag = tcg_temp_new();
    tcg_gen_setcondi_tl(TCG_COND_EQ, flag, dest, 0);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, flag, Z_FLAG, 1);
    tcg_gen_setcondi_tl(TCG_COND_LT, flag, dest, 0);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, flag, S_FLAG, 1);
    tcg_gen_extract_tl(flag, dest, 0, 1);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, flag, P_FLAG, 1);
    tcg_temp_free(flag);
}

static void gen_zspc_flags(TCGv dest)
{
    gen_zsp_flags(dest);
    TCGv flag = tcg_temp_new();
    tcg_gen_extract_tl(flag, dest, 9, 1);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, flag, C_FLAG, 1);
    tcg_temp_free(flag);
}

static void decode_inc_8_r(DisasContext *ctx)
{
    uint32_t regnum = MASK_MOV_IMM_8_REG(ctx->opcode);
    gen_load_temp(ctx, regnum);
    tcg_gen_addi_tl(ctx->T0, ctx->T0, 1);
    tcg_gen_andi_tl(ctx->T0, ctx->T0, 0xff);
    gen_zsp_flags(ctx->T0);
    gen_store_temp(ctx, regnum);
}

static void decode_dec_8_r(DisasContext *ctx)
{
    uint32_t regnum = MASK_MOV_IMM_8_REG(ctx->opcode);
    gen_load_temp(ctx, regnum);
    tcg_gen_subi_tl(ctx->T0, ctx->T0, 1);
    tcg_gen_andi_tl(ctx->T0, ctx->T0, 0xff);
    gen_zsp_flags(ctx->T0);
    gen_store_temp(ctx, regnum);
}

static void decode_add(DisasContext *ctx)
{
    tcg_gen_add_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A); 
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff); 
}

static void decode_adc(DisasContext *ctx)
{
    tcg_gen_add_tl(cpu_A, cpu_A, ctx->T0);
    tcg_gen_extract_tl(ctx->T0, cpu_FLAGS, C_FLAG, 1);
    tcg_gen_add_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A);   
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff);
}

static void decode_sub(DisasContext *ctx)
{
    tcg_gen_sub_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A);
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff);  
}

static void decode_sbc(DisasContext *ctx)
{
    tcg_gen_sub_tl(cpu_A, cpu_A, ctx->T0);
    tcg_gen_extract_tl(ctx->T0, cpu_FLAGS, C_FLAG, 1);
    tcg_gen_sub_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A);
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff);
}

static void decode_and(DisasContext *ctx)
{
    tcg_gen_and_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A); 
}

static void decode_xor(DisasContext *ctx)
{
    tcg_gen_xor_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A); 
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff); 
}

static void decode_or(DisasContext *ctx)
{
    tcg_gen_or_tl(cpu_A, cpu_A, ctx->T0);
    gen_zspc_flags(cpu_A); 
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff); 
}

static void decode_cmp(DisasContext *ctx)
{
    tcg_gen_sub_tl(ctx->T0, cpu_A, ctx->T0);
    gen_zspc_flags(ctx->T0);
}

static void decode_cmp_r(DisasContext *ctx)
{
    uint32_t reg = MASK_8_REG(ctx->opcode);
    gen_load_temp(ctx, reg);
    decode_cmp(ctx);
}

static void decode_op(DisasContext *ctx)
{
    uint32_t operation =  MASK_OP_ARITH(ctx->opcode);
    switch(operation) {
    case ARITH_OP_ADD:
        decode_add(ctx);
        break;
    case ARITH_OP_ADC:
        decode_adc(ctx);
        break;
    case ARITH_OP_SUB:
        decode_sub(ctx);
        break;
    case ARITH_OP_SBC:
        decode_sbc(ctx);
        break;
    case ARITH_OP_AND:
        decode_and(ctx);
        break;
    case ARITH_OP_XOR:
        decode_xor(ctx);
        break;
    case ARITH_OP_OR:
        decode_or(ctx);
        break;
    case ARITH_OP_CMP:
        decode_cmp(ctx);
        break;
    }
}

static void decode_op_imm(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_movi_tl(ctx->T0, imm);
    decode_op(ctx);
}

static void decode_push_16_r(DisasContext *ctx)
{
    TCGv dest = cpu_REGS[MASK_OP_16_REG(ctx->opcode)];
    gen_push(dest);
}

static void decode_push_a_flags(DisasContext *ctx)
{
    tcg_gen_mov_tl(ctx->T0, cpu_FLAGS);
    tcg_gen_deposit_tl(ctx->T0, ctx->T0, cpu_A, 8, 8);
    gen_push(ctx->T0);
}

static void decode_not(DisasContext *ctx)
{
    tcg_gen_not_tl(cpu_A, cpu_A);
}

static void decode_rol(DisasContext *ctx)
{
    /* shift left */
    tcg_gen_shli_tl(cpu_A, cpu_A, 1);
    /* copy carry bit to low*/
    tcg_gen_extract_tl(ctx->T0, cpu_A, 8, 1);
    tcg_gen_deposit_tl(cpu_A, cpu_A, ctx->T0, 0, 1);
    /* clear to 8 bit */
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff);
    /* copy carry bit to carry flag */
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, ctx->T0, C_FLAG, 1);
}

static void decode_ror(DisasContext *ctx)
{
    /* copy low bit to up */
    tcg_gen_extract_tl(ctx->T0, cpu_A, 0, 1);
    tcg_gen_deposit_tl(cpu_A, cpu_A, ctx->T0, 8, 1);
    /* copy carry bit to carry flag */
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, ctx->T0, C_FLAG, 1);
    /* shift right */
    tcg_gen_shri_tl(cpu_A, cpu_A, 1);
}

static void decode_rlc(DisasContext *ctx)
{
    /* shift left */
    tcg_gen_shli_tl(cpu_A, cpu_A, 1);
    /* copy carry flag to low */
    tcg_gen_extract_tl(ctx->T0, cpu_FLAGS, C_FLAG, 1);
    tcg_gen_deposit_tl(cpu_A, cpu_A, ctx->T0, 0, 1);
    /* copy carry bit to carry flag */
    tcg_gen_extract_tl(ctx->T0, cpu_A, 8, 1);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, ctx->T0, C_FLAG, 1);
    /* clear to 8 bit */
    tcg_gen_andi_tl(cpu_A, cpu_A, 0xff);
}

static void decode_rrc(DisasContext *ctx)
{
    /* copy carry flag to up */
    tcg_gen_extract_tl(ctx->T0, cpu_FLAGS, C_FLAG, 1);
    tcg_gen_deposit_tl(cpu_A, cpu_A, ctx->T0, 8, 1);
    /* copy carry bit to carry flag */
    tcg_gen_extract_tl(ctx->T0, cpu_A, 0, 1);
    tcg_gen_deposit_tl(cpu_FLAGS, cpu_FLAGS, ctx->T0, C_FLAG, 1);
    /* shift right */
    tcg_gen_shri_tl(cpu_A, cpu_A, 1);
}

static void translate_1_byte_insn(DisasContext *ctx, CPUI8080State *env)
{
    uint32_t insn; 
    switch(ctx->opcode) {
    case OPC_MOV_B_B:
    case OPC_MOV_B_C:
    case OPC_MOV_B_D:
    case OPC_MOV_B_E:
    case OPC_MOV_B_H:
    case OPC_MOV_B_L:
    case OPC_MOV_B_HL_REF:
    case OPC_MOV_B_A:
    case OPC_MOV_C_B:
    case OPC_MOV_C_C:
    case OPC_MOV_C_D:
    case OPC_MOV_C_E:
    case OPC_MOV_C_H:
    case OPC_MOV_C_L:
    case OPC_MOV_C_HL_REF:
    case OPC_MOV_C_A:
    case OPC_MOV_D_B:
    case OPC_MOV_D_C:
    case OPC_MOV_D_D:
    case OPC_MOV_D_E:
    case OPC_MOV_D_H:
    case OPC_MOV_D_L:
    case OPC_MOV_D_HL_REF:
    case OPC_MOV_D_A:
    case OPC_MOV_E_B:
    case OPC_MOV_E_C:
    case OPC_MOV_E_D:
    case OPC_MOV_E_E:
    case OPC_MOV_E_H:
    case OPC_MOV_E_L:
    case OPC_MOV_E_HL_REF:
    case OPC_MOV_E_A:
    case OPC_MOV_H_B:
    case OPC_MOV_H_C:
    case OPC_MOV_H_D:
    case OPC_MOV_H_E:
    case OPC_MOV_H_H:
    case OPC_MOV_H_L:
    case OPC_MOV_H_HL_REF:
    case OPC_MOV_H_A:
    case OPC_MOV_L_B:
    case OPC_MOV_L_C:
    case OPC_MOV_L_D:
    case OPC_MOV_L_E:
    case OPC_MOV_L_H:
    case OPC_MOV_L_L:
    case OPC_MOV_L_HL_REF:
    case OPC_MOV_L_A:
    case OPC_MOV_HL_REF_B:
    case OPC_MOV_HL_REF_C:
    case OPC_MOV_HL_REF_D:
    case OPC_MOV_HL_REF_E:
    case OPC_MOV_HL_REF_H:
    case OPC_MOV_HL_REF_L:
    case OPC_MOV_HL_REF_HL_REF:
    case OPC_MOV_HL_REF_A:
    case OPC_MOV_A_B:
    case OPC_MOV_A_C:
    case OPC_MOV_A_D:
    case OPC_MOV_A_E:
    case OPC_MOV_A_H:
    case OPC_MOV_A_L:
    case OPC_MOV_A_HL_REF:
    case OPC_MOV_A_A: 
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_mov_8_r(ctx, env);
        break;
    case OPC_MOV_BC_REF_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        tcg_gen_qemu_st8(cpu_A, cpu_REGS[REG_BC], 0);
        break;
    case OPC_MOV_DE_REF_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        tcg_gen_qemu_st8(cpu_A, cpu_REGS[REG_DE], 0);
        break;
    case OPC_MOV_A_BC_REF:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        tcg_gen_qemu_ld8u(cpu_A, cpu_REGS[REG_BC], 0);
        break;
    case OPC_MOV_A_DE_REF:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        tcg_gen_qemu_ld8u(cpu_A, cpu_REGS[REG_DE], 0);
        break;
    case OPC_MOV_B_IMM:
    case OPC_MOV_C_IMM:
    case OPC_MOV_D_IMM:
    case OPC_MOV_H_IMM:
    case OPC_MOV_L_IMM:
    case OPC_MOV_HL_REF_IMM:
    case OPC_MOV_A_IMM:
        insn = cpu_ldub_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 2;
        decode_mov_8_imm(ctx, insn);
        break;
    case OPC_MOV_BC_IMM:
    case OPC_MOV_DE_IMM:
    case OPC_MOV_HL_IMM:
    case OPC_MOV_SP_IMM:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_mov_16_imm(ctx, insn);
        break;
    case OPC_NOT_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_not(ctx);
        break;
    case OPC_ROL:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_rol(ctx);
        break;
    case OPC_ROR:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_ror(ctx);
        break;
    case OPC_RLC:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_rlc(ctx);
        break;
    case OPC_RRC:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_rrc(ctx);
        break;
    case OPC_INC_BC:
    case OPC_INC_DE:
    case OPC_INC_HL:
    case OPC_INC_SP:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_inc_16_r(ctx);
        break;
    case OPC_DEC_BC:
    case OPC_DEC_DE:
    case OPC_DEC_HL:
    case OPC_DEC_SP:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_dec_16_r(ctx);
        break;
    case OPC_INC_B:
    case OPC_INC_C:
    case OPC_INC_D:
    case OPC_INC_E:
    case OPC_INC_H:
    case OPC_INC_L:
    case OPC_INC_HL_REF:
    case OPC_INC_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_inc_8_r(ctx);
        break;
    case OPC_DEC_B:
    case OPC_DEC_C:
    case OPC_DEC_D:
    case OPC_DEC_E:
    case OPC_DEC_H:
    case OPC_DEC_L:
    case OPC_DEC_HL_REF:
    case OPC_DEC_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_dec_8_r(ctx);
        break;
    case OPC_ST_A:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_st_a(ctx, insn);
        break;
    case OPC_LD_A:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_ld_a(ctx, insn);
        break;
    case OPC_ST_HL:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_st_hl(ctx, insn);
        break;
    case OPC_LD_HL:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_ld_hl(ctx, insn);
        break;
    case OPC_JMP_IMM:  
    case OPC_JMP_IMM_NZ:
    case OPC_JMP_IMM_Z:
    case OPC_JMP_IMM_NC:
    case OPC_JMP_IMM_C:
    case OPC_JMP_IMM_PO:
    case OPC_JMP_IMM_PE:
    case OPC_JMP_IMM_P:
    case OPC_JMP_IMM_M: 
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_jmp(ctx, insn);
        break;
    case OPC_JMP_HL_REF:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_jmp_hl(ctx);
        break;
    case OPC_CALL_NZ_IMM:
    case OPC_CALL_Z_IMM:
    case OPC_CALL_NC_IMM:
    case OPC_CALL_C_IMM:
    case OPC_CALL_PO_IMM:
    case OPC_CALL_PE_IMM:
    case OPC_CALL_P_IMM:
    case OPC_CALL_M_IMM:
    case OPC_CALL_IMM:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_call(ctx, insn);
        break;
    case OPC_RET_NZ:
    case OPC_RET_Z:
    case OPC_RET_NC:
    case OPC_RET_C:
    case OPC_RET_PO:
    case OPC_RET_PE:
    case OPC_RET_P:
    case OPC_RET_M:
    case OPC_RET:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_ret(ctx);
        break;
    case OPC_CMP_B:
    case OPC_CMP_C:
    case OPC_CMP_D:
    case OPC_CMP_E:
    case OPC_CMP_H:
    case OPC_CMP_L:
    case OPC_CMP_HL_REH:
    case OPC_CMP_A:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_cmp_r(ctx);
        break;
    case OPC_ADD_IMM:
    case OPC_ADC_IMM:
    case OPC_SUB_IMM:
    case OPC_SBC_IMM:
    case OPC_AND_IMM:
    case OPC_XOR_IMM:
    case OPC_OR_IMM:
    case OPC_CMP_IMM:
        insn = cpu_ldub_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 2;
        decode_op_imm(ctx, insn);
        break;
    case OPC_PUSH_BC:
    case OPC_PUSH_DE:
    case OPC_PUSH_HL:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_push_16_r(ctx);
        break;
    case OPC_PUSH_A_FLAGS:
        ctx->pc_succ_insn = ctx->base.pc_next + 1;
        decode_push_a_flags(ctx);
        break;
    default:
        qemu_log("undefined instruction pc = 0x%04x, op code = 0x%02x, "
            "not implemented\n", ctx->base.pc_next, ctx->opcode);
        abort();
    }
}

static void i8080_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    CPUI8080State *env = cpu->env_ptr;
    ctx->opcode = cpu_ldub_code(env, ctx->base.pc_next);
    translate_1_byte_insn(ctx, env);
    ctx->base.pc_next = ctx->pc_succ_insn;
}

static void i8080_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
}

static void i8080_tr_disas_log(const DisasContextBase *dcbase, CPUState *cpu)
{
    qemu_log("IN: %s\n", lookup_symbol(dcbase->pc_first));
    log_target_disas(cpu, dcbase->pc_first, dcbase->tb->size);
}

static const TranslatorOps i8080_tr_ops = {
    .init_disas_context = i8080_tr_init_disas_context,
    .tb_start           = i8080_tr_tb_start,
    .insn_start         = i8080_tr_insn_start,
    .breakpoint_check   = i8080_tr_breakpoint_check,
    .translate_insn     = i8080_tr_translate_insn,
    .tb_stop            = i8080_tr_tb_stop,
    .disas_log          = i8080_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext ctx;
    translator_loop(&i8080_tr_ops, &ctx.base, cs, tb, max_insns);
}

void restore_state_to_opc(CPUI8080State *env, TranslationBlock *tb,
                     target_ulong *data)
{
}

void i8080_tcg_init(void)
{
    int i = 0;
    /* pc init */
    cpu_PC = tcg_global_mem_new(cpu_env, offsetof(CPUI8080State, pc), "pc");

    cpu_A = tcg_global_mem_new(cpu_env, offsetof(CPUI8080State, a), "a");
    /* regs init */
    for (i = 0 ; i < REG_MAX ; i++) 
        cpu_REGS[i] = tcg_global_mem_new(cpu_env,
                                          offsetof(CPUI8080State, regs[i]),
                                          regnames[i]);

    /* flags init */
    cpu_FLAGS = tcg_global_mem_new(cpu_env, offsetof(CPUI8080State, flags), "flags");

}
