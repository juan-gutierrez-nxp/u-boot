// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019-2020 NXP
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
#include <asm/arch/s32-gen1/serdes_regs.h>
#include <asm/arch/s32-gen1/serdes_xpcs_regs.h>
#include <linux/string.h>
#include <clk.h>
#include <dt-bindings/clock/s32g274a-clock.h>
#include <s32gen1_clk_utils.h>
#include <dm/device_compat.h>

#include "pfeng.h"

static u32 emac_intf[PFENG_EMACS_COUNT] = {
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB)
	PHY_INTERFACE_MODE_SGMII, /* ARQ107 on PROC board */
	PHY_INTERFACE_MODE_RGMII, /* KSZ9031 on PROC board */
	PHY_INTERFACE_MODE_RGMII  /* SJA1105 on PLAT board */
#endif
#if CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
	PHY_INTERFACE_MODE_SGMII, /* SJA1110A */
	PHY_INTERFACE_MODE_NONE,  /* disabled, clashed with GMAC */
	PHY_INTERFACE_MODE_RGMII  /* KSZ9031 */
#endif
};

static u32 pfeng_mode = PFENG_MODE_DISABLE;

static u32 pfeng_intf_to_s32g(u32 intf);
static inline bool pfeng_emac_type_is_valid(u32 idx, u32 mode);
static void print_emacs_mode(char *label);

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

	emac_intf[idx] = mode;

	return true;
}

#define PART_PFE_NO	2

static void disable_partition_2(void)
{
	u32 val;

	/* Unlocking the RDC register */
	writel(readl(RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO)) |
			RDC_RD_CTRL_UNLOCK,
			RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO));
	/* Clearing XBAR_INTERFACE_DISABLE bit */
	writel(readl(RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO)) &
			~RDC_RD_INTERCONNECT_DISABLE,
	       RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO));
	while ((readl(RDC_RD_N_STATUS(RDC_BASE_ADDR, PART_PFE_NO)) &
		0x00000010))
		;
	/* Locking the register Write */
	writel(readl(RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO)) & 0x7FFFFFFF,
	       RDC_RD_N_CTRL(RDC_BASE_ADDR, PART_PFE_NO));

	/* Disabling MC_ME partition 2 clock */
	val = readl(MC_ME_PRTN_N_PCONF(MC_ME_BASE_ADDR, PART_PFE_NO));
	if (val & MC_ME_PRTN_N_PCE) {
		writel(readl(MC_ME_PRTN_N_PCONF(MC_ME_BASE_ADDR, PART_PFE_NO)) &
		       ~MC_ME_PRTN_N_PCE,
		       MC_ME_PRTN_N_PCONF(MC_ME_BASE_ADDR, PART_PFE_NO));
		writel(readl(MC_ME_PRTN_N_PUPD(MC_ME_BASE_ADDR, PART_PFE_NO)) |
		       MC_ME_PRTN_N_PCUD,
		       MC_ME_PRTN_N_PUPD(MC_ME_BASE_ADDR, PART_PFE_NO));
		writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
		writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
		while (readl(MC_ME_PRTN_N_STAT(MC_ME_BASE_ADDR, PART_PFE_NO))
				& MC_ME_PRTN_N_PCS)
			;
	}

	writel(readl(MC_ME_PRTN_N_PCONF(MC_ME_BASE_ADDR, PART_PFE_NO)) |
			MC_ME_PRTN_N_OSSE,
	       MC_ME_PRTN_N_PCONF(MC_ME_BASE_ADDR, PART_PFE_NO));
	writel(readl(MC_ME_PRTN_N_PUPD(MC_ME_BASE_ADDR, PART_PFE_NO)) |
			MC_ME_PRTN_N_OSSUD,
	       MC_ME_PRTN_N_PUPD(MC_ME_BASE_ADDR, PART_PFE_NO));
	writel(MC_ME_CTL_KEY_KEY, (MC_ME_BASE_ADDR));
	writel(MC_ME_CTL_KEY_INVERTEDKEY, (MC_ME_BASE_ADDR));
	while (!(readl(MC_ME_PRTN_N_STAT(MC_ME_BASE_ADDR, PART_PFE_NO)) &
				MC_ME_PRTN_N_OSSS))
		;

	/* Assert partition reset for PFE */
	writel(readl(RGM_PRST(MC_RGM_BASE_ADDR, PART_PFE_NO)) | 0x1,
	       RGM_PRST(MC_RGM_BASE_ADDR, PART_PFE_NO));
	while (!(readl(RGM_PSTAT(MC_RGM_BASE_ADDR, PART_PFE_NO)) & 0x1))
		;
}

/* Get xpcs and SerDes from MAC*/
int pfeng_map_emac_to_serdes_xpcs(int emac, int *serdes, int *xpcs)
{
	int emac_to_serdes[] = {1, 1, 0};
	enum serdes_xpcs_mode mode;
	int mac_serdes;

	if (emac >= ARRAY_SIZE(emac_intf)) {
		pr_err("invalid emac index %d\n", emac);
		return -ENXIO;
	}

	mac_serdes = emac_to_serdes[emac];
	mode = s32_get_xpcs_mode(mac_serdes);

	if (mode == SGMII_INAVALID) {
		pr_err("Emac %d not initialized\n", emac);
		return -ENXIO;
	}

	if ((mac_serdes == 0 && emac == 2) ||
	    (mac_serdes == 1 && emac == 1)) {
		switch (mode) {
		case SGMII_XPCS1:
		case SGMII_XPCS0_XPCS1:
			*xpcs = 1;
			break;
		default:
			return -ENXIO;
		}
	} else if (mac_serdes == 1 && emac == 0) {
		switch (mode) {
		case SGMII_XPCS0:
		case SGMII_XPCS0_XPCS1:
		case SGMII_XPCS0_2G5:
			*xpcs = 0;
			break;
		default:
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}
	*serdes = mac_serdes;

	return 0;
}

/* Get link status for sgmii EMAC */
int pfeng_serdes_wait_link(int emac)
{
	int serdes, xpcs;

	if (!pfeng_map_emac_to_serdes_xpcs(emac, &serdes, &xpcs))
		return s32_sgmii_wait_link(serdes, xpcs);

	return -ENXIO;
}

/* Check if SerDes is initialized for emac operation */
int pfeng_serdes_emac_is_init(int emac)
{
	int serdes, xpcs;

	if (!pfeng_map_emac_to_serdes_xpcs(emac, &serdes, &xpcs))
		return 0;

	pr_err("Invalid sgmii configuration for emac index %d\n", emac);
	return -ENXIO;
}

static void switch_pfe0_clock(int intf)
{
	u32 csel = 0;

	if (intf == PHY_INTERFACE_MODE_SGMII)
		csel = SGMII_CSEL;

	/* Extra switch driving TX_CLK for PFE_EMAC_0 */
	writel(csel, S32G_MAIN_GENCTRL1);
}

static int set_mac0_rx_tx_clk(struct udevice *pfe_dev, ulong mux4, ulong mux1)
{
	int ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX4, mux4);

	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux4 source\n");

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX1, mux1);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux1 source\n");

	return 0;
}

static int set_mac1_rx_tx_clk(struct udevice *pfe_dev, ulong mux5, ulong mux2)
{
	int ret;

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX5, mux5);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux5 source\n");

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX2, mux2);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux2 source\n");

	return 0;
}

static int set_mac2_rx_tx_clk(struct udevice *pfe_dev, ulong mux6, ulong mux3)
{
	int ret;

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX6, mux6);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux6 source\n");

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX3, mux3);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux2 source\n");

	return 0;
}

static void set_pfe_mac0_clk(int intf0, struct udevice *pfe_dev)
{
	int ret;

	switch (intf0) {
	case PHY_INTERFACE_MODE_SGMII:
		switch_pfe0_clock(PHY_INTERFACE_MODE_SGMII);

		ret = set_mac0_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_SERDES1_LANE0_CDR,
					 S32G274A_CLK_SERDES1_LANE0_TX);
		break;
	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if CONFIG_IS_ENABLED(FSL_PFENG_EMAC_0_RGMII)
		ret = set_mac0_rx_tx_clk(pfe_dev, S32G274A_CLK_PFE_MAC0_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
#endif
#endif
		break;
	case PHY_INTERFACE_MODE_RMII:
		ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX7,
						S32G274A_CLK_PFE_MAC0_EXT_REF);
		if (ret)
			dev_err(pfe_dev, "Failed to set cgm2_mux7 source\n");

		ret = s32gen1_enable_dev_clk("pfe_mac0_ref_div", pfe_dev);
		if (ret)
			dev_err(pfe_dev, "Failed to enable pfe_mac0_ref_div clock\n");

		ret = set_mac0_rx_tx_clk(pfe_dev, S32G274A_CLK_PFE_MAC0_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
		break;
	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;
	case PHY_INTERFACE_MODE_NONE:
		/* Don't set tx/rx clocks */
		return;
	default:
		ret = set_mac0_rx_tx_clk(pfe_dev,
					 S32GEN1_CLK_FIRC,
					 S32GEN1_CLK_FIRC);
		break;
	}

	ret = s32gen1_enable_dev_clk("pfe_mac0_rx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac0_rx clock\n");

	ret = s32gen1_enable_dev_clk("pfe_mac0_tx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac0_tx clock\n");
}

static void set_pfe_mac1_clk(int intf1, struct udevice *pfe_dev)
{
	int ret;

	switch (intf1) {
	case PHY_INTERFACE_MODE_SGMII:
		ret = set_mac1_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_SERDES1_LANE1_CDR,
					 S32G274A_CLK_SERDES1_LANE1_TX);
		break;

	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if CONFIG_IS_ENABLED(FSL_PFENG_EMAC_1_RGMII)
		ret = set_mac1_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_PFE_MAC1_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
#endif
#endif
		break;

	case PHY_INTERFACE_MODE_RMII:
		ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX8,
						S32G274A_CLK_PFE_MAC1_EXT_REF);
		if (ret)
			dev_err(pfe_dev, "Failed to set cgm2_mux8 source\n");

		ret = s32gen1_enable_dev_clk("pfe_mac1_ref_div", pfe_dev);
		if (ret)
			dev_err(pfe_dev, "Failed to enable pfe_mac1_ref_div clock\n");

		ret = set_mac1_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_PFE_MAC1_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
		break;
	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;
	case PHY_INTERFACE_MODE_NONE:
		/* Don't set tx/rx clocks */
		return;
	default:
		ret = set_mac1_rx_tx_clk(pfe_dev,
					 S32GEN1_CLK_FIRC,
					 S32GEN1_CLK_FIRC);
		break;
	}

	ret = s32gen1_enable_dev_clk("pfe_mac1_rx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac1_rx clock\n");

	ret = s32gen1_enable_dev_clk("pfe_mac1_tx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac1_tx clock\n");
}

static void set_pfe_mac2_clk(int intf2, struct udevice *pfe_dev)
{
	int ret;

	switch (intf2) {
	case PHY_INTERFACE_MODE_SGMII:
		ret = set_mac2_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_SERDES0_LANE1_CDR,
					 S32G274A_CLK_SERDES0_LANE1_TX);
		break;
	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if !CONFIG_IS_ENABLED(FSL_PFENG_EMAC_0_RGMII)
		ret = set_mac2_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_PFE_MAC2_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
#endif
#endif
		break;
	case PHY_INTERFACE_MODE_RMII:
		ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX9,
						S32G274A_CLK_PFE_MAC2_EXT_REF);
		if (ret)
			dev_err(pfe_dev, "Failed to set cgm2_mux8 source\n");

		ret = s32gen1_enable_dev_clk("pfe_mac2_ref_div", pfe_dev);
		if (ret)
			dev_err(pfe_dev, "Failed to enable pfe_mac2_ref_div clock\n");

		ret = set_mac2_rx_tx_clk(pfe_dev,
					 S32G274A_CLK_PFE_MAC2_EXT_RX,
					 S32GEN1_CLK_PERIPH_PLL_PHI5);
		break;
	case PHY_INTERFACE_MODE_MII:
		/* TODO */
		break;
	case PHY_INTERFACE_MODE_NONE:
		/* Don't set tx/rx clocks */
		return;
	default:
		ret = set_mac2_rx_tx_clk(pfe_dev,
					 S32GEN1_CLK_FIRC,
					 S32GEN1_CLK_FIRC);
		break;
	}

	ret = s32gen1_enable_dev_clk("pfe_mac2_rx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac2_rx clock\n");

	if (ret && intf2 == PHY_INTERFACE_MODE_RMII)
		dev_err(pfe_dev, "Failed to switch PFE2 RX clock, check the external PHY\n");

	ret = s32gen1_enable_dev_clk("pfe_mac2_tx", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_mac2_tx clock\n");
}

static void setup_pfe_clocks(int intf0, int intf1, int intf2,
			     struct udevice *pfe_dev)
{
	int ret;
	struct pfeng_priv *priv;

	if (!pfe_dev) {
		pr_err("%s: Invalid PFE device\n", __func__);
		return;
	}

	priv = dev_get_priv(pfe_dev);
	if (!priv) {
		pr_err("%s: Invalid PFE device data\n", __func__);
		return;
	}

	ret = s32gen1_set_parent_clk_id(S32G274A_CLK_MC_CGM2_MUX0,
					S32G274A_CLK_ACCEL_PLL_PHI1);
	if (ret)
		dev_err(pfe_dev, "Failed to set cgm2_mux0 source\n");

	ret = s32gen1_enable_dev_clk("pfe_pe", pfe_dev);
	if (ret)
		dev_err(pfe_dev, "Failed to enable pfe_pe clock\n");

	/* Apply clock setting to all EMAC ports for first time only.
	 * Otherwise setup clocks only on focused EMAC port
	 */
	if (!priv->clocks_done) {
		set_pfe_mac0_clk(intf0, pfe_dev);
		set_pfe_mac1_clk(intf1, pfe_dev);
		set_pfe_mac2_clk(intf2, pfe_dev);
		priv->clocks_done = true;
	} else if (priv->if_index == 0) {
		set_pfe_mac0_clk(intf0, pfe_dev);
	} else if (priv->if_index == 1) {
		set_pfe_mac1_clk(intf1, pfe_dev);
	} else if (priv->if_index == 2) {
		set_pfe_mac2_clk(intf2, pfe_dev);
	}
}

static void setup_iomux_pfe(int intf0, int intf1, int intf2)
{
	/* EMAC 0 */

	switch (intf0) {
	case PHY_INTERFACE_MODE_SGMII:
		/* SerDes_1 lane_0 */
		break;

	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if CONFIG_IS_ENABLED(FSL_PFENG_EMAC_0_RGMII)
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
#endif
#endif
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
		/* SerDes_1 lane_1 */
		break;

	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if CONFIG_IS_ENABLED(FSL_PFENG_EMAC_1_RGMII)
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
#endif
#endif
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
		/* SerDes_0 lane_1 */
		break;

	case PHY_INTERFACE_MODE_RGMII:
#if CONFIG_IS_ENABLED(TARGET_S32G274AEVB) || CONFIG_IS_ENABLED(TARGET_S32G274ARDB)
#if !CONFIG_IS_ENABLED(FSL_PFENG_EMAC_0_RGMII)
		/* set PF2 - MSCR[82] - for PFE MAC2 MDC*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PF2));

		/* set PE15 - MSCR[79] - for PFE MAC2 MDO*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_S32_G1_PFE_IN
		       | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE15));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_MDO_IN));

		/* set PH6 - MSCR[118] - for PFE MAC2 RXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH6));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RXD0_IN));

		/* set PH7 - MSCR[119] - for PFE MAC2 RXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH7));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RXD1_IN));

		/* set PH8 - MSCR[120] - for PFE MAC2 RXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH8));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RXD2_IN));

		/* set PH9 - MSCR[121] - for PFE MAC2 RXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH9));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RXD3_IN));

		/* set PH5 - MSCR[117] - for PFE MAC2 RX CTRL*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH5));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RXDV_IN));

		/* set PH4 - MSCR[116] - for PFE MAC2 RX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_IN,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH4));
		writel(SIUL2_MSCR_MUX_MODE_ALT3,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PFE_MAC2_RX_CLK_IN));

		/* set PJ0 - MSCR[144] - for PFE MAC2 TXD0*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PJ0));

		/* set PH1 - MSCR[113] - for PFE MAC2 TXD1*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH1));

		/* set PH2 - MSCR[114] - for PFE MAC2 TXD2*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH2));

		/* set PH3 - MSCR[115] - for PFE MAC2 TXD3*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH3));

		/* set PH10 - MSCR[122] - for PFE MAC2 TX CLK*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_1_MSCRn(SIUL2_MSCR_S32_G1_PH10));

		/* set PE14 - MSCR[78] - for PFE MAC2 TX EN*/
		writel(SIUL2_MSCR_S32_G1_PFE_OUT | SIUL2_MSCR_MUX_MODE_ALT2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE14));
#endif
#endif
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

/* setup all EMACs clocks */
void pfeng_apply_clocks(struct udevice *pfe_dev)
{
	setup_pfe_clocks(emac_intf[0], emac_intf[1], emac_intf[2], pfe_dev);
}

/* disable power for EMACs */
void pfeng_cfg_emacs_disable_all(void)
{
	writel(GPR_PFE_EMACn_PWR_DWN(0) |
	       GPR_PFE_EMACn_PWR_DWN(1) |
	       GPR_PFE_EMACn_PWR_DWN(2),
	       S32G_PFE_PRW_CTRL);
}

/* enable power for EMACs */
void pfeng_cfg_emacs_enable_all(void)
{
	int i;

	pfeng_cfg_emacs_disable_all();

	writel((pfeng_intf_to_s32g(emac_intf[2]) << 8) |
		(pfeng_intf_to_s32g(emac_intf[1]) << 4) |
		(pfeng_intf_to_s32g(emac_intf[0])),
		(unsigned long)S32G_PFE_EMACS_INTF_SEL);
	udelay(100);
	writel(0, S32G_PFE_PRW_CTRL);

	/* reset all EMACs */
	for (i = 0; i < PFENG_EMACS_COUNT; i++) {
		writel(readl((unsigned long)S32G_PFE_EMACn_MODE(i))
		       | EMAC_MODE_SWR_MASK,
		       (unsigned long)S32G_PFE_EMACn_MODE(i));
		udelay(10);
		while (readl((unsigned long)S32G_PFE_EMACn_MODE(i))
		       & EMAC_MODE_SWR_MASK)
			udelay(10);
	}
}

static int pfeng_cfg_mode_disable(void)
{
	/* disable all EMACs to allow interface change */
	pfeng_cfg_emacs_disable_all();

	/* disable partition 2 */
	disable_partition_2();

	return 0;
}

static int pfeng_cfg_mode_enable(struct udevice *pfe_dev)
{
	struct pfeng_priv *priv = pfe_dev ? dev_get_priv(pfe_dev) : NULL;

	if (!pfe_dev || !priv) {
		pr_err("%s: Invalid PFE device\n", __func__);
		return -EINVAL;
	}

	/* Setup PFE coherency for all HIFs */
	writel(PFE_COH_PORTS_MASK_HIF_0_3, S32G_PFE_COH_EN);

	/* Setup pins */
	setup_iomux_pfe(emac_intf[0], emac_intf[1], emac_intf[2]);

	/* Setup clocks */
	priv->clocks_done = false;
	setup_pfe_clocks(emac_intf[0], emac_intf[1], emac_intf[2], pfe_dev);

	return 0;
}

bool pfeng_cfg_set_mode(u32 mode, struct udevice *pfe_dev)
{
	int ret = EINVAL;

	if (pfeng_mode == mode)
		/* already in the same mode */
		return true;

	switch (mode) {
	case PFENG_MODE_DISABLE:
		ret = pfeng_cfg_mode_disable();
		break;
	case PFENG_MODE_ENABLE:
		ret = pfeng_cfg_mode_enable(pfe_dev);
		break;
	}

	if (!ret) {
		pfeng_mode = mode;
		print_emacs_mode(" PFE: ");
	}

	return !ret;
}

static bool parse_interface_name(char *modestr, int *intf)
{
	if (!strcmp(modestr, "mii"))
		*intf = PHY_INTERFACE_MODE_MII;
	else if (!strcmp(modestr, "rmii"))
		*intf = PHY_INTERFACE_MODE_RMII;
	else if (!strcmp(modestr, "rgmii"))
		*intf = PHY_INTERFACE_MODE_RGMII;
	else if (!strcmp(modestr, "sgmii"))
		*intf = PHY_INTERFACE_MODE_SGMII;
	else if (!strcmp(modestr, "none"))
		*intf = PHY_INTERFACE_MODE_NONE;
	else
		return false;
	return true;
}

int pfeng_set_emacs_from_env(char *env_mode)
{
	char *tok, *loc_mode;
	int i, intf[PFENG_EMACS_COUNT] = { -1, -1, -1 };

	loc_mode = strdup(env_mode);
	tok = strtok(loc_mode, ",");
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

	/* set INTF_SEL */
	writel((pfeng_intf_to_s32g(emac_intf[2]) << 8) |
		(pfeng_intf_to_s32g(emac_intf[1]) << 4) |
		(pfeng_intf_to_s32g(emac_intf[0])),
		S32G_PFE_EMACS_INTF_SEL);

	return 0;
}

/* command interface */

static inline bool pfeng_emac_type_is_valid(u32 idx, u32 mode)
{
	return (mode == PHY_INTERFACE_MODE_NONE ||
		mode == PHY_INTERFACE_MODE_SGMII ||
		mode == PHY_INTERFACE_MODE_RGMII ||
		mode == PHY_INTERFACE_MODE_RMII ||
		mode == PHY_INTERFACE_MODE_MII);
}

static u32 pfeng_intf_to_s32g(u32 intf)
{
	switch (intf) {
	case PHY_INTERFACE_MODE_MII:
		return GPR_PFE_EMAC_IF_MII;
	case PHY_INTERFACE_MODE_RMII:
		return GPR_PFE_EMAC_IF_RMII;
	case PHY_INTERFACE_MODE_RGMII:
		return GPR_PFE_EMAC_IF_RGMII;
	default:
		return GPR_PFE_EMAC_IF_SGMII; /* SGMII mode by default */
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
	printf("%semac0: %s emac1: %s emac2: %s\n", label,
	       pfeng_emac_get_interface_type_str(0),
	       pfeng_emac_get_interface_type_str(1),
	       pfeng_emac_get_interface_type_str(2));
}

const char *pfeng_cfg_get_mode_str(void)
{
	static const char *modes[3] = {
		"disable",
		"enable",
		"enable/run"
	};

	return modes[pfeng_cfg_get_mode()];
}

static int do_pfeng_cmd(cmd_tbl_t *cmdtp, int flag,
		       int argc, char * const argv[])
{
	char *env_mode = env_get(PFENG_ENV_VAR_MODE_NAME);
	struct udevice *pfe_dev = eth_get_dev_by_name("eth_pfeng");

	if (!env_mode) {
		/* set the default mode */
		env_set(PFENG_ENV_VAR_MODE_NAME,
			PFENG_MODE_DEFAULT == PFENG_MODE_ENABLE ?
			"enable" : "disable");
	}

	/* process command */
	if (!strcmp(argv[1], "info")) {
		char *env_fw = env_get(PFENG_ENV_VAR_FW_SOURCE);

		printf("PFE mode: %s\n", pfeng_cfg_get_mode_str());
		print_emacs_mode("  ");
		if (env_fw)
			printf("  fw: '%s' (from env)\n", env_fw);
		else
#if CONFIG_IS_ENABLED(FSL_PFENG_FW_LOC_SDCARD)
			printf("  fw: '%s' on mmc@%s\n",
			       CONFIG_FSL_PFENG_FW_NAME,
			       CONFIG_FSL_PFENG_FW_PART);
#else
			printf("  fw: on qspi@%s\n",
			       CONFIG_FSL_PFENG_FW_PART);
#endif
		return 0;
	} else if (!strcmp(argv[1], "disable")) {
		pfeng_cfg_set_mode(PFENG_MODE_DISABLE, pfe_dev);
		return 0;
	} else if (!strcmp(argv[1], "enable")) {
		pfeng_cfg_set_mode(PFENG_MODE_ENABLE, pfe_dev);
		return 0;
	} else if (!strcmp(argv[1], "stop")) {
		if (pfeng_cfg_get_mode() > PFENG_MODE_DISABLE) {
			/* we emulate STOP by DISABLE/ENABLE */
			pfeng_cfg_set_mode(PFENG_MODE_DISABLE, pfe_dev);
			pfeng_cfg_set_mode(PFENG_MODE_ENABLE, pfe_dev);
		}
		return 0;
	} else if (!strcmp(argv[1], "emacs")) {
		if (argc < 3) {
			print_emacs_mode("  ");
			return 0;
		} else if (!strcmp(argv[2], "reapply-clocks")) {
			setup_iomux_pfe(emac_intf[0], emac_intf[1],
					emac_intf[2]);
			pfeng_apply_clocks(pfe_dev);
			printf("PFE reapply clocks\n");
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
	/* for development only */
	} else if (!strcmp(argv[1], "debug")) {
		pfeng_debug();
		return 0;
	} else if (!strcmp(argv[1], "help")) {
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	   pfeng, 6, 0, do_pfeng_cmd,
	   "PFE controller info",
	   /*  */"info                              - important hw info\n"
	   "pfeng [disable|enable]                  - disable/enable full PFE/EMACs subsystem\n"
	   "pfeng stop                              - stop the driver but don't disable PFE/EMACs\n"
	   "pfeng emacs [<inf0-mode>,<intf1-mode>,<intf2-mode>] - read or set EMAC0-2 interface mode\n"
	   "pfeng emacs reapply-clocks              - reapply clock setting for all EMACs\n"
	   "pfeng reg <offset>                      - read register"
);
