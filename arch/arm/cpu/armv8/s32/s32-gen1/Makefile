# SPDX-License-Identifier:	GPL-2.0+
#
# Copyright 2018-2020 NXP
#

obj-$(CONFIG_TARGET_S32G274ARDB)	+= cmu.o
obj-$(CONFIG_TARGET_S32G274AEVB)	+= cmu.o
obj-$(CONFIG_TARGET_S32R45EVB)		+= cmu.o
obj-y					+= ncore.o
obj-y					+= soc.o
ifneq ($(filter y,$(CONFIG_TARGET_S32G274ARDB) $(CONFIG_TARGET_S32G274AEVB) $(CONFIG_TARGET_S32R45EVB)),)
obj-$(CONFIG_NETDEVICES)		+= sgmii/sgmii.o sgmii/serdes_xpcs.o
endif
obj-$(CONFIG_S32_GEN_1_USES_FLASH)	+= qspi.o
ccflags-y				+= -I./board/freescale/s32-gen1/
ccflags-y				+= -I./arch/arm/include/asm/arch-s32/s32-gen1/
ccflags-y				+= -I./drivers/clk/s32/include/
