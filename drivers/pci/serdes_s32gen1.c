// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 * S32Gen1 PCIe driver
 */

#include <common.h>
#include <pci.h>
#include <asm/io.h>
#include <errno.h>
#include <malloc.h>
#include <dm.h>
#include <asm/arch/clock.h>
#include <linux/sizes.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <asm/arch-s32/siul.h>
#include <hwconfig.h>
#include <clk.h>

#include "serdes_s32gen1.h"
#include "mc_rgm_regs.h"

#define PCIE_DEFAULT_INTERNAL_CLK	CLK_INT
#define PCIE_DEFAULT_INTERNAL_CLK_FMHZ	CLK_100MHZ

#define SERDES_RC_MODE_STR "RootComplex"
#define SERDES_EP_MODE_STR "EndPoint"
#define SERDES_SGMII_MODE_STR "SGMII"
#define SERDES_SGMII_MODE_NONE_STR "None"
#define SERDES_MODE_SIZE 64

#define SERDES_CLK_MODE(clk_type) \
			((clk_type == CLK_INT) ? "internal" : "external")
#define SERDES_CLK_FMHZ(clk_type) \
			((clk_type == CLK_100MHZ) ? "100Mhz" : "125Mhz")

DECLARE_GLOBAL_DATA_PTR;

LIST_HEAD(s32_serdes_list);

char xpcs_str[][64] = {	"[INVALID XPCS CFG]",
			"[XPCS0 1G, XPCS1 OFF(PCIex1)]",
			"[XPCS0 OFF(PCIex1), XPCS1 1G]",
			"[XPCS0 1G, XPCS1 1G]",
			"[XPCS0 2.5G, XPCS1 OFF]",
};

static inline const char *get_serdes_xpcs_str(enum serdes_xpcs_mode mode)
{
	if (mode > SGMII_INAVALID && mode <= SGMII_XPCS_LAST)
		return xpcs_str[mode];

	return xpcs_str[SGMII_INAVALID];
}

static inline int get_serdes_mode_str(enum serdes_dev_type mode,
		enum serdes_xpcs_mode xpcs_mode, char *buf)
{
	char *start = buf;

	if (mode & PCIE_RC)
		start += sprintf(start, SERDES_RC_MODE_STR);
	if (mode & PCIE_EP)
		start += sprintf(start, SERDES_EP_MODE_STR);
	if (mode & SGMII) {
		if (xpcs_mode == SGMII_XPCS0 || xpcs_mode == SGMII_XPCS1) {
			if (start != buf)
				start += sprintf(start, "(x1)&");
			start += sprintf(start, SERDES_SGMII_MODE_STR);
		} else if (xpcs_mode != SGMII_INAVALID)
			start += sprintf(start, SERDES_SGMII_MODE_STR "(x2)");

		start += sprintf(start, " %s",  get_serdes_xpcs_str(xpcs_mode));
	} else if (start != buf) {
		start += sprintf(start, "(x2)");
	} else {
		start += sprintf(start, "Not configured");
	}

	return start - buf;
}

static int wait_read32(void *address, uint32_t expect_data,
		uint32_t mask, int read_attempts)
{
	uint32_t tmp;

	while ((tmp = (in_le32(address) & (mask))) != (expect_data)) {
		udelay(DELAY_QUANTUM); read_attempts--;
		if (read_attempts < 0) {
			debug_wr("WARNING: timeout read 0x%x from 0x%llx,",
				tmp, (uint64_t)(address));
			debug_wr(" expected 0x%x\n", expect_data);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

bool s32_pcie_wait_link_up(void __iomem *dbi)
{
	int count = PCIE_LINK_UP_COUNT;

	return (wait_read32((void *)(dbi + SS_PE0_LINK_DBG_2),
			SERDES_LINKUP_EXPECT, SERDES_LINKUP_MASK, count) == 0);
}

enum serdes_mode s32_get_serdes_mode_from_target(void *dbi, int id)
{
	return SUBSYS_MODE_VALUE(in_le32(dbi + SS_SS_RW_REG_0));
}

int s32_serdes_set_mode(void *dbi, int id, enum serdes_mode mode)
{
	enum serdes_dev_type devtype = s32_serdes_get_mode_from_hwconfig(id);

	/* validate that required 'mode' does not interfere
	 * with 'hwconfig'
	 */
	switch (mode) {
	case SERDES_MODE_PCIE_PCIE:
		/* only PCIE, no SGMII for this mode */
		if (!IS_SERDES_PCIE(devtype) || IS_SERDES_SGMII(devtype))
			return -EINVAL;
		break;
	/* Will have to figure out how to handle SERDES_MODE_SGMII_PCIE
	 * and SERDES_MODE_PCIE_SGMII, since lane assignment may differ.
	 */
	case SERDES_MODE_PCIE_SGMII0:
	case SERDES_MODE_PCIE_SGMII1:
		if (!IS_SERDES_PCIE(devtype) || !IS_SERDES_SGMII(devtype))
			return -EINVAL;
		break;
	case SERDES_MODE_SGMII_SGMII:
		if (IS_SERDES_PCIE(devtype) || !IS_SERDES_SGMII(devtype))
			return -EINVAL;
		break;
	default: return -EINVAL;
	}

	BSET32(dbi + SS_SS_RW_REG_0, SUBSYS_MODE_VALUE(mode));

	/* small delay for stabilizing the signals */
	udelay(100);

	return 0;
}

static void s32_serdes_disable_ltssm(void __iomem *dbi)
{
	BCLR32(dbi + SS_PE0_GEN_CTRL_3, LTSSM_EN);
}

static void s32_serdes_enable_ltssm(void __iomem *dbi)
{
	BSET32(dbi + SS_PE0_GEN_CTRL_3, LTSSM_EN);
}

/* SERDES Peripheral reset.
 * See Reference Manual for peripheral indices used below.
 */

static int rgm_get_regs(u32 id, phys_addr_t *prst, phys_addr_t *pstat)
{
	if (id <= 17U) {
		*prst = RGM_PRST(MC_RGM_BASE_ADDR, 0);
		*pstat = RGM_PSTAT(MC_RGM_BASE_ADDR, 0);
	} else if ((id >= 64U) && (id <= 68U)) {
		*prst = RGM_PRST(MC_RGM_BASE_ADDR, 1);
		*pstat = RGM_PSTAT(MC_RGM_BASE_ADDR, 1);
	} else if ((id >= 128U) && (id <= 130)) {
		*prst = RGM_PRST(MC_RGM_BASE_ADDR, 2);
		*pstat = RGM_PSTAT(MC_RGM_BASE_ADDR, 2);
	} else if ((id >= 192U) && (id <= 194U)) {
		*prst = RGM_PRST(MC_RGM_BASE_ADDR, 3);
		*pstat = RGM_PSTAT(MC_RGM_BASE_ADDR, 3);
	} else {
		printf("error: Reset of unknown peripheral");
		printf(" or domain requested (%d)\n", id);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief		Issue peripheral/domain reset
 * @param[in]	pid Peripheral/domain index. See RM.
 */
int rgm_issue_reset(u32 pid)
{
	phys_addr_t prst, pstat;
	u32 regval = RGM_PERIPH_RST(pid % 32);
	int retval;

	retval = rgm_get_regs(pid, &prst, &pstat);
	if (retval)
		return retval;

	BSET32(prst, regval);

	return wait_read32((void *)pstat, regval, regval, PCIE_RESET_COUNT);
}

/**
 * @brief		Release peripheral/domain reset
 * @param[in]	pid Peripheral/domain index. See RM.
 */
int rgm_release_reset(u32 pid)
{
	phys_addr_t prst, pstat;
	u32 regval = RGM_PERIPH_RST(pid % 32);
	int retval;

	retval = rgm_get_regs(pid, &prst, &pstat);
	if (retval)
		return retval;

	BCLR32(prst, regval);

	return wait_read32((void *)pstat, 0, regval, PCIE_RESET_COUNT);
}

int s32_deassert_serdes_reset(struct s32_serdes *pcie)
{
	printf("%s: SerDes%d\n", __func__, pcie->id);

	/* Deassert SerDes reset */
	if (pcie->id == 0)
		if (rgm_release_reset(PRST_PCIE_0_SERDES) ||
				rgm_release_reset(PRST_PCIE_0_FUNC)) {
			printf("PCIe%d reset failed\n", pcie->id);
			return -ENODEV;
		}
	if (pcie->id == 1)
		if (rgm_release_reset(PRST_PCIE_1_SERDES) ||
				rgm_release_reset(PRST_PCIE_1_FUNC)) {
			printf("PCIe%d reset failed\n", pcie->id);
			return -ENODEV;
		}

	return 0;
}

bool s32_assert_serdes_reset(struct s32_serdes *pcie)
{
	printf("%s: SerDes%d\n", __func__, pcie->id);

	/* Assert SerDes reset */
	if (pcie->id == 0)
		if (rgm_issue_reset(PRST_PCIE_0_SERDES) ||
		    rgm_issue_reset(PRST_PCIE_0_FUNC)) {
			printf("PCIe%d reset failed\n", pcie->id);
			return -ENODEV;
		}
	if (pcie->id == 1)
		if (rgm_issue_reset(PRST_PCIE_1_FUNC) ||
		    rgm_issue_reset(PRST_PCIE_1_SERDES)) {
			printf("PCIe%d reset failed\n", pcie->id);
			return -ENODEV;
		}

	return 0;
}

void s32_serdes_phy_reg_write(struct s32_serdes *pcie, uint32_t addr,
		uint32_t wdata, uint32_t wmask)
{
	uint32_t temp_data;

	W32(pcie->dbi + SS_PHY_REG_ADDR, (PHY_REG_ADDR_FIELD_VALUE(addr) |
			PHY_REG_EN));
	udelay(100);
	if (wmask != 0xFFFF)
		temp_data = in_le32(pcie->dbi + SS_PHY_REG_DATA);
	else
		temp_data = 0;
	temp_data &= 0x0000FFFF;
	temp_data = (temp_data & (!wmask)) | (wdata & wmask);
	W32(pcie->dbi + SS_PHY_REG_DATA, temp_data);
	udelay(100);
}

void s32_serdes_phy_init(struct s32_serdes *pcie)
{
	/* DELTA_IQ_OVRD_IN enable and overrides PCIe0.L0 */
	s32_serdes_phy_reg_write(pcie, 0x3019, 0x03, 0xff);
	s32_serdes_phy_reg_write(pcie, 0x3019, 0x13, 0xff);

	/* DELTA_IQ_OVRD_IN enable and overrides PCIe0.L1 */
	s32_serdes_phy_reg_write(pcie, 0x3119, 0x03, 0xff);
	s32_serdes_phy_reg_write(pcie, 0x3119, 0x13, 0xff);
}

bool s32_serdes_init(struct s32_serdes *pcie)
{
	/* Reset the Serdes module */
	s32_assert_serdes_reset(pcie);

	if (IS_SERDES_PCIE(pcie->devtype) && !IS_SERDES_SGMII(pcie->devtype)) {
		if (s32_serdes_set_mode(pcie->dbi, pcie->id,
					SERDES_MODE_PCIE_PCIE))
			return false;
	} else if (IS_SERDES_PCIE(pcie->devtype) &&
				IS_SERDES_SGMII(pcie->devtype)) {
		if (pcie->xpcs_mode != SGMII_XPCS0 &&
		    pcie->xpcs_mode != SGMII_XPCS1) {
			printf("ERROR: Invalid XPCS config on PCIe%d\n",
			       pcie->id);

			/* Fall back to mode compatible with PCIe */
			if (s32_serdes_set_mode(pcie->dbi, pcie->id,
						SERDES_MODE_PCIE_SGMII0))
				return false;
		}

		/* Configure SS mode based on XPCS */
		if (pcie->xpcs_mode == SGMII_XPCS0)
			if (s32_serdes_set_mode(pcie->dbi, pcie->id,
						SERDES_MODE_PCIE_SGMII0))
				return false;
		if (pcie->xpcs_mode == SGMII_XPCS1)
			if (s32_serdes_set_mode(pcie->dbi, pcie->id,
						SERDES_MODE_PCIE_SGMII1))
				return false;
	} else if (!IS_SERDES_PCIE(pcie->devtype) &&
				IS_SERDES_SGMII(pcie->devtype)) {
		/*	Set pipeP_pclk */
		W32(pcie->dbi + SS_PHY_GEN_CTRL, EXT_PCLK_REQ);
		if (s32_serdes_set_mode(pcie->dbi, pcie->id,
					 SERDES_MODE_SGMII_SGMII))
			return false;
	}

	s32_deassert_serdes_reset(pcie);

	/* Set the clock for the Serdes module */
	if (pcie->clktype == CLK_INT) {
		printf("Set internal clock\n");
		BCLR32(pcie->dbi + SS_PHY_GEN_CTRL,
		       PHY_GEN_CTRL_REF_USE_PAD);
		BSET32(pcie->dbi + SS_SS_RW_REG_0, 1 << 23);
	} else {
		printf("Set external clock\n");
		BSET32(pcie->dbi + SS_PHY_GEN_CTRL,
		       PHY_GEN_CTRL_REF_USE_PAD);
		BCLR32(pcie->dbi + SS_SS_RW_REG_0, 1 << 23);
	}

	if (IS_SERDES_PCIE(pcie->devtype)) {

		/* Monitor Serdes MPLL state */
		if (wait_read32((void *)(pcie->dbi + SS_PHY_MPLLA_CTRL),
			MPLL_STATE, MPLL_STATE, PCIE_MPLL_LOCK_COUNT)) {
			printf("WARNING: Failed to lock PCIe%d MPLLs\n",
				pcie->id);
			return false;
		}

		/* Set PHY register access to CR interface */
		BSET32(pcie->dbi + SS_SS_RW_REG_0, 0x200);
		s32_serdes_phy_init(pcie);
	}

	return true;
}

__weak bool s32_pcie_init(void __iomem *dbi, int id, bool rc_mode,
		enum serdes_link_width linkwidth)
{
	printf("PCIe%d disabled\n", id);
	return false;
}

__weak bool s32_pcie_set_link_width(void __iomem *dbi,
		int id, enum serdes_link_width linkwidth)
{
	printf("PCIe%d disabled\n", id);
	return false;
}

__weak int s32_eth_xpcs_init(void __iomem *dbi, int id,
			     bool combo,
			     enum serdes_xpcs_mode xpcs_mode,
			     enum serdes_clock clktype,
			     enum serdes_clock_fmhz fmhz)
{
	/* Configure SereDes XPCS for PFE/GMAC*/
	printf("PCIe%d disabled\n", id);
	return -ENODEV;
}

enum serdes_dev_type s32_serdes_get_mode_from_hwconfig(int id)
{
	char pcie_name[10];
	sprintf(pcie_name, "pcie%d", id);
	enum serdes_dev_type devtype = SERDES_INVALID;

	printf("%s: testing hwconfig for '%s'\n", __func__,
		pcie_name);

	if (hwconfig_subarg_cmp(pcie_name, "mode", "rc"))
		devtype = PCIE_RC;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "ep"))
		devtype = PCIE_EP;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "sgmii"))
		devtype = SGMII;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "rc&sgmii"))
		devtype = PCIE_RC | SGMII;
	if (hwconfig_subarg_cmp(pcie_name, "mode", "ep&sgmii"))
		devtype = PCIE_EP | SGMII;

	return devtype;
}

enum serdes_xpcs_mode s32_serdes_get_xpcs_cfg_from_hwconfig(int id)
{
	char pcie_name[10];

	sprintf(pcie_name, "pcie%d", id);
	/* Set default mode to invalid to force configuration */
	enum serdes_xpcs_mode xpcs_mode = SGMII_INAVALID;

	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "0"))
		xpcs_mode = SGMII_XPCS0;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "1"))
		xpcs_mode = SGMII_XPCS1;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "both"))
		xpcs_mode = SGMII_XPCS0_XPCS1;
	if (hwconfig_subarg_cmp(pcie_name, "xpcs_mode", "2G5"))
		xpcs_mode = SGMII_XPCS0_2G5;

	return xpcs_mode;
}

enum serdes_clock s32_serdes_get_clock_from_hwconfig(int id)
{
	char pcie_name[10];

	sprintf(pcie_name, "pcie%d", id);
	enum serdes_clock clk = PCIE_DEFAULT_INTERNAL_CLK;

	if (hwconfig_subarg_cmp(pcie_name, "clock", "ext"))
		clk = CLK_EXT;
	if (hwconfig_subarg_cmp(pcie_name, "clock", "int"))
		clk = CLK_INT;

	return clk;
}

enum serdes_clock_fmhz s32_serdes_get_clock_fmhz_from_hwconfig(int id)
{
	char pcie_name[10];

	sprintf(pcie_name, "pcie%d", id);
	enum serdes_clock_fmhz clk = PCIE_DEFAULT_INTERNAL_CLK_FMHZ;

	if (hwconfig_subarg_cmp(pcie_name, "fmhz", "100"))
		clk = CLK_100MHZ;
	if (hwconfig_subarg_cmp(pcie_name, "fmhz", "125"))
		clk = CLK_125MHZ;

	return clk;
}

static bool s32_serdes_is_xpcs_cfg_valid(struct s32_serdes *pcie)
{
	bool ret = true;

	if (IS_SERDES_PCIE(pcie->devtype) && pcie->fmhz == CLK_125MHZ) {
		printf("Invalid \"hwconfig\": In PCIe/SGMII combo");
		printf(" reference clock has to be 100Mhz\n");
		/* SGMII configuration fail */
		ret = false;
	}

	if (pcie->xpcs_mode == SGMII_INAVALID) {
		printf("Invalid \"hwconfig\": \"xpcs_mode\" is missing\n");
		/* SGMII configuration fail */
		ret = false;
	}

	if (pcie->xpcs_mode == SGMII_XPCS0_2G5 &&
	    pcie->fmhz == CLK_100MHZ) {
		printf("Invalid \"hwconfig\": In SGMII 2.5G frequency");
		printf("has to be 125MHz\n");
		/* We just fail in this case user has to reconfigure */
		ret = false;
	}

	return ret;
}

static int s32_serdes_get_config_from_device_tree(struct s32_serdes *pcie)
{
	const void *fdt = gd->fdt_blob;
	struct udevice *dev = pcie->bus;
	int node = dev_of_offset(dev);
	int ret = 0;

	printf("dt node: %d\n", node);

	ret = fdt_get_named_resource(fdt, node, "reg", "reg-names",
				     "dbi", &pcie->dbi_res);
	if (ret) {
		printf("s32-serdes: resource \"dbi\" not found\n");
		return ret;
	}

	pcie->dbi = map_physmem(pcie->dbi_res.start,
				fdt_resource_size(&pcie->dbi_res),
				MAP_NOCACHE);

	printf("%s: dbi: 0x%p (0x%p)\n", __func__, (void *)pcie->dbi_res.start,
			pcie->dbi);

	pcie->id = fdtdec_get_int(fdt, node, "device_id", -1);

	pcie->linkwidth = fdtdec_get_int(fdt, node, "num-lanes", 0);

	return ret;
}

static int enable_serdes_clocks(struct udevice *dev)
{
	struct clk_bulk bulk;
	int ret;

	ret = clk_get_bulk(dev, &bulk);
	if (ret == -ENOSYS)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&bulk);
	if (ret) {
		clk_release_bulk(&bulk);
		return ret;
	}
	return 0;
}

static int s32_serdes_probe(struct udevice *dev)
{
	struct s32_serdes *pcie = dev_get_priv(dev);
	char mode[SERDES_MODE_SIZE];
	int ret = 0;
	bool combo_mode;

	printf("%s: probing %s\n", __func__, dev->name);
	if (!pcie) {
		printf("s32-serdes: invalid internal data\n");
		return -EINVAL;
	}

	pcie->bus = dev;

	list_add(&pcie->list, &s32_serdes_list);

	ret = s32_serdes_get_config_from_device_tree(pcie);
	if (ret)
		return ret;

	ret = enable_serdes_clocks(dev);
	if (ret) {
		dev_err(dev, "Failed to enable SERDES clocks\n");
		return ret;
	}

	pcie->devtype = s32_serdes_get_mode_from_hwconfig(pcie->id);
	if (pcie->devtype == SERDES_INVALID) {
		printf("Not configuring SerDes%d,", pcie->id);
		printf(" no RC/EP/SGMII configuration selected\n");
		return ret;
	}

	pcie->clktype = s32_serdes_get_clock_from_hwconfig(pcie->id);
	/* Get XPCS configuration */
	pcie->xpcs_mode = s32_serdes_get_xpcs_cfg_from_hwconfig(pcie->id);
	pcie->fmhz = s32_serdes_get_clock_fmhz_from_hwconfig(pcie->id);

	/* In case of sgmii mode check xpcs configuration */
	if (IS_SERDES_SGMII(pcie->devtype) &&
	    !s32_serdes_is_xpcs_cfg_valid(pcie))
		pcie->xpcs_mode = SGMII_INAVALID;

	printf("Using %s clock for PCIe%d\n",
	       SERDES_CLK_MODE(pcie->clktype),
	       pcie->id);
	if (IS_SERDES_SGMII(pcie->devtype) &&
	    pcie->xpcs_mode != SGMII_INAVALID)
		printf("Frequency %s configured for PCIe%d\n",
		       SERDES_CLK_FMHZ(pcie->fmhz),
		       pcie->id);

	get_serdes_mode_str(pcie->devtype, pcie->xpcs_mode, mode);
	printf("Configuring PCIe%d as %s\n", pcie->id, mode);


	/* Keep ltssm_enable =0 to disable link  training for programming
	 * the DBI.
	 * Note: ltssm_enable is set to 1 from the PCIe driver.
	 */
	s32_serdes_disable_ltssm(pcie->dbi);

        printf("After ltssm\n");

	/* Apply the base SerDes/PHY settings */
	if (!s32_serdes_init(pcie))
		return ret;

         printf("After serdes_init devtype = %d xpcs_mode %d dbi = 0x%x \n", pcie->devtype ,pcie->xpcs_mode, pcie->dbi);

	if (IS_SERDES_SGMII(pcie->devtype) &&
	    pcie->xpcs_mode != SGMII_INAVALID) {
		combo_mode = (IS_SERDES_SGMII(pcie->devtype) &&
			      IS_SERDES_PCIE(pcie->devtype));
		ret = s32_eth_xpcs_init(pcie->dbi, pcie->id,
					combo_mode,
					pcie->xpcs_mode,
					pcie->clktype,
					pcie->fmhz);
		if (ret) {
			printf("Error during configuration of SGMII on");
			printf(" PCIe%d\n", pcie->id);
		}
	}

	if (IS_SERDES_PCIE(pcie->devtype)) {
		char mode[SERDES_MODE_SIZE];

		/* Update the max link depending on other factors */

		get_serdes_mode_str(pcie->devtype, pcie->xpcs_mode, mode);
		printf("SerDes%d: Configure as %s\n", pcie->id, mode);
		if (IS_SERDES_SGMII(pcie->devtype))
			pcie->linkwidth = X1;

		/* Restrict EP to X1, since we don't always habe link
		 * in case of EP (RC may start after the EP)
		 */
		if (!(pcie->devtype & PCIE_RC))
			pcie->linkwidth = X1;

		if (!s32_pcie_init(pcie->dbi, pcie->id,
					pcie->devtype & PCIE_RC,
					pcie->linkwidth) ||
			!s32_pcie_set_link_width(pcie->dbi, pcie->id,
					pcie->linkwidth))
			return ret;

		s32_serdes_enable_ltssm(pcie->dbi);

		if (s32_pcie_wait_link_up(pcie->dbi)) {
			printf("SerDes%d: link is up (X%d)\n", pcie->id,
					pcie->linkwidth);
		} else {
			if (pcie->linkwidth > X1) {
				/* Attempt to link at X1 */
				pcie->linkwidth = X1;
				s32_serdes_disable_ltssm(pcie->dbi);

				if (!s32_pcie_set_link_width(pcie->dbi,
						pcie->id,
						pcie->linkwidth))
					return ret;

				s32_serdes_enable_ltssm(pcie->dbi);
				if (s32_pcie_wait_link_up(pcie->dbi))
					printf("SerDes%d: link is up (X%d)\n",
						pcie->id, pcie->linkwidth);
			}
		}
	}

	return ret;
}

__weak void show_pcie_devices(void)
{
}

/* pci_init - called before the probe function */
int initr_pci(void)
{
	struct udevice *bus;

	printf("%s\n", __func__);

	/*
	 * Enumerate all known UCLASS_PCI_GENERIC devices. This will
	 * also probe them, so the SerDes devices will be enumerated too.
	 * TODO: Enumerate first the EPs, so that loopback between
	 * the two PCIe interfaces will also work if PCIe1 is EP.
	 */
	for (uclass_first_device(UCLASS_PCI_GENERIC, &bus);
	     bus;
	     uclass_next_device(&bus)) {
		;
	}

	/*
	 * Enumerate all known PCIe controller devices. Enumeration has
	 * the side-effect of probing them, so PCIe devices will be
	 * enumerated too.
	 * This is inspired from commands `pci` and `dm tree`.
	 */
	pci_init();

	/* now show the devices */
	show_pcie_devices();

	return 0;
}

static const struct udevice_id s32_serdes_ids[] = {
	{ .compatible = "fsl,s32gen1-serdes" },
	{ }
};

U_BOOT_DRIVER(serdes_s32gen1) = {
	.name = "serdes_s32gen1",
	.id = UCLASS_PCI_GENERIC,
	.of_match = s32_serdes_ids,
	.probe	= s32_serdes_probe,
	.priv_auto_alloc_size = sizeof(struct s32_serdes),
};
