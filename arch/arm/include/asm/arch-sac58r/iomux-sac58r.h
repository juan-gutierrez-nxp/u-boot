/*
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __IOMUX_SAC58R_H__
#define __IOMUX_SAC58R_H__

#include <asm/imx-common/iomux-v3.h>


/* Pad control groupings */
#define SAC58R_UART_PAD_CTRL	(PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED | \
			PAD_CTL_DSE_25ohm | PAD_CTL_OBE_IBE_ENABLE)
#define SAC58R_UART_PAD_CTRL_RX	(PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED | \
			PAD_CTL_DSE_25ohm | PAD_CTL_IBE_ENABLE)
#define SAC58R_SDHC_PAD_CTRL	(PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_HIGH | \
			PAD_CTL_DSE_20ohm | PAD_CTL_OBE_IBE_ENABLE)
#define SAC58R_ENET_PAD_CTRL (PAD_CTL_PUS_47K_UP | PAD_CTL_SPEED_HIGH | \
			PAD_CTL_DSE_50ohm | PAD_CTL_OBE_IBE_ENABLE)
#define SAC58R_DDR_PAD_CTRL	PAD_CTL_DSE_25ohm
#define SAC58R_I2C_PAD_CTRL	(PAD_CTL_PUS_47K_UP | PAD_CTL_DSE_50ohm | \
				PAD_CTL_SPEED_HIGH | PAD_CTL_OBE_IBE_ENABLE)
#define SAC58R_NFC_IO_PAD_CTRL (PAD_CTL_SPEED_HIGH | PAD_CTL_SRE | \
				PAD_CTL_DSE_50ohm | PAD_CTL_PUS_47K_UP | PAD_CTL_OBE_IBE_ENABLE)
#define SAC58R_NFC_SIG_PAD_CTRL	(PAD_CTL_SPEED_HIGH | PAD_CTL_SRE | \
				PAD_CTL_DSE_50ohm | PAD_CTL_PUS_47K_UP | PAD_CTL_OBE_ENABLE)
#define SAC58R_NFC_SIG_RB_PAD_CTRL	(PAD_CTL_SPEED_HIGH | PAD_CTL_SRE | \
				PAD_CTL_DSE_50ohm | PAD_CTL_IBE_ENABLE)
#define SAC58R_SAI_PAD_CTRL	(PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_HIGH | \
				PAD_CTL_DSE_25ohm | PAD_CTL_OBE_IBE_ENABLE)


enum {
	SAC58R_PAD_PH14__UART0_TX		= IOMUX_PAD(0x03b8, 0x03b8, 1, 0x0b50, 2, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PH15__UART0_RX		= IOMUX_PAD(0x03bc, 0x03bc, 0, 0x0b20, 2, SAC58R_UART_PAD_CTRL_RX),
	SAC58R_PAD_PF11__UART1_TX		= IOMUX_PAD(0x02ac, 0x02ac, 1, 0x0b54, 2, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PF12__UART1_RX		= IOMUX_PAD(0x02b0, 0x02b0, 0, 0x0b28, 2, SAC58R_UART_PAD_CTRL_RX),
	SAC58R_PAD_PL5__UART2_TX		= IOMUX_PAD(0x0594, 0x0594, 1, 0x0b58, 2, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PL4__UART2_RX		= IOMUX_PAD(0x03bc, 0x03bc, 0, 0x0b30, 2, SAC58R_UART_PAD_CTRL_RX),
	SAC58R_PAD_PE28__UART3_TX		= IOMUX_PAD(0x0270, 0x0270, 1, 0x0b38, 2, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PE27__UART3_RX		= IOMUX_PAD(0x026c, 0x026c, 0, 0x0b5c, 2, SAC58R_UART_PAD_CTRL_RX),
	SAC58R_PAD_PH0__UART4_TX		= IOMUX_PAD(0x0380, 0x0380, 1, 0x0b60, 3, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PH1__UART4_RX		= IOMUX_PAD(0x0384, 0x0384, 0, 0x0b40, 3, SAC58R_UART_PAD_CTRL_RX),
	SAC58R_PAD_PH9__UART5_TX		= IOMUX_PAD(0x03a4, 0x03a4, 1, 0x0b64, 3, SAC58R_UART_PAD_CTRL),
	SAC58R_PAD_PH10__UART5_RX		= IOMUX_PAD(0x03a8, 0x03a8, 0, 0x0b48, 3, SAC58R_UART_PAD_CTRL_RX),

	SAC58R_PAD_PF0__SDHC0_CLK		= IOMUX_PAD(0x0280, 0x0280, 1, 0x0dd8, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF1__SDHC0_CMD		= IOMUX_PAD(0x0284, 0x0284, 1, 0x0dc4, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF2__SDHC0_DAT0		= IOMUX_PAD(0x0288, 0x0288, 1, 0x0dc8, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF3__SDHC0_DAT1		= IOMUX_PAD(0x028c, 0x028c, 1, 0x0dcc, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF4__SDHC0_DAT2		= IOMUX_PAD(0x0290, 0x0290, 1, 0x0dd0, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF5__SDHC0_DAT3		= IOMUX_PAD(0x0294, 0x0294, 1, 0x0dd4, 2, SAC58R_SDHC_PAD_CTRL),

	SAC58R_PAD_PF26__SDHC1_CLK		= IOMUX_PAD(0x02e8, 0x02e8, 1, 0x0df8, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF27__SDHC1_CMD		= IOMUX_PAD(0x02ec, 0x02ec, 1, 0x0de4, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF22__SDHC1_DAT0		= IOMUX_PAD(0x02d8, 0x02d8, 1, 0x0de8, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF23__SDHC1_DAT1		= IOMUX_PAD(0x02dc, 0x02dc, 1, 0x0dec, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF19__SDHC1_DAT2		= IOMUX_PAD(0x02cc, 0x02cc, 1, 0x0df0, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PF20__SDHC1_DAT3		= IOMUX_PAD(0x02d0, 0x02d0, 1, 0x0df4, 2, SAC58R_SDHC_PAD_CTRL),

	SAC58R_PAD_PC12__SDHC2_CLK		= IOMUX_PAD(0x0130, 0x0130, 2, 0x0e28, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC13__SDHC2_CMD		= IOMUX_PAD(0x0134, 0x0134, 2, 0x0e00, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC9__SDHC2_DAT0		= IOMUX_PAD(0x0124, 0x0124, 2, 0x0e08, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC6__SDHC2_DAT1		= IOMUX_PAD(0x0118, 0x0118, 2, 0x0e0c, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC5__SDHC2_DAT2		= IOMUX_PAD(0x0114, 0x0114, 2, 0x0e10, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC2__SDHC2_DAT3		= IOMUX_PAD(0x0108, 0x0108, 2, 0x0e14, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC10__SDHC2_DAT4		= IOMUX_PAD(0x0128, 0x0128, 2, 0x0e18, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC11__SDHC2_DAT5		= IOMUX_PAD(0x012c, 0x012c, 2, 0x0e1c, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC7__SDHC2_DAT6		= IOMUX_PAD(0x011c, 0x011c, 2, 0x0e20, 2, SAC58R_SDHC_PAD_CTRL),
	SAC58R_PAD_PC8__SDHC2_DAT7		= IOMUX_PAD(0x0120, 0x0120, 2, 0x0e24, 2, SAC58R_SDHC_PAD_CTRL),

	SAC58R_PAD_DDR_A15__DDR_A15		= IOMUX_PAD(0x006c, 0x006c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A14__DDR_A14		= IOMUX_PAD(0x0068, 0x0068, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A13__DDR_A13		= IOMUX_PAD(0x0064, 0x0064, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A12__DDR_A12		= IOMUX_PAD(0x0060, 0x0060, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A11__DDR_A11		= IOMUX_PAD(0x005c, 0x005c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A10__DDR_A10		= IOMUX_PAD(0x0058, 0x0058, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A9__DDR_A9		= IOMUX_PAD(0x0054, 0x0054, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A8__DDR_A8		= IOMUX_PAD(0x0050, 0x0050, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A7__DDR_A7		= IOMUX_PAD(0x004c, 0x004c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A6__DDR_A6		= IOMUX_PAD(0x0048, 0x0048, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A5__DDR_A5		= IOMUX_PAD(0x0044, 0x0044, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A4__DDR_A4		= IOMUX_PAD(0x0040, 0x0040, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A3__DDR_A3		= IOMUX_PAD(0x003c, 0x003c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A2__DDR_A2		= IOMUX_PAD(0x0038, 0x0038, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A1__DDR_A1		= IOMUX_PAD(0x0034, 0x0034, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_A0__DDR_A0		= IOMUX_PAD(0x0030, 0x0030, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_BA2__DDR_BA_2	= IOMUX_PAD(0x002c, 0x002c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_BA1__DDR_BA_1	= IOMUX_PAD(0x0028, 0x0028, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_BA0__DDR_BA_0	= IOMUX_PAD(0x0024, 0x0024, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CAS__DDR_CAS		= IOMUX_PAD(0x0008, 0x0008, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CKE1__DDR_CKE1	= IOMUX_PAD(0x0018, 0x0018, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CKE0__DDR_CKE0	= IOMUX_PAD(0x0014, 0x0014, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CLK0__DDR_CLK0	= IOMUX_PAD(0x0004, 0x0004, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CS_B1__DDR_CS_B1	= IOMUX_PAD(0x0020, 0x0020, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_CS_B0__DDR_CS_B0	= IOMUX_PAD(0x001c, 0x001c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D31__DDR_D31		= IOMUX_PAD(0x010c, 0x010c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D30__DDR_D30		= IOMUX_PAD(0x0108, 0x0108, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D29__DDR_D29		= IOMUX_PAD(0x0104, 0x0104, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D28__DDR_D28		= IOMUX_PAD(0x0100, 0x0100, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D27__DDR_D27		= IOMUX_PAD(0x00fc, 0x00fc, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D26__DDR_D26		= IOMUX_PAD(0x00f8, 0x00f8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D25__DDR_D25		= IOMUX_PAD(0x00f4, 0x00f4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D24__DDR_D24		= IOMUX_PAD(0x00f0, 0x00f0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D23__DDR_D23		= IOMUX_PAD(0x00ec, 0x00ec, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D22__DDR_D22		= IOMUX_PAD(0x00e8, 0x00e8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D21__DDR_D21		= IOMUX_PAD(0x00e4, 0x00e4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D20__DDR_D20		= IOMUX_PAD(0x00e0, 0x0ee0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D19__DDR_D19		= IOMUX_PAD(0x00dc, 0x00dc, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D18__DDR_D18		= IOMUX_PAD(0x00d8, 0x00d8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D17__DDR_D17		= IOMUX_PAD(0x00d4, 0x00d4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D16__DDR_D16		= IOMUX_PAD(0x00d0, 0x00d0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D15__DDR_D15		= IOMUX_PAD(0x00cc, 0x00cc, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D14__DDR_D14		= IOMUX_PAD(0x00c8, 0x00c8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D13__DDR_D13		= IOMUX_PAD(0x00c4, 0x00c4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D12__DDR_D12		= IOMUX_PAD(0x00c0, 0x00c0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D11__DDR_D11		= IOMUX_PAD(0x00bc, 0x00bc, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D10__DDR_D10		= IOMUX_PAD(0x00b8, 0x00b8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D9__DDR_D9		= IOMUX_PAD(0x00b4, 0x00b4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D8__DDR_D8		= IOMUX_PAD(0x00b0, 0x00b0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D7__DDR_D7		= IOMUX_PAD(0x00ac, 0x00ac, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D6__DDR_D6		= IOMUX_PAD(0x00a8, 0x00a8, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D5__DDR_D5		= IOMUX_PAD(0x00a4, 0x00a4, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D4__DDR_D4		= IOMUX_PAD(0x00a0, 0x00a0, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D3__DDR_D3		= IOMUX_PAD(0x009c, 0x009c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D2__DDR_D2		= IOMUX_PAD(0x0098, 0x0098, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D1__DDR_D1		= IOMUX_PAD(0x0094, 0x0094, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_D0__DDR_D0		= IOMUX_PAD(0x0090, 0x0090, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DM1__DDR_DM3		= IOMUX_PAD(0x007c, 0x007c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DM0__DDR_DM2		= IOMUX_PAD(0x0078, 0x0078, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DM1__DDR_DM1		= IOMUX_PAD(0x0074, 0x0074, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DM0__DDR_DM0		= IOMUX_PAD(0x0070, 0x0070, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DQS3__DDR_DQS3	= IOMUX_PAD(0x008c, 0x008c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DQS0__DDR_DQS2	= IOMUX_PAD(0x0088, 0x0088, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DQS1__DDR_DQS1	= IOMUX_PAD(0x0084, 0x0084, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_DQS0__DDR_DQS0	= IOMUX_PAD(0x0080, 0x0080, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_RAS__DDR_RAS		= IOMUX_PAD(0x000c, 0x000c, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_WE__DDR_WE_B		= IOMUX_PAD(0x0010, 0x0010, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_ODT1__DDR_ODT0	= IOMUX_PAD(0x0110, 0x0110, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_ODT0__DDR_ODT1	= IOMUX_PAD(0x0114, 0x0114, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),
	SAC58R_PAD_DDR_ZQ__DDR_ZQ		= IOMUX_PAD(0x0118, 0x0118, 0, __NA_, 0, SAC58R_DDR_PAD_CTRL),

	SAC58R_PAD_PA0_I2C0_SDA			= IOMUX_PAD(0x0000, 0x0000, 1, 0x0c34, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PA1_I2C0_SCL			= IOMUX_PAD(0x0004, 0x0004, 1, 0x0c30, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PE19_I2C1_SCL		= IOMUX_PAD(0x024c, 0x024c, 1, 0x0c38, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PE10_I2C1_SDA		= IOMUX_PAD(0x0250, 0x0250, 1, 0x0c3c, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PA8_I2C2_SDA			= IOMUX_PAD(0x0020, 0x0020, 1, 0x0c44, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PA9_I2C2_SCL			= IOMUX_PAD(0x0021, 0x0021, 1, 0x0c40, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PH2_I2C3_SCL			= IOMUX_PAD(0x0388, 0x0388, 1, 0x0c48, 2, SAC58R_I2C_PAD_CTRL),
	SAC58R_PAD_PH4_I2C3_SDA			= IOMUX_PAD(0x0390, 0x0390, 1, 0x0c4c, 2, SAC58R_I2C_PAD_CTRL),

	SAC58R_PAD_PH11_SAI1_TX_BCLK 	= IOMUX_PAD(0x03AC, 0x03AC, 2, 0x0994, 2, SAC58R_SAI_PAD_CTRL),
	SAC58R_PAD_PH12_SAI1_TX_SYNC 	= IOMUX_PAD(0x03B0, 0x03B0, 1, 0x0990, 2, SAC58R_SAI_PAD_CTRL),
	SAC58R_PAD_PH16_SAI1_RX_BCLK 	= IOMUX_PAD(0x03C0, 0x03C0, 1, 0x0928, 2, SAC58R_SAI_PAD_CTRL),
	SAC58R_PAD_PH17_SAI1_RX_SYNC 	= IOMUX_PAD(0x03C4, 0x03C4, 1, 0x0924, 2, SAC58R_SAI_PAD_CTRL),
	SAC58R_PAD_PH18_SAI1_RX_DATA 	= IOMUX_PAD(0x03C8, 0x03C8, 1, 0x092c, 2, SAC58R_SAI_PAD_CTRL),
	SAC58R_PAD_PH19_SAI1_TX_DATA 	= IOMUX_PAD(0x03CC, 0x03CC, 1,  __NA_, 0, SAC58R_SAI_PAD_CTRL),
};

#endif	/* __IOMUX_SAC58R_H__ */