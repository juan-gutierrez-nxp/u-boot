/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017,2019-2020 NXP
 *
 */
#ifndef _FSL_S32_CPU_H
#define _FSL_S32_CPU_H

#include <asm/armv8/mmu.h>
#include <config.h>

#ifndef CONFIG_SYS_DCACHE_OFF
#define TCR_EL2_PS_40BIT	(2 << 16)

#define CONFIG_SYS_FSL_PERIPH_BASE      0x40000000

#if defined(CONFIG_S32V234)
#define CONFIG_SYS_FSL_PERIPH_SIZE      0x40000000
#elif defined(CONFIG_S32_GEN1)
#define CONFIG_SYS_FSL_PERIPH_SIZE      0x20000000
#endif

#ifdef CONFIG_PCIE_S32GEN1
/* TODO: These should go to defconfig, or even better,
 * be read from device tree
 */
#define CONFIG_SYS_PCIE0_PHYS_ADDR_LO		0x53000000ULL
#define CONFIG_SYS_PCIE1_PHYS_ADDR_LO		0x45000000ULL
#define CONFIG_SYS_PCIE0_PHYS_SIZE_LO		0x1000000
#define CONFIG_SYS_PCIE1_PHYS_SIZE_LO		0x1000000
#define CONFIG_SYS_PCIE0_PHYS_ADDR_HI		0x5800000000ULL
#define CONFIG_SYS_PCIE1_PHYS_ADDR_HI		0x4800000000ULL
#define CONFIG_SYS_PCIE0_PHYS_SIZE_HI		0x200000000
#define CONFIG_SYS_PCIE1_PHYS_SIZE_HI		0x200000000
#endif

#endif

u32 cpu_mask(void);
int cpu_numcores(void);

#endif /* _FSL_S32_CPU_H */
