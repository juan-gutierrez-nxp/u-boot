// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <command.h>
#include <phy.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/soc.h>
#include <asm/io.h>

#include <asm/arch/mc_cgm_regs.h>
#include <asm/arch/mc_me_regs.h>
#include <asm/arch/mc_rgm_regs.h>

#include <linux/string.h>

#include "pfeng.h"

#define PFENG_ENV_VAR_MODE_NAME "pfeng_mode"
/*
 * pfeng_mode environment variable syntax:
 * <state>[,<emac0_interface_type>[,<emac1_intf_type>[,<emac2_intf_type>]]]
 *
 * where:
 * 	state			: enable/disable
 * 	emac_interface_type	: none/sgmii/rgmii/rmii/mii(+)
 * 				  (+) some might be unsupported on your board
 */

enum {
	PFENG_MODE_DISABLE = 0,
	PFENG_MODE_ENABLE,
};

static u32 emac_intf[PFENG_EMACS_COUNT] = {
	PHY_INTERFACE_MODE_NONE,
	PHY_INTERFACE_MODE_NONE,
	PHY_INTERFACE_MODE_NONE
};

static u32 new_intf[PFENG_EMACS_COUNT] = {
	PHY_INTERFACE_MODE_NONE,
	PHY_INTERFACE_MODE_NONE,
	PHY_INTERFACE_MODE_NONE
};

static u32 pfeng_mode = PFENG_MODE_DISABLE;

static u32 pfeng_intf_to_s32g(u32 intf);
static inline bool pfeng_emac_type_is_valid(u32 idx, u32 mode);
static void print_emacs_mode(char *label);

#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
u32 s32ccgmac_cfg_get_interface_mode(void);
#endif

/* pfeng cfg api */

u32 pfeng_cfg_get_mode(void)
{
	return pfeng_mode;
}

u32 pfeng_cfg_emac_get_interface(u32 idx)
{
	if (idx >= ARRAY_SIZE(emac_intf)) {
		pr_err("invalid emac index %d", idx);
		return PHY_INTERFACE_MODE_NONE;
	}
	return emac_intf[idx];
}

static bool pfeng_cfg_emac_set_interface(u32 idx, u32 mode)
{
	if (idx >= ARRAY_SIZE(emac_intf)) {
		pr_err("invalid emac index %d", idx);
		return false;
	}

	if (emac_intf[idx] == mode)
		/* already in the same mode */
		return true;

	if (!pfeng_emac_type_is_valid(idx, mode)) {
		pr_err("invalid interface mode for emac%i 0x%x", idx, mode);
		return false;
	}

#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
	if (idx == 1) {
		if (s32ccgmac_cfg_get_interface_mode() != PHY_INTERFACE_MODE_NONE &&
		    s32ccgmac_cfg_get_interface_mode() == emac_intf[1]) {
			pr_err("pfe: invalid config: MAC1 uses the same interface as s32ccgmac/eqos");
			return false;
		}
	}
#endif

	new_intf[idx] = mode;

	printf("pfe: warning: emac%i configuration has changed. You have to disable and enable PFE to get it ready\n", idx);

	return true;
}

#define PART_PFE_NO	2

static void disable_partition_2(void)
{
	u32 val;

	/* Unlocking the RDC register */
	writel(readl(RDC_RD_2_CTRL) | RDC_RD_CTRL_UNLOCK, RDC_RD_2_CTRL);
	/* Clearing XBAR_INTERFACE_DISABLE bit */
	writel(readl(RDC_RD_2_CTRL) & ~RDC_RD_INTERCONNECT_DISABLE,
	       RDC_RD_2_CTRL);
	while ((readl(RDC_RD_2_STAT) & 0x00000010))
		;
	/* Locking the register Write */
	writel(readl(RDC_RD_2_CTRL) & 0x7FFFFFFF, RDC_RD_2_CTRL);

	/* Disabling MC_ME partition 2 clock */
	val = readl(MC_ME_PRTN_N_PCONF(PART_PFE_NO));
	if (val & MC_ME_PRTN_N_PCE) {
		writel(readl(MC_ME_PRTN_N_PCONF(PART_PFE_NO)) &
		       ~MC_ME_PRTN_N_PCE, MC_ME_PRTN_N_PCONF(PART_PFE_NO));
		writel(readl(MC_ME_PRTN_N_PUPD(PART_PFE_NO)) |
		       MC_ME_PRTN_N_PCUD, MC_ME_PRTN_N_PUPD(PART_PFE_NO));
		writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
		writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
		while (readl(MC_ME_PRTN_N_STAT(PART_PFE_NO)) & MC_ME_PRTN_N_PCS)
			;
	}

	writel(readl(MC_ME_PRTN_N_PCONF(PART_PFE_NO)) | MC_ME_PRTN_N_OSSE,
	       MC_ME_PRTN_N_PCONF(PART_PFE_NO));
	writel(readl(MC_ME_PRTN_N_PUPD(PART_PFE_NO)) | MC_ME_PRTN_N_OSSUD,
	       MC_ME_PRTN_N_PUPD(PART_PFE_NO));
	writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
	writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
	//while (readl(MC_ME_PRTN_N_PUPD(PART_PFE_NO)) & ~MC_ME_PRTN_N_OSSE)
	while (readl(MC_ME_PRTN_N_STAT(PART_PFE_NO)) & MC_ME_PRTN_N_OSSS)
		;

	/* Lift partition reset for PFE */
	writel(readl(RGM_PRST(PART_PFE_NO)) | 0x1, RGM_PRST(PART_PFE_NO));
	while (readl(RGM_PSTAT(PART_PFE_NO)) & 0x1)
		;
}

static void enable_partition_2(void)
{
	/* Enabling PFE clocks (EMACs + TS) in partition 2 */
	writel(readl(MC_ME_PRTN_N_COFB0_CLKEN(PART_PFE_NO)) |
	       MC_ME_PFE_REQ_GROUP, MC_ME_PRTN_N_COFB0_CLKEN(PART_PFE_NO));
	writel(readl(MC_ME_MODE_UPD) | MC_ME_MODE_UPD_UPD, MC_ME_MODE_UPD);
	writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
	writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));

	/* Enabling MC_ME partition 2 clock */
	writel(readl(MC_ME_PRTN_N_PCONF(PART_PFE_NO)) | MC_ME_PRTN_N_PCE,
	       MC_ME_PRTN_N_PCONF(PART_PFE_NO));
	writel(readl(MC_ME_PRTN_N_PUPD(PART_PFE_NO)) | MC_ME_PRTN_N_PCUD,
	       MC_ME_PRTN_N_PUPD(PART_PFE_NO));
	writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
	writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
	while (!(readl(MC_ME_PRTN_N_STAT(PART_PFE_NO)) & MC_ME_PRTN_N_PCS))
		;

	/* Unlocking the RDC register */
	writel(readl(RDC_RD_2_CTRL) | RDC_RD_CTRL_UNLOCK, RDC_RD_2_CTRL);
	writel(readl(RDC_RD_2_CTRL) & ~RDC_RD_INTERCONNECT_DISABLE,
	       RDC_RD_2_CTRL);
	while ((readl(RDC_RD_2_STAT) & RDC_RD_INTERCONNECT_DISABLE_STAT))
		;

	/* Releasing partition reset for PFE */
	writel(readl(RGM_PRST(PART_PFE_NO)) & 0xFFFFFFFE,
	       RGM_PRST(PART_PFE_NO));

	/* Disabling OSSE */
	writel(readl(MC_ME_PRTN_N_PCONF(PART_PFE_NO)) & ~MC_ME_PRTN_N_OSSE,
	       MC_ME_PRTN_N_PCONF(PART_PFE_NO));
	writel(readl(MC_ME_PRTN_N_PUPD(PART_PFE_NO)) | MC_ME_PRTN_N_OSSUD,
	       MC_ME_PRTN_N_PUPD(PART_PFE_NO));
	writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
	writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
	while (readl(RGM_PSTAT(PART_PFE_NO)) & 0x1)
		;
	while (readl(MC_ME_PRTN_N_STAT(PART_PFE_NO)) & MC_ME_PRTN_N_OSSE)
		;
}

static void reset_partition_2(void)
{
	disable_partition_2();
	enable_partition_2();
}

static void setup_mux_clocks_pfe(int intf0, int intf1, int intf2)
{
	/* PFE MC_CGM clock MUX*/

	/* setup the mux clock divider for PFE_PE_CLK
	 * ( ACCEL_PLL_PHI1_FREQ = 1800Mhz / (2+1) = 600 MHz)
	 */
	mux_div_clk_config(MC_CGM2_BASE_ADDR, 0, 0, 0);
	mux_source_clk_config(MC_CGM2_BASE_ADDR, 0,
			      MC_CGM_MUXn_CSC_SEL_ACCEL_PLL_PHI1);

	/* EMAC 0 */

	/* setup the mux clock divider for PFE_MAC_0_RX_CLK
	 * ( PFE_MAC_0_EXT_RX_CLK)
	 */
	mux_source_clk_config(MC_CGM2_BASE_ADDR, 4,
			      MC_CGM_MUXn_CSC_SEL_PFE_MAC_0_EXT_RX_CLK);

	/* setup the mux clock divider for PFE_MAC_0_REF_DIV_CLK
	 * ( PFE_MAC_0_EXT_REF_CLK)
	 */
	mux_div_clk_config(MC_CGM2_BASE_ADDR, 7, 0, 0);
	mux_source_clk_config(MC_CGM2_BASE_ADDR, 7,
			      MC_CGM_MUXn_CSC_SEL_PFE_MAC_0_EXT_REF_CLK);

	switch (intf0) {
	case PHY_INTERFACE_MODE_RGMII:
		/* Tx clk generated by S32G, RX,REF clk received */
		/* PFE_MAC_0_TX_CLK ( PERIPH_PLL_PHI5 = 125Mhz) */
		mux_div_clk_config(MC_CGM2_BASE_ADDR, 1, 0, 0);
		mux_source_clk_config(MC_CGM2_BASE_ADDR, 1,
				      MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI5);
		break;

	case PHY_INTERFACE_MODE_SGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_GMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}

	/* EMAC 1 */

	/* setup the mux clock divider for PFE_MAC_1_RX_CLK
	 * ( PFE_MAC_1_EXT_RX_CLK)
	 */
	mux_source_clk_config(MC_CGM2_BASE_ADDR, 5,
			      MC_CGM_MUXn_CSC_SEL_PFE_MAC_1_EXT_RX_CLK);

	/* setup the mux clock divider for PFE_MAC_1_REF_DIV_CLK
	 * ( PFE_MAC_1_EXT_REF_CLK)
	 */
	mux_div_clk_config(MC_CGM2_BASE_ADDR, 8, 0, 0);
	mux_source_clk_config(MC_CGM2_BASE_ADDR, 8,
			      MC_CGM_MUXn_CSC_SEL_PFE_MAC_1_EXT_REF_CLK);

	switch (intf1) {
	case PHY_INTERFACE_MODE_SGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RGMII:

		/* Tx clk generated by S32G, RX,REF clk received */
		/* PFE_MAC_1_TX_CLK ( PERIPH_PLL_PHI5 = 125Mhz) */
		mux_div_clk_config(MC_CGM2_BASE_ADDR, 2, 0, 0);
		mux_source_clk_config(MC_CGM2_BASE_ADDR, 2,
				      MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI5);
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}

	/* EMAC 2 */

	switch (intf2) {
	case PHY_INTERFACE_MODE_SGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}
}

static void setup_iomux_pfe(int intf0, int intf1, int intf2)
{
	/* EMAC 0 */

	switch (intf0) {
	case PHY_INTERFACE_MODE_SGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RGMII:
		/* set PF2 - MSCR[82] - for PFE MAC0 MDC*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PF2));

		/* set PE15 - MSCR[79] - for PFE MAC0 MDO*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_S32_G1_PFE_IN
		       | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE15));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_MDO_IN));

		/* set PH6 - MSCR[118] - for PFE MAC0 RXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH6));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RXD0_IN));

		/* set PH7 - MSCR[119] - for PFE MAC0 RXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH7));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RXD1_IN));

		/* set PH8 - MSCR[120] - for PFE MAC0 RXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH8));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RXD2_IN));

		/* set PH9 - MSCR[121] - for PFE MAC0 RXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH9));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RXD3_IN));

		/* set PH5 - MSCR[117] - for PFE MAC0 RX CTRL*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH5));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RXDV_IN));

		/* set PH4 - MSCR[116] - for PFE MAC0 RX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH4));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC0_RX_CLK_IN));

		/* set PJ0 - MSCR[144] - for PFE MAC0 TXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PJ0));

		/* set PH1 - MSCR[113] - for PFE MAC0 TXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH1));

		/* set PH2 - MSCR[114] - for PFE MAC0 TXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH2));

		/* set PH3 - MSCR[115] - for PFE MAC0 TXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH3));

		/* set PH10 - MSCR[122] - for PFE MAC0 TX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH10));

		/* set PE14 - MSCR[78] - for PFE MAC0 TX EN*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT1,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE14));
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}

	/* EMAC 1 */

	switch (intf1) {
	case PHY_INTERFACE_MODE_SGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RGMII:
		/* set PD12 - MSCR[60] - for PFE MAC1 MDC*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD12));

		/* set PD13 - MSCR[61] - for PFE MAC1 MDO*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_S32_G1_PFE_IN
		       | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD13));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_MDI_IN));

		/* set PE10 - MSCR[74] - for PFE MAC1 RXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE10));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RXD0_IN));

		/* set PE11 - MSCR[75] - for PFE MAC1 RXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE11));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RXD1_IN));

		/* set PE12 - MSCR[76] - for PFE MAC1 RXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE12));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RXD2_IN));

		/* set PE13 - MSCR[77] - for PFE MAC1 RXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE13));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RXD3_IN));

		/* set PE9 - MSCR[73] - for PFE MAC1 RX CTRL*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE9));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RXDV_IN));

		/* set PE8 - MSCR[72] - for PFE MAC1 RX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE8));
		writel(SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC1_RX_CLK_IN));

		/* set PE4 - MSCR[68] - for PFE MAC1 TXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE4));

		/* set PE5 - MSCR[69] - for PFE MAC1 TXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE5));

		/* set PE6 - MSCR[70] - for PFE MAC1 TXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE6));

		/* set PE7 - MSCR[71] - for PFE MAC1 TXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE7));

		/* set PE2 - MSCR[66] - for PFE MAC1 TX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE2));

		/* set PE3 - MSCR[67] - for PFE MAC1 TX EN*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE3));

		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}

	/* EMAC 2 */

	switch (intf2) {
	case PHY_INTERFACE_MODE_SGMII:
		/* invalid setting  - SGMII available for MAC0, MAC1 only */
		break;

	case PHY_INTERFACE_MODE_RGMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;

	default:
		break;
	}
}

bool pfeng_cfg_set_mode(u32 mode)
{
	if (pfeng_mode == mode)
		/* already in the same mode */
		return true;

	if (mode == PFENG_MODE_DISABLE) {
		/* power down all 3 macs */
		/* TODO */

		/* disable partition 2 */
		disable_partition_2();

	} else {
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
		if (s32ccgmac_cfg_get_interface_mode() != PHY_INTERFACE_MODE_NONE &&
		    s32ccgmac_cfg_get_interface_mode() == emac_intf[1]) {
			pr_info("pfe: warning: MAC1 uses the same interface as s32ccgmac/eqos");
		}
#endif
		/* all checks passed, so copy new_intf[] to emac_intf[] */
		memcpy(emac_intf, new_intf, sizeof(emac_intf));

		/* enable partition 2 */
		enable_partition_2();
		setup_iomux_pfe(emac_intf[0], emac_intf[1], emac_intf[2]);
		setup_mux_clocks_pfe(emac_intf[0], emac_intf[1], emac_intf[2]);

		/* INTF_SEL */
		writel((pfeng_intf_to_s32g(emac_intf[2]) << 8) |
			(pfeng_intf_to_s32g(emac_intf[1]) << 4) |
			(pfeng_intf_to_s32g(emac_intf[0])),
			(u32 *)S32G_PFE_EMACS_INTF_SEL);
		reset_partition_2();

		/* power up all 3 macs */
		/* TODO */

		print_emacs_mode("PFE: ");
	}

	pfeng_mode = mode;

	return true;
}

static bool parse_interface_name(char *modestr, int *intf)
{
	if (!strcmp(modestr, "mii"))
		*intf = PHY_INTERFACE_MODE_MII;
	else if (!strcmp(modestr, "rmii"))
		*intf = PHY_INTERFACE_MODE_RMII;
	else if (!strcmp(modestr, "rgmii"))
		*intf = PHY_INTERFACE_MODE_RGMII;
	else if (!strcmp(modestr, "gmii"))
		*intf = PHY_INTERFACE_MODE_GMII;
	else if (!strcmp(modestr, "sgmii"))
		*intf = PHY_INTERFACE_MODE_SGMII;
	else if (!strcmp(modestr, "none"))
		*intf = PHY_INTERFACE_MODE_NONE;
	else
		return false;
	return true;
}

static int pfeng_set_emacs_from_env(char *env_mode)
{
	char *tok;
	int i, intf[PFENG_EMACS_COUNT] = { -1, -1, -1 };

	tok = strtok(env_mode, ",");
	for (i = 0; i < PFENG_EMACS_COUNT; i++) {
		if (!tok)
			break;
		if (!parse_interface_name(tok, &intf[i])) {
			pr_err("invalid interface name for emac%d", i);
			return CMD_RET_USAGE;
		}
		tok = strtok(NULL, ",");
	}

	for (i = 0; i < PFENG_EMACS_COUNT; i++)
		if (intf[i] > -1)
			pfeng_cfg_emac_set_interface(i, intf[i]);
	return 0;
}

void pfeng_init_s32g(void)
{
	char *env_mode = env_get(PFENG_ENV_VAR_MODE_NAME);
	char *tok;

	if (!env_mode || !strlen(env_mode)) {
		pfeng_cfg_set_mode(PFENG_MODE_DISABLE);
		return;
	}

	tok = strchr(env_mode, ',');
	if (tok)
		*tok = '\0';
	if (!strcmp("disable", env_mode)) {
		pfeng_cfg_set_mode(PFENG_MODE_DISABLE);
		return;
	} else {
		if (strcmp("enable", env_mode)) {
			/* not "disable" nor "enable" so do default */
			pfeng_cfg_set_mode(PFENG_MODE_DISABLE);
			return;
		}
	}

	/* jump to emacs part */
	tok++;
	if (strlen(tok)) {
		if(!pfeng_set_emacs_from_env(tok)) {
			pfeng_cfg_set_mode(PFENG_MODE_ENABLE);
			return;
		}
	}

	/* default */
	pfeng_cfg_set_mode(PFENG_MODE_DISABLE);
}

/* command interface */

static inline bool pfeng_emac_type_is_valid(u32 idx, u32 mode)
{
	return (mode == PHY_INTERFACE_MODE_NONE ||
		mode == PHY_INTERFACE_MODE_GMII ||
		mode == PHY_INTERFACE_MODE_SGMII ||
		mode == PHY_INTERFACE_MODE_RGMII ||
		mode == PHY_INTERFACE_MODE_GMII ||
		mode == PHY_INTERFACE_MODE_MII);
}

static u32 pfeng_intf_to_s32g(u32 intf)
{
	switch (intf) {
	case PHY_INTERFACE_MODE_MII:
		return 0x1;
	case PHY_INTERFACE_MODE_RMII:
		return 0x9;
	case PHY_INTERFACE_MODE_RGMII:
		return 0x2;
	default:
		return 0x0; /* SGMII mode by default */
	}
}

static const char *pfeng_emac_get_interface_type_str(u32 idx)
{
	if (idx >= ARRAY_SIZE(emac_intf)) {
		pr_err("invalid emac index %d", idx);
		return "<invalid>";
	}

	return strlen(phy_string_for_interface(emac_intf[idx])) ?
	       phy_string_for_interface(emac_intf[idx]) : "none";
}

static void print_emacs_mode(char *label)
{
	bool flg;

	flg = emac_intf[0] != new_intf[0] || emac_intf[1] != new_intf[1] ||
	      emac_intf[2] != new_intf[2];
	printf("%semac0: %s emac1: %s emac2: %s %s\n", label,
			pfeng_emac_get_interface_type_str(0),
			pfeng_emac_get_interface_type_str(1),
			pfeng_emac_get_interface_type_str(2),
			(flg && (pfeng_cfg_get_mode() != PFENG_MODE_DISABLE))
			? "note: cycle disable/enable to get new config ready"
			  : "");
}

static int do_pfeng_cmd(cmd_tbl_t *cmdtp, int flag,
		       int argc, char * const argv[])
{
	char *env_mode = env_get(PFENG_ENV_VAR_MODE_NAME);

	if (!env_mode) {
		/* set the default mode */
		env_set(PFENG_ENV_VAR_MODE_NAME, "disable");
	}

	/* process command */
	if (!strcmp(argv[1], "info")) {
		printf("PFE mode: %s\n",
				pfeng_cfg_get_mode() == PFENG_MODE_DISABLE ?
				"disabled" : "enabled");
		print_emacs_mode("  ");
		return 0;
	} else if (!strcmp(argv[1], "disable")) {
		pfeng_cfg_set_mode(PFENG_MODE_DISABLE);
		return 0;
	} else if (!strcmp(argv[1], "enable")) {
		pfeng_cfg_set_mode(PFENG_MODE_ENABLE);
		return 0;
	} else if (!strcmp(argv[1], "emacs")) {
		if (argc < 3) {
			print_emacs_mode("  ");
			return 0;
		} else {
			/* parse argv[2] for "rgmii,none,mii" */
			if (pfeng_set_emacs_from_env(argv[2]))
				return CMD_RET_USAGE;
			return 0;
		}
	} else if (!strcmp(argv[1], "reg")) {
		u32 reg, offs = 0;

		if (argc != 3)
			return CMD_RET_USAGE;

		offs = simple_strtoul(argv[2], NULL, 16);
		reg = readl(((void *)(S32G_PFE_REGS_BASE)) + offs);
		printf("reg 0x%x at 0x%p: %08x\n", offs,
		       ((void *)(S32G_PFE_REGS_BASE)) + offs,
		       reg);
		return 0;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	   pfeng, 6, 0, do_pfeng_cmd,
	   "PFE controller info",
	   /*  */"info                              - important hw info\n"
	   "pfeng [disable|enable]                  - disable/enable full PFE/EMACs subsystem\n"
	   "pfeng emacs [<inf0-mode>,<intf1-mode>,<intf2-mode>] - read or set EMAC0-2 interface mode\n"
	   "pfeng reg <offset>                      - read register"
);

