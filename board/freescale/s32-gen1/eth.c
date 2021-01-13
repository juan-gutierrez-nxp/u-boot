// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2020 NXP
 */
#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <net.h>
#ifndef CONFIG_DM_ETH
#include <netdev.h>
#endif
#include <phy.h>
#include <malloc.h>
#include <asm/types.h>
#include <asm/arch/soc.h>
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
#include <dm/platform_data/dwc_eth_qos_dm.h>
#endif
#if CONFIG_IS_ENABLED(FSL_PFENG)
#include <dm/platform_data/pfeng_dm_eth.h>
#endif
#include <fdt_support.h>
#include <clk.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#include <s32gen1_clk_utils.h>
#include <s32gen1_gmac_utils.h>
#include <dm/device_compat.h>

static void ft_update_eth_addr_by_name(const char *name, const u8 idx,
				       void *fdt, int nodeoff)
{
	u8 ea[ARP_HLEN];

	if (eth_env_get_enetaddr_by_index(name, idx, ea)) {
		fdt_setprop(fdt, nodeoff, "local-mac-address", ea, ARP_HLEN);
		printf("DT: %s%i set to %pM\n", name, idx, ea);
	}
}

#if CONFIG_IS_ENABLED(FSL_PFENG)
static bool intf_is_xmii(u32 intf)
{
	return intf == PHY_INTERFACE_MODE_MII ||
		intf == PHY_INTERFACE_MODE_RMII ||
		intf == PHY_INTERFACE_MODE_RGMII;
}

static void ft_enet_pfe_emac_fixup(u32 idx, void *fdt)
{
	int nlen = 0, nodeoff = -1;
	char *ifname;
	char reqname[8];

	sprintf(reqname, "pfe%i", idx);

	while (1) {

		nodeoff = fdt_node_offset_by_compatible(fdt, nodeoff, "fsl,pfeng-logif");
		if (nodeoff < 0)
			return;

		ifname = (char *)fdt_getprop(fdt, nodeoff, "fsl,pfeng-if-name", &nlen);
		if (!ifname || !nlen)
			continue;

		if (strncmp(reqname, ifname, strlen(reqname)))
			continue;

		if (pfeng_cfg_emac_get_interface(idx) == PHY_INTERFACE_MODE_NONE) {
			printf("DT: Disabling PFE_EMAC_%i\n", idx);
			fdt_status_disabled(fdt, nodeoff);
		} else {
			printf("DT: Enabling PFE_EMAC_%i\n", idx);
			fdt_status_okay(fdt, nodeoff);

			/* sync MAC HW addr to DT [local-mac-address] */
			ft_update_eth_addr_by_name("pfe", idx, fdt, nodeoff);
		}
		/* We are done */
		return;
	}
}

static bool pfeng_drv_status_active(void)
{
	struct udevice *dev;

	if (uclass_get_device_by_name(UCLASS_ETH, "eth_pfeng", &dev))
		return false;

	return dev->flags & DM_FLAG_ACTIVATED;
}
#endif

/*
 * Ethernet DT fixup before OS load
 *
 */
void ft_enet_fixup(void *fdt)
{
	int __maybe_unused nodeoff;
	bool __maybe_unused ena;

	/* PFE */
#if CONFIG_IS_ENABLED(FSL_PFENG)
	nodeoff = fdt_node_offset_by_compatible(fdt, 0, "fsl,s32g274a-pfeng");
	if (nodeoff >= 0) {
		if (!pfeng_drv_status_active()) {
			/* Disable PFE in DT fully */
			printf("DT: Disabling PFE\n");
			fdt_status_disabled(fdt, nodeoff);
		} else {
			printf("DT: Enabling PFE\n");
			fdt_status_okay(fdt, nodeoff);

			/* Check for interfaces and manage accordingly */
			ft_enet_pfe_emac_fixup(0, fdt);
			ft_enet_pfe_emac_fixup(1, fdt);
			ft_enet_pfe_emac_fixup(2, fdt);
		}
	}
#endif /* CONFIG_IS_ENABLED(FSL_PFENG) */

	/* GMAC */
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
	nodeoff = fdt_node_offset_by_compatible(fdt, 0, "fsl,s32cc-dwmac");
	if (nodeoff >= 0) {

		if (s32ccgmac_cfg_get_mode() == S32CCGMAC_MODE_DISABLE) {
			ena = false;
#if CONFIG_IS_ENABLED(FSL_PFENG)
		} else if (intf_is_xmii(s32ccgmac_cfg_get_interface()) &&
			   intf_is_xmii(pfeng_cfg_emac_get_interface(1)) &&
			   pfeng_drv_status_active()) {
			ena = false;
#endif /* CONFIG_IS_ENABLED(FSL_PFENG) */
		} else
			ena = true;

		if (!ena) {
			printf("DT: Disabling GMAC\n");
			fdt_status_disabled(fdt, nodeoff);
		} else {
			printf("DT: Enabling GMAC\n");
			fdt_status_okay(fdt, nodeoff);

			/* sync MAC HW addr to DT [local-mac-address] */
			ft_update_eth_addr_by_name("eth", 0, fdt, nodeoff);
		}
	}
#endif /* CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC) */
}

/*
 * GMAC driver for common chassis
 *
 */
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)

#if !CONFIG_IS_ENABLED(OF_CONTROL)
/* driver platform data (in case of no DT) */
static struct eqos_pdata dwmac_pdata = {
	.eth = {
		/* registers base address */
		.iobase = (phys_addr_t)ETHERNET_0_BASE_ADDR,
		/* default phy mode */
		.phy_interface = PHY_INTERFACE_MODE_RGMII,
		/* max 1 Gbps */
		.max_speed = SPEED_1000,
	},
	/* vendor specific driver config */
	.config = &eqos_s32cc_config,
};
#endif /* OF_CONTROL */

/* GMAC platform specific setup */

void setup_iomux_enet_gmac(int intf)
{
	/* configure interface specific pins */
	switch (intf) {
	case PHY_INTERFACE_MODE_SGMII:
		/* GMAC_MDC */
		writel(SIUL2_MSCR_S32_G1_ENET_MDC,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD12));
		/* GMAC_MDIO */
		writel(SIUL2_MSCR_S32_G1_ENET_MDIO,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD13));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D3_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_MDI_IN));
		break;

	case PHY_INTERFACE_MODE_RGMII:
		/* set PE2 - MSCR[66] - for TX CLK */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_CLK,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_TX_CLK));
		/* set PE3 - MSCR[67] - for TX_EN */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_EN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE3));
		/* set PE4 - MSCR[68] - for TX_D0 */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_D0,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE4));
		/* set PE5 - MSCR[69] - for TX_D1 */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_D1,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE5));
		/* set PE6 - MSCR[70] - for TX_D2 */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_D2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE6));
		/* set PE7 - MSCR[71] - for TX_D3 */
		writel(SIUL2_MSCR_S32_G1_ENET_TX_D3,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE7));
		/* set PE8 - MSCR[72] - for RX_CLK */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_CLK,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE8));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_CLK_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RX_CLK_IN));
		/* set PD9 - MSCR[73] - for RX_DV */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_DV,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE9));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_DV_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RXDV_IN));
		/* set PE10 - MSCR[74] - for RX_D0 */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D0,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE10));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D0_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RXD0_IN));
		/* set PE11 - MSCR[75] - for RX_D1 */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D1,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE11));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D1_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RXD1_IN));
		/* set PE12 - MSCR[76] - for RX_D2 */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D2,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE12));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D2_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RXD2_IN));
		/* set PE13 - MSCR[77] - for RX_D3 */
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D3,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PE13));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D3_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_RXD3_IN));
		/* GMAC_MDC */
		writel(SIUL2_MSCR_S32_G1_ENET_MDC,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD12));
		/* GMAC_MDIO */
		writel(SIUL2_MSCR_S32_G1_ENET_MDIO,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_PD13));
		writel(SIUL2_MSCR_S32_G1_ENET_RX_D3_IN,
		       SIUL2_0_MSCRn(SIUL2_MSCR_S32_G1_GMAC0_MDI_IN));
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO: pinmuxing for RMII  */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO: pinmuxing for MII  */
		break;

	default:
		break;
	}
}

static ulong gmac_calc_link_speed(u32 speed)
{
	switch (speed) {
	case SPEED_10:   /* 2.5MHz */
		return 2500000UL;
	case SPEED_100:  /* 25MHz */
		return 25000000UL;
	default:
	case SPEED_1000: /* 125MHz (also 325MHz for 2.5G) */
		return 125000000UL;
	}
}

int set_tx_clk_enet_gmac(struct udevice *gmac_dev, u32 speed)
{
	ulong freq = gmac_calc_link_speed(speed);

	if (s32gen1_set_dev_clk_rate("tx", gmac_dev, freq) != freq)
		return -EINVAL;

	if (s32gen1_enable_dev_clk("tx", gmac_dev)) {
		dev_err(gmac_dev, "Failed to enable gmac_tx clock\n");
		return -EINVAL;
	}

	return 0;
}

void setup_clocks_enet_gmac(int intf, struct udevice *gmac_dev)
{
	int ret;
	u32 tx_id, rx_id;

	ret = s32gen1_set_parent_clk_id(S32GEN1_CLK_MC_CGM0_MUX9,
					S32GEN1_CLK_PERIPH_PLL_PHI4);
	if (ret) {
		dev_err(gmac_dev, "Failed to set cgm0_mux9 source\n");
		return;
	}

	rx_id = S32GEN1_CLK_FIRC;
	tx_id = rx_id;

	/* configure interface specific clocks */
	switch (intf) {
	case PHY_INTERFACE_MODE_SGMII:
		printf("==== SGMII =====\n");
		tx_id = S32GEN1_CLK_SERDES0_LANE0_TX;
		rx_id = S32GEN1_CLK_SERDES0_LANE0_CDR;
		break;

	case PHY_INTERFACE_MODE_RGMII:
		tx_id = S32GEN1_CLK_PERIPH_PLL_PHI5;
		rx_id = S32GEN1_CLK_GMAC0_EXT_RX;
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* TODO: clocks cfg for RMII */
		break;

	case PHY_INTERFACE_MODE_MII:
		/* TODO: clocks cfg for MII */
		break;

	default:
		break;
	}

	ret = s32gen1_set_parent_clk_id(S32GEN1_CLK_MC_CGM0_MUX10, tx_id);
	if (ret) {
		dev_err(gmac_dev, "Failed to set cgm0_mux10 source\n");
		return;
	}

	ret = s32gen1_set_parent_clk_id(S32GEN1_CLK_MC_CGM0_MUX11, rx_id);
	if (ret) {
		dev_err(gmac_dev, "Failed to set cgm0_mux11 source\n");
		return;
	}

	if (rx_id == S32GEN1_CLK_FIRC && tx_id == S32GEN1_CLK_FIRC)
		return;

	if (set_tx_clk_enet_gmac(gmac_dev, SPEED_1000)) {
		dev_err(gmac_dev, "Failed to set GMAC TX frequency\n");
		return;
	}

	ret = s32gen1_enable_dev_clk("rx", gmac_dev);
	if (ret)
		dev_err(gmac_dev, "Failed to enable gmac_rx clock\n");

	ret = s32gen1_enable_dev_clk("tx", gmac_dev);
	if (ret)
		dev_err(gmac_dev, "Failed to enable gmac_tx clock\n");

	ret = s32gen1_enable_dev_clk("ts", gmac_dev);
	if (ret)
		dev_err(gmac_dev, "Failed to enable gmac_tx clock\n");
}

#endif /* CONFIG_DWC_ETH_QOS_S32CC */

/*
 * PFEng driver for S32G only
 *
 */
#if CONFIG_IS_ENABLED(FSL_PFENG) && !CONFIG_IS_ENABLED(OF_CONTROL)

/* driver platform data (in case of no DT) */
static struct pfeng_pdata pfeng_platdata = {
	.eth = {
		/* registers base address */
		.iobase = (phys_addr_t)ETHERNET_0_BASE_ADDR,
		/* default phy mode */
		.phy_interface = PHY_INTERFACE_MODE_RGMII,
	},
	/* vendor specific driver config */
	.config = &pfeng_s32g274a_config,
};

#endif /* CONFIG_FSL_PFENG */

#if !CONFIG_IS_ENABLED(OF_CONTROL)
/*
 * Platform network devices
 *
 */
U_BOOT_DEVICES(s32_enet) = {
#if CONFIG_IS_ENABLED(DWC_ETH_QOS_S32CC)
	{
		.name = "eth_eqos",
		.platdata = &dwmac_pdata,
	},
#endif
#if CONFIG_IS_ENABLED(FSL_PFENG)
	{
		.name = "eth_pfeng",
		.platdata = &pfeng_platdata,
	},
#endif
};
#endif
