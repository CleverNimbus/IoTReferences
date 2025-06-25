/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_check.h"
#include "nand.h"
#include "spi_nand_oper.h"
#include "nand_flash_devices.h"

static const char *TAG = "nand_winbond";

esp_err_t spi_nand_winbond_init(spi_nand_flash_device_t *dev)
{
    uint8_t device_id_buf[2];
    spi_nand_transaction_t t = {
        .command = CMD_READ_ID,
        .address = 0,
        .address_bytes = 2,
        .miso_len = 2,
        .miso_data = device_id_buf,
        .flags = SPI_TRANS_USE_RXDATA,
    };
    spi_nand_execute_transaction(dev, &t);
    dev->chip.has_quad_enable_bit = 0;
    dev->chip.quad_enable_bit_pos = 0;
    uint16_t device_id = (device_id_buf[0] << 8) + device_id_buf[1];
    dev->chip.read_page_delay_us = 10;
    dev->chip.erase_block_delay_us = 2500;
    dev->chip.program_page_delay_us = 320;
    ESP_LOGD(TAG, "%s: device_id: %x\n", __func__, device_id);
    switch (device_id) {
    case WINBOND_DI_AA20:
    case WINBOND_DI_BA20:
        dev->chip.num_blocks = 512;
        break;
    case WINBOND_DI_AA21:
    case WINBOND_DI_BA21:
    case WINBOND_DI_BC21:
        dev->chip.num_blocks = 1024;
        break;
    default:
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}
