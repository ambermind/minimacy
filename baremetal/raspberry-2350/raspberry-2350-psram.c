// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
   
#include "hardware/address_mapped.h"
#include "hardware/gpio.h"
#include "hardware/regs/addressmap.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/sync.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "../../src/minimacy.h"
#include "raspberry-2350.h"
#ifdef USE_PSRAM
void __no_inline_not_in_flash_func(qmiEnableQuad)(int clkDiv)
{
	qmi_hw->direct_csr = clkDiv << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
	while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
}

void __no_inline_not_in_flash_func(qmiDisableQuad)()
{
	qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
}

void __no_inline_not_in_flash_func(qmiSpiCmd)(io_wo_32 *Tx, uint8_t *Rx, int len)
{
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
	while(len--) {
		qmi_hw->direct_tx = *(Tx++);
		while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0);
		while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0);
		*(Rx++)=qmi_hw->direct_rx;
	}
	qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
}

io_wo_32 QmiSpiExitQuadMod[1]={QMI_DIRECT_TX_OE_BITS | QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB | 0xf5};
io_wo_32 QmiSpiReadId[7]={0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
io_wo_32 QmiSpiReset[2]={0x66, 0x99};
io_wo_32 QmiSpiEnterQuadMod[1]={0x35};

int __no_inline_not_in_flash_func(psram_init)()
{
	uint8_t spiOutput[8];
	int clkDiv=30;

	gpio_set_function(XIP_CSI_PIN, GPIO_FUNC_XIP_CS1);

	uint32_t savedInterrupts = save_and_disable_interrupts();

	qmiEnableQuad(clkDiv);
	qmiSpiCmd(QmiSpiExitQuadMod,spiOutput,1);
	qmiSpiCmd(QmiSpiReadId,spiOutput,7);
	uint8_t kgd = spiOutput[5];
	uint8_t eid = spiOutput[6];
	if (kgd != 0x5D)
	{
		qmiDisableQuad();
		restore_interrupts(savedInterrupts);
		return 0;
	}
	qmiSpiCmd(QmiSpiReset,spiOutput,2);
	qmiSpiCmd(QmiSpiEnterQuadMod,spiOutput,1);
	qmiDisableQuad();

	qmi_hw->m[1].timing =
		QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
		3 << QMI_M1_TIMING_SELECT_HOLD_LSB |
		1 << QMI_M1_TIMING_COOLDOWN_LSB | 1 << QMI_M1_TIMING_RXDELAY_LSB |
		16 << QMI_M1_TIMING_MAX_SELECT_LSB |
		7 << QMI_M1_TIMING_MIN_DESELECT_LSB |
		2 << QMI_M1_TIMING_CLKDIV_LSB;
	qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
						 QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB |
						 QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
						 QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
						 QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
	qmi_hw->m[1].rcmd = 0xeb << QMI_M1_RCMD_PREFIX_LSB | 0 << QMI_M1_RCMD_SUFFIX_LSB;
	qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
						 QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
						 QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
						 QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
						 QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
	qmi_hw->m[1].wcmd = 0x38 << QMI_M1_WCMD_PREFIX_LSB | 0 << QMI_M1_WCMD_SUFFIX_LSB;

	uint8_t sizeCode = (eid == 0x26)?2:(eid >> 5);
	int psramSize = (1024 * 1024)* (1<<(sizeCode+1));

	// Mark that we can write to PSRAM.
	xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
	restore_interrupts(savedInterrupts);
	return psramSize;
}
#endif