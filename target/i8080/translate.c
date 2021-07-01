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

static inline void gen_save_pc(target_ulong pc)
{
    tcg_gen_movi_tl(cpu_PC, pc);
}

static inline void gen_ld_16_imm(TCGv reg, target_ulong val)
{
    tcg_gen_movi_tl(reg, val);
}

static inline void gen_ld_8_imm(uint32_t regnum, TCGv val)
{
    TCGv dest = cpu_REGS[MASK_REG_PAIR(regnum)];
    if (MASK_LO_8_REG(regnum)) 
        tcg_gen_deposit_tl(dest, dest, val, 0, 8);
    else
        tcg_gen_deposit_tl(dest, dest, val, 8, 8);
}

static inline bool use_goto_tb(DisasContext *ctx, target_ulong dest)
{
    if (unlikely(ctx->base.singlestep_enabled)) {
        return false;
    }
    return (ctx->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
}

static inline void gen_goto_tb(DisasContext *ctx, int tb_num, target_ulong dest)
{
    if (use_goto_tb(ctx, dest))  {
        /* jump to same page: we can use a direct jump */
        tcg_gen_goto_tb(tb_num);
        gen_save_pc(dest);
        tcg_gen_exit_tb(ctx->base.tb, tb_num);
    } else {
        /* jump to another page */
        gen_save_pc(dest);
        tcg_gen_exit_tb(NULL, 0);
    }
}

static void gen_jmp(DisasContext *ctx, CPUI8080State *env, uint32_t abs)
{
    gen_goto_tb(ctx, 0, abs);
}

static void gen_jcc(DisasContext *ctx, CPUI8080State *env, uint32_t abs)
{
    // TODO: not implemented
    qemu_log("gen_jcc not implemented\n");
    abort();
}

static void decode_jmp(DisasContext *ctx, CPUI8080State *env, uint32_t abs)
{
    if (MASK_JUMP_IS_UNCOND(ctx->opcode)) 
        gen_jmp(ctx, env, abs);
    else
        gen_jcc(ctx, env, abs);
    ctx->base.is_jmp = DISAS_NORETURN;
}

static void decode_ld_16_imm(DisasContext *ctx, CPUI8080State *env, uint32_t val)
{
    gen_ld_16_imm(cpu_REGS[MASK_LD_IMM_16_REG(ctx->opcode)], val);
}

static void decode_ld_8_imm(DisasContext *ctx, CPUI8080State *env, uint32_t val)
{
    uint32_t regnum = MASK_LD_IMM_8_REG(ctx->opcode);
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
        gen_ld_8_imm(regnum, ctx->T0);
        break;
    }
}

static void decode_st_a(DisasContext *ctx, CPUI8080State *env, uint32_t addr)
{
    tcg_gen_movi_tl(ctx->T0, addr);
    tcg_gen_qemu_st8(cpu_A, ctx->T0, 0);
}

static void translate_1_byte_insn(DisasContext *ctx, CPUI8080State *env)
{
    uint32_t insn; 
    switch(ctx->opcode) {
    case OPC_LD_A_IMM:
    case OPC_LD_B_IMM:
    case OPC_LD_C_IMM:
    case OPC_LD_D_IMM:
    case OPC_LD_H_IMM:
    case OPC_LD_L_IMM:
        insn = cpu_ldub_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 2;
        decode_ld_8_imm(ctx, env, insn);
        break;
    case OPC_LD_BC_IMM:
    case OPC_LD_DE_IMM:
    case OPC_LD_HL_IMM:
    case OPC_LD_SP_IMM:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_ld_16_imm(ctx, env, insn);
        break;
    case OPC_ST_A:
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_st_a(ctx, env, insn);
        break;
    case OPC_JMP_IMM:      
        insn = cpu_lduw_code(env, ctx->base.pc_next + 1);
        ctx->pc_succ_insn = ctx->base.pc_next + 3;
        decode_jmp(ctx, env, insn);
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
