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

typedef struct DisasContext {
    DisasContextBase base;
    /* current opcode */
    uint32_t opcode;
    /* address of next instruction */
    uint32_t pc_succ_insn;

    /* TCG local temps */
    TCGv T0;
} DisasContext;


enum {
    /* 16-bit registers */
    REG_BC  = 0x00,
    REG_DE  = 0x01,
    REG_HL  = 0x02,
    REG_SP  = 0x03,
    
    /* 8-bit registers */

    REG_B   = 0x00,
    REG_C   = 0x01,
    REG_D   = 0x02,
    REG_E   = 0x03,
    REG_H   = 0x04,
    REG_L   = 0x05,
    REG_A   = 0x07,   
    
    MEM_REF = 0x06,

    REG_MAX = 4,
};

/*
 * TCG registers
 */
static TCGv cpu_PC;

/* GPR registers */
static TCGv cpu_A;
static TCGv cpu_REGS[4];

static const char *regnames[] = {
      "bc"  , "de"  , "hl"  , "sp",
    };

void i8080_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    I8080CPU *cpu = I8080_CPU(cs);
    CPUI8080State *env = &cpu->env;
    int i;

    qemu_fprintf(f, "pc: 0x%04x\n", env->pc);
    qemu_fprintf(f, " a: 0x%02x\n", env->a);
    for (i = 0 ; i < REG_MAX ; i++) 
        qemu_fprintf(f, "%s: 0x%04x, ", regnames[i], env->regs[i]);
    
    qemu_fprintf(f, "\n"); 
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

static bool i8080_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                      const CPUBreakpoint *bp)
{
    return false;
}

static bool is_1_byte_inst(uint32_t ins)
{
    // TODO: not inmplemented
    return true;
}

static inline void gen_store_16_val(TCGv reg, target_ulong val)
{
    tcg_gen_movi_tl(reg, val);
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

static void gen_call(DisasContext *ctx, uint32_t abs)
{
    tcg_gen_movi_tl(ctx->T0, ctx->pc_succ_insn);
    gen_push(ctx->T0);
    gen_goto_tb(ctx, 0, abs);
}

static void gen_jcc(DisasContext *ctx, uint32_t abs)
{
    // TODO: not implemented
    qemu_log("gen_jcc not implemented\n");
    abort();
}

static void gen_ccc(DisasContext *ctx, uint32_t abs)
{
    // TODO: not implemented
    qemu_log("gen_ccc not implemented\n");
    abort();
}

static void decode_jmp(DisasContext *ctx, uint32_t abs)
{
    if (MASK_IS_UNCOND(ctx->opcode)) 
        gen_jmp(ctx, abs);
    else
        gen_jcc(ctx, abs);
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

static void decode_mov_16_imm(DisasContext *ctx, uint32_t val)
{
    gen_store_16_val(cpu_REGS[MASK_OP_16_REG(ctx->opcode)], val);
}

static void decode_inc_16_r(DisasContext *ctx)
{
    TCGv dest = cpu_REGS[MASK_OP_16_REG(ctx->opcode)];
    tcg_gen_addi_tl(dest, dest, 1);
}

static void decode_dec_16_r(DisasContext *ctx)
{
    TCGv dest = cpu_REGS[MASK_OP_16_REG(ctx->opcode)];
    tcg_gen_subi_tl(dest, dest, 1);
}

static void decode_mov_8_imm(DisasContext *ctx, uint32_t val)
{
    uint32_t regnum = MASK_MOV_IMM_8_REG(ctx->opcode);
    switch(regnum) {
    case REG_A:
        tcg_gen_movi_tl(cpu_A, val);
        break;
    case MEM_REF:
        qemu_log("undefined instruction 0x%2x, mem ref\n", ctx->opcode);
        abort();
        break;
    default:
        tcg_gen_movi_tl(ctx->T0, val);
        gen_store_8_val(regnum, ctx->T0);
        break;
    }
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
    case OPC_MOV_A_IMM:
    case OPC_MOV_B_IMM:
    case OPC_MOV_C_IMM:
    case OPC_MOV_D_IMM:
    case OPC_MOV_H_IMM:
    case OPC_MOV_L_IMM:
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
    case OPC_ST_A:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_st_a(ctx, insn);
        break;
    case OPC_JMP_IMM:      
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_jmp(ctx, insn);
        break;
    case OPC_CALL_IMM:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_call(ctx, insn);
        break;
    default:
        qemu_log("undefined instruction 0x%2x, not implemented\n", ctx->opcode);
        abort();
    }
}

static void i8080_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    CPUI8080State *env = cpu->env_ptr;
    uint32_t ins = cpu_ldub_code(env, ctx->base.pc_next);
    if (is_1_byte_inst(ins)) {
        ctx->opcode = ins;
        translate_1_byte_insn(ctx, env);
    } else {
        // TODO: not implemented
        qemu_log("translate_2_byte_insn is not implemented\n");
    }
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
    cpu_PC = tcg_global_mem_new(cpu_env,
                          offsetof(CPUI8080State, pc), "pc");

    cpu_A = tcg_global_mem_new(cpu_env,
                          offsetof(CPUI8080State, a), "a");
    /* reg init */
    for (i = 0 ; i < REG_MAX ; i++) 
        cpu_REGS[i] = tcg_global_mem_new(cpu_env,
                                          offsetof(CPUI8080State, regs[i]),
                                          regnames[i]);
}
