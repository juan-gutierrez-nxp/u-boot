#
# Copyright 2020 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#
obj-y += clk_dump.o
obj-y += s32gen1_clk_names.o
obj-$(CONFIG_S32G274A) += s32g274a_clk_names.o
obj-$(CONFIG_S32R45) += s32r45_clk_names.o

ccflags-y += -Idrivers/clk/s32/include
ccflags-y += -Idrivers/clk/s32/clk_dump/include
