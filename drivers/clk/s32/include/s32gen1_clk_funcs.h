/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020 NXP
 */
#ifndef S32GEN1_CLK_FUNCS_H
#define S32GEN1_CLK_FUNCS_H
#include <clk-uclass.h>
#include <inttypes.h>
#include <s32gen1_clk_modules.h>

struct s32gen1_clk *get_clock(uint32_t id);
struct s32gen1_clk *get_plat_clock(uint32_t id);
struct s32gen1_clk *get_plat_cc_clock(uint32_t id);
ulong s32gen1_set_rate(struct clk *c, ulong rate);
int s32gen1_set_parent(struct clk *c, struct clk *p);
int s32gen1_enable(struct clk *c);

ulong s32gen1_get_rate(struct clk *clk);
ulong get_module_rate(struct s32gen1_clk_obj *module,
		      struct s32gen1_clk_priv *priv);

void *get_base_addr(enum s32gen1_clk_source id, struct s32gen1_clk_priv *priv);

int pllclk2clk(u32 pll_clk_id, u32 *clk_id);
int get_pll_mfi_mfn(ulong pll_vco, ulong ref_freq, u32 *mfi, u32 *mfn);

#endif /* S32GEN1_CLK_FUNCS_H */
