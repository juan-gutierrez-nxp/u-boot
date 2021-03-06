/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef SIUL_S32R45_H
#define SIUL_S32R45_H

#include "siul-s32-gen1.h"

#ifdef SIUL2_1_MSCR_BASE
#undef SIUL2_1_MSCR_BASE
#define SIUL2_1_MSCR_BASE		(SIUL2_1_BASE_ADDR + 0x000003D8)
#endif

#ifdef SIUL2_1_IMCR_BASE
#undef SIUL2_1_IMCR_BASE
#define SIUL2_1_IMCR_BASE		 (SIUL2_1_BASE_ADDR + 0x00000BAC)
#endif

/* MSCR 102-785 */
#ifdef SIUL2_1_MSCRn
#undef SIUL2_1_MSCRn
#define SIUL2_1_MSCRn(i)		(SIUL2_1_MSCR_BASE + 4 * (i - 102))
#endif

#ifdef SIUL2_1_IMCRn
#undef SIUL2_1_IMCRn
#define SIUL2_1_IMCRn(i)		(SIUL2_1_IMCR_BASE +  4 * (i - 91))
#endif

/* UART0 settings */
/* TXD */
#define SIUL2_PC09_MSCR_S32R45_UART0	41
/* RXD */
#define SIUL2_PC10_MSCR_S32R45_UART0	42
#define SIUL2_PC10_IMCR_S32R45_UART0	(512 - 512)

/* UART1 settings */
/* TXD */
#define SIUL2_PA12_MSCR_S32R45_UART1	12
/* RXD */
#define SIUL2_PA11_MSCR_S32R45_UART1	11
#define SIUL2_PA11_IMCR_S32R45_UART1	(674 - 512)

/* DSPI settings*/
/* DSPI 1 */
#define SIUL2_PB04_MSCR_S32R45_DSPI1_SCK	(20)
#define SIUL2_PB04_IMCR_S32R45_DSPI1_SCK	(622 - 512)

#define SIUL2_PB05_MSCR_S32R45_DSPI1_SIN	(21)
#define SIUL2_PB05_IMCR_S32R45_DSPI1_SIN	(623 - 512)

#define SIUL2_PB06_MSCR_S32R45_DSPI1_SOUT	(22)

#define SIUL2_PB07_MSCR_S32R45_DSPI1_PCS0	(23)

#define SIUL2_PB08_MSCR_S32R45_DSPI1_PCS1	(24)
#define SIUL2_PC03_MSCR_S32R45_DSPI1_PCS2	(35)
#define SIUL2_PB10_MSCR_S32R45_DSPI1_PCS3	(26)

/* DSPI2 */
#define SIUL2_PB11_MSCR_S32R45_DSPI2_SCK	(27)
#define SIUL2_PB11_IMCR_S32R45_DSPI2_SCK	(632 - 512)

#define SIUL2_PC01_MSCR_S32R45_DSPI2_SIN	(33)
#define SIUL2_PC01_IMCR_S32R45_DSPI2_SIN	(681 - 512)

#define SIUL2_PB13_MSCR_S32R45_DSPI2_SOUT	(29)

#define SIUL2_PB14_MSCR_S32R45_DSPI2_PCS0	(30)
#define SIUL2_PB15_MSCR_S32R45_DSPI2_PCS1	(31)
#define SIUL2_PB09_MSCR_S32R45_DSPI2_PCS2	(25)
#define SIUL2_PC02_MSCR_S32R45_DSPI2_PCS3	(34)

/* DSPI3 */
#define SIUL2_PC04_MSCR_S32R45_DSPI3_SCK	(36)
#define SIUL2_PC04_IMCR_S32R45_DSPI3_SCK	(643 - 512)

#define SIUL2_PC06_MSCR_S32R45_DSPI3_SIN	(38)
#define SIUL2_PC06_IMCR_S32R45_DSPI3_SIN	(654 - 512)

#define SIUL2_PC13_MSCR_S32R45_DSPI3_SOUT	(45)

#define SIUL2_PC07_MSCR_S32R45_DSPI3_PCS0	(39)

/* DSPI5 */
#define SIUL2_PK00_MSCR_S32R45_DSPI5_SCK	(128)
#define SIUL2_PK00_IMCR_S32R45_DSPI5_SCK	(749 - 512)

#define SIUL2_PK04_MSCR_S32R45_DSPI5_SIN	(132)
#define SIUL2_PK04_IMCR_S32R45_DSPI5_SIN	(750 - 512)

#define SIUL2_PK05_MSCR_S32R45_DSPI5_SOUT	(133)

#define SIUL2_PK03_MSCR_S32R45_DSPI5_PCS0	(131)

/* UART muxing */
#define SIUL2_MSCR_S32R45_PORT_CTRL_UART_TXD	 \
	 SIUL2_MSCR_S32_G1_OBE_EN

#define SIUL2_MSCR_S32R45_PORT_CTRL_UART0_TXD	 \
	(SIUL2_MSCR_S32R45_PORT_CTRL_UART_TXD |	 \
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PORT_CTRL_UART1_TXD	 \
	(SIUL2_MSCR_S32R45_PORT_CTRL_UART_TXD |	 \
	 SIUL2_MSCR_MUX_MODE_ALT3)

#define SIUL2_MSCR_S32R45_PORT_CTRL_UART_RXD	 \
	 SIUL2_MSCR_S32_G1_IBE_EN

#define SIUL2_IMCR_S32R45_UART_RXD_to_pad	\
	SIUL2_MSCR_MUX_MODE_ALT2

/* DSPI */
#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SIN	\
	(SIUL2_MSCR_S32_G1_IBE_EN |		\
	 SIUL2_MSCR_S32_G1_PUE_EN |		\
	 SIUL2_MSCR_S32_G1_PUS_EN)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SCK_O	\
	(SIUL2_MSCR_S32_G1_OBE_EN)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SOUT	\
	(SIUL2_MSCR_S32_G1_OBE_EN)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx	\
	(SIUL2_MSCR_S32_G1_OBE_EN |		\
	 SIUL2_MSCR_S32_G1_PUE_EN |		\
	 SIUL2_MSCR_S32_G1_PUS_EN)

/* DSPI 1*/
#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_SCK	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SCK_O |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)
#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI1_SCK	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI1_SIN	\
	(SIUL2_MSCR_MUX_MODE_ALT4)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_SOUT	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SOUT |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_PCS0	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_PCS1	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_PCS2	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI1_PCS3	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

/* DSPI2 */
#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_SCK	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SCK_O |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)
#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI2_SCK	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI2_SIN	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_SOUT	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SOUT |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_PCS0	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_PCS1	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_PCS2	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI2_PCS3	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT2)

/* DSPI3 */
#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI3_SCK	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SCK_O |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)
#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI3_SCK	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI3_SIN	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI3_SOUT	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SOUT |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI3_PCS0	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT2)

/* DSPI5 */
#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI5_SCK	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SCK_O |	\
	 SIUL2_MSCR_MUX_MODE_ALT1)
#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI5_SCK	\
	(SIUL2_MSCR_MUX_MODE_ALT2)

#define SIUL2_IMCR_S32R45_PAD_CTL_DSPI5_SIN	\
	(SIUL2_MSCR_MUX_MODE_ALT4)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI5_SOUT	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_SOUT |	\
	 SIUL2_MSCR_MUX_MODE_ALT4)

#define SIUL2_MSCR_S32R45_PAD_CTL_DSPI5_PCS0	\
	(SIUL2_MSCR_S32R45_PAD_CTL_DSPI_PCSx |	\
	 SIUL2_MSCR_MUX_MODE_ALT3)

#define SIUL2_MSCR_S32R45_PB_00	16
#define SIUL2_MSCR_S32R45_PB_01	17
#define SIUL2_MSCR_S32R45_PA_15	15
#define SIUL2_MSCR_S32R45_PA_14	14
#define SIUL2_PB_00_IMCR_S32R45_I2C0_SDA	(565 - 512)
#define SIUL2_PB_01_IMCR_S32R45_I2C0_SCLK	(566 - 512)
#define SIUL2_PA_15_IMCR_S32R45_I2C1_SDA	(616 - 512)
#define SIUL2_PA_14_IMCR_S32R45_I2C1_SCLK	(615 - 512)

/* I2C0 - Serial Data Input */
#define SIUL2_MSCR_S32R45_PAD_CTRL_I2C0_SDA \
	(SIUL2_MSCR_MUX_MODE_ALT1 | \
	 SIUL2_MSCR_S32_G1_OBE_EN | \
	 SIUL2_MSCR_S32_G1_IBE_EN | \
	 SIUL2_MSCR_S32_G1_ODE_EN)
#define SIUL2_IMCR_S32R45_PAD_CTRL_I2C0_SDA (SIUL2_MSCR_MUX_MODE_ALT2)

/* I2C0 - Serial Clock Input */
#define SIUL2_MSCR_S32R45_PAD_CTRL_I2C0_SCLK \
	(SIUL2_MSCR_MUX_MODE_ALT1 | \
	 SIUL2_MSCR_S32_G1_OBE_EN | \
	 SIUL2_MSCR_S32_G1_IBE_EN | \
	 SIUL2_MSCR_S32_G1_ODE_EN)
#define SIUL2_IMCR_S32R45_PAD_CTRL_I2C0_SCLK (SIUL2_MSCR_MUX_MODE_ALT2)

/* I2C1 - Serial Data Input */
#define SIUL2_MSCR_S32R45_PAD_CTRL_I2C1_SDA \
	(SIUL2_MSCR_MUX_MODE_ALT3 | \
	 SIUL2_MSCR_S32_G1_OBE_EN | \
	 SIUL2_MSCR_S32_G1_IBE_EN | \
	 SIUL2_MSCR_S32_G1_ODE_EN)
#define SIUL2_IMCR_S32R45_PAD_CTRL_I2C1_SDA (SIUL2_MSCR_MUX_MODE_ALT2)

/* I2C1 - Serial Clock Input */
#define SIUL2_MSCR_S32R45_PAD_CTRL_I2C1_SCLK \
	(SIUL2_MSCR_MUX_MODE_ALT4 | \
	 SIUL2_MSCR_S32_G1_OBE_EN | \
	 SIUL2_MSCR_S32_G1_IBE_EN | \
	 SIUL2_MSCR_S32_G1_ODE_EN)
#define SIUL2_IMCR_S32R45_PAD_CTRL_I2C1_SCLK (SIUL2_MSCR_MUX_MODE_ALT2)

/* GMAC1 */
#define SIUL2_MSCR_S32R45_G1_PH6          102
#define SIUL2_MSCR_S32R45_G1_PH7          103
#define SIUL2_MSCR_S32R45_G1_GMAC1_MDI_IN 659

#endif /* SIUL_S32R45_H */

