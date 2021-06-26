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
#include "cpu.h"
#include "exec/translator.h"

typedef struct DisasContext {
    DisasContextBase base;
} DisasContext;

static void i8080_tr_init_disas_context(DisasContextBase *dcbase,
                                          CPUState *cs)
{
}

static void i8080_tr_tb_start(DisasContextBase *db, CPUState *cpu)
{
}

static void i8080_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
}

static bool i8080_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                      const CPUBreakpoint *bp)
{
    return false;
}

static void i8080_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
}

static void i8080_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
}

static void i8080_tr_disas_log(const DisasContextBase *dcbase, CPUState *cpu)
{
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

void
restore_state_to_opc(CPUI8080State *env, TranslationBlock *tb,
                     target_ulong *data)
{
}