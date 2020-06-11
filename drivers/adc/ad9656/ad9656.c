/***************************************************************************//**
 * @file ad9656.c
 * @brief Implementation of AD9656 Driver.
 * @author DHotolea (dan.hotoleanu@analog.com)
 ********************************************************************************
 * Copyright 2014-2016(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights
 * of one or more patent holders. This license does not release you
 * from the requirement that you obtain separate licenses from these
 * patent holders to use this software.
 * - Use of the software either in source or binary form, must be run
 * on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform_drivers.h"
#include "ad9656.h"

/***************************************************************************//**
 * @brief ad9656_spi_read
 *******************************************************************************/
int32_t ad9656_spi_read(struct ad9656_dev *dev,
			uint16_t reg_addr,
			uint8_t *reg_data)
{
	uint8_t buf[3];

	int32_t ret;

	// the MSB of byte 0 indicates a r/w operation, following 7 bits are the 
	// bits 14-8 of the address of the register that is accessed. Byte 1 
	// contains the bits 7-0 of the address of the register.
	buf[0] = 0x80 | (reg_addr >> 8);
	buf[1] = reg_addr & 0xFF;
	buf[2] = 0x00;

	ret = spi_write_and_read(dev->spi_desc,
				 buf,
				 3);
	*reg_data = buf[2];

	return ret;
}

/***************************************************************************//**
 * @brief ad9656_spi_write
 *******************************************************************************/
int32_t ad9656_spi_write(struct ad9656_dev *dev,
			 uint16_t reg_addr,
			 uint8_t reg_data)
{
	uint8_t buf[3];

	int32_t ret;

	// the MSB of byte 0 indicates a r/w operation, following 7 bits are the 
	// bits 14-8 of the address of the register that is accessed. Byte 1 
	// contains the bits 7-0 of the address of the register. Byte 2 contains
	// the data to be written.
	buf[0] = reg_addr >> 8;
	buf[1] = reg_addr & 0xFF;
	buf[2] = reg_data;

	ret = spi_write_and_read(dev->spi_desc,
				 buf,
				 3);

	return ret;
}

/***************************************************************************//**
 * @brief ad9656_setup
 *******************************************************************************/
int32_t ad9656_test(struct ad9656_dev *dev,
		    uint32_t test_mode)
{
	ad9656_spi_write(dev,
			 AD9656_REG_ADC_TEST_MODE,
			 test_mode);
	if (test_mode == AD9656_TEST_OFF)
		ad9656_spi_write(dev,
				 AD9656_REG_OUTPUT_MODE,
				 AD9656_FORMAT_2S_COMPLEMENT);
	else
		ad9656_spi_write(dev,
				 AD9656_REG_OUTPUT_MODE,
				 AD9656_FORMAT_OFFSET_BINARY);
	return(0);
}

/***************************************************************************//**
 * @brief ad9656_setup
 *******************************************************************************/
int32_t ad9656_setup(struct ad9656_dev **device,
		     const struct ad9656_init_param *init_param)
{
	uint8_t chip_id;
	uint8_t pll_stat;
	int32_t ret;
	struct ad9656_dev *dev;

	ret = 0;

	dev = (struct ad9656_dev *)malloc(sizeof(*dev));
	if (!dev)
		return -1;

	/* SPI */
	ret = spi_init(&dev->spi_desc, &init_param->spi_init);

	ad9656_spi_read(dev,
			AD9656_REG_CHIP_ID,
			&chip_id);
	if(chip_id != AD9656_CHIP_ID) {
		printf("AD9656: Invalid CHIP ID (0x%x).\n", chip_id);
		return -1;
	}

	ad9656_spi_write(dev,
			 AD9656_SPI_CONFIG,
			 0x3C);	// RESET
	mdelay(250);

	ad9656_spi_write(dev,
			 AD9656_REG_LINK_CONTROL,
			 0x15);	// disable link, ilas enable
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_MF_CTRL,
			 0x1f);	// 32 frames per multiframe
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_M_CTRL,
			 0x03);	// 4 converters
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_CSN_CONFIG,
			 0x0d);	// converter resolution of 14-bit
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_SUBCLASS_CONFIG,
			 0x2f);	// subclass-1, N'=16
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_QUICK_CONFIG,
			 0x44);	// m=4, l=4
	ad9656_spi_write(dev,
			 AD9656_REG_JESD204B_SCR_L,
			 0x83);	// enable scrambling, l=4
	if (init_param->lane_rate_kbps < 2000000)
		ad9656_spi_write(dev,
				 AD9656_REG_JESD204B_LANE_RATE_CTRL,
				 0x08);	// low line rate mode must be enabled
	else
		ad9656_spi_write(dev,
				 AD9656_REG_JESD204B_LANE_RATE_CTRL,
				 0x00);	// low line rate mode must be disabled
	ad9656_spi_write(dev,
			 AD9656_REG_LINK_CONTROL,
			 0x14);	// link enable
	mdelay(250);

	ad9656_spi_read(dev,
			AD9656_REG_JESD204B_PLL_LOCK_STATUS,
			&pll_stat);
	if ((pll_stat & 0x80) != 0x80) {
		printf("AD9656: PLL is NOT locked!\n");
		ret = -1;
	}

	*device = dev;

	return ret;
}

/***************************************************************************//**
 * @brief Free the resources allocated by ad9656_setup().
 *
 * @param dev - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ad9656_remove(struct ad9656_dev *dev)
{
	int32_t ret;

	ret = spi_remove(dev->spi_desc);

	free(dev);

	return ret;
}
