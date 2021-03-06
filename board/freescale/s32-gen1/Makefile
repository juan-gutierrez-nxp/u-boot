#
# (C) Copyright 2013-2015 Freescale Semiconductor, Inc.
# Copyright 2018-2020 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

ccflags-$(CONFIG_S32_GEN1) += -Iarch/$(ARCH)/cpu/armv8/s32
ccflags-$(CONFIG_S32_GEN1) += -Iboard/freescale/s32-gen1
ccflags-$(CONFIG_S32_GEN1) += -Idrivers/clk/s32/include
obj-y := common.o

obj-$(CONFIG_S32G274A) += s32g274a.o
obj-$(CONFIG_S32R45) += s32r45.o

ifeq (,$(CONFIG_OF_EMBED)$(CONFIG_OF_SEPARATE))
	obj-y	+= ../../../drivers/core/ofnode.o
endif

obj-$(CONFIG_NETDEVICES) += eth.o

obj-$(CONFIG_TARGET_S32G274AEVB) += s32g274aevb.o
obj-$(CONFIG_TARGET_S32G274ARDB) += s32g274ardb.o
obj-$(CONFIG_TARGET_S32G274ASIM) += s32g274asim.o
obj-$(CONFIG_TARGET_S32G274AEMU) += s32g274aemu.o

obj-$(CONFIG_TARGET_S32R45EVB) += s32r45evb.o
obj-$(CONFIG_TARGET_S32R45SIM) += s32r45sim.o
obj-$(CONFIG_TARGET_S32R45EMU) += s32r45emu.o

obj-$(CONFIG_TARGET_S32V344EVB) += s32v344sim.o
obj-$(CONFIG_TARGET_S32V344SIM) += s32v344sim.o

ifdef CONFIG_TARGET_TYPE_S32GEN1_EMULATOR
obj-$(CONFIG_SYS_FSL_DDRSS) += ddrss_regconf_emu.o ddrss_firmware_emu.o
else

ifneq ($(filter y,$(CONFIG_TARGET_S32G274ARDB) $(CONFIG_TARGET_S32G274AEVB)),)
obj-$(CONFIG_SYS_FSL_DDRSS) += s32g274a/phy_cfg.o s32g274a/pie_cfg.o s32g274a/ddrc_cfg.o ddr_utils.o ddrss_cfg.o dq_swap_cfg.o imem_cfg.o s32g274a/dmem_cfg.o ddr_init.o
endif

ifneq ($(filter y,$(CONFIG_TARGET_S32R45EVB)),)
obj-$(CONFIG_SYS_FSL_DDRSS) += s32r45/phy_cfg.o s32r45/pie_cfg.o s32r45/ddrc_cfg.o ddr_utils.o ddrss_cfg.o dq_swap_cfg.o imem_cfg.o s32r45/dmem_cfg.o ddr_init.o
endif

ifdef CONFIG_TARGET_S32G274AEVB
obj-$(CONFIG_SJA1105) += sja1105_cfg_evb.o
else ifdef CONFIG_TARGET_S32G274ARDB
obj-$(CONFIG_SJA1105) += sja1105_cfg_rdb.o
endif

endif

#########################################################################
