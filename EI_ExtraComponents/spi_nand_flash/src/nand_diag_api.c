/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "spi_nand_flash.h"
#include "nand.h"
#include "nand_private/nand_impl_wrap.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "nand_diag";

esp_err_t nand_get_bad_block_stats(spi_nand_flash_device_t *flash, uint32_t *bad_block_count)
{
    esp_err_t ret = ESP_OK;
    uint32_t bad_blocks = 0;
    uint32_t num_blocks;
    spi_nand_flash_get_block_num(flash, &num_blocks);
    for (uint32_t blk = 0; blk < num_blocks; blk++) {
        bool is_bad = false;
        ret = nand_wrap_is_bad(flash, blk, &is_bad);
        if (ret == ESP_OK && is_bad) {
            bad_blocks++;
            ESP_LOGD(TAG, "bad block num=%"PRIu32"", blk);
        } else if (ret) {
            ESP_LOGE(TAG, "Failed to get bad block status for blk=%"PRIu32"", blk);
            return ret;
        }
    }
    *bad_block_count = bad_blocks;
    return ret;
}

static bool is_ecc_exceed_threshold(spi_nand_flash_device_t *handle)
{
    uint8_t min_bits_corrected = 0;
    bool ret = false;
    if (handle->chip.ecc_data.ecc_corrected_bits_status == STAT_ECC_1_TO_3_BITS_CORRECTED) {
        min_bits_corrected = 1;
    } else if (handle->chip.ecc_data.ecc_corrected_bits_status == STAT_ECC_4_TO_6_BITS_CORRECTED) {
        min_bits_corrected = 4;
    } else if (handle->chip.ecc_data.ecc_corrected_bits_status == STAT_ECC_7_8_BITS_CORRECTED) {
        min_bits_corrected = 7;
    }

    if (min_bits_corrected >= handle->chip.ecc_data.ecc_data_refresh_threshold) {
        ret = true;
    }
    return ret;
}

esp_err_t nand_get_ecc_stats(spi_nand_flash_device_t *flash)
{
    esp_err_t ret = ESP_OK;
    uint32_t sector_size, block_size, num_blocks;
    uint32_t ecc_err_total_count = 0;
    uint32_t ecc_err_exceeding_threshold_count = 0;
    uint32_t ecc_err_not_corrected_count = 0;

    spi_nand_flash_get_sector_size(flash, &sector_size);
    spi_nand_flash_get_block_size(flash, &block_size);
    spi_nand_flash_get_block_num(flash, &num_blocks);
    uint32_t pages_per_block = block_size / sector_size;
    uint32_t num_pages = num_blocks * pages_per_block;

    bool is_free = true;
    for (uint32_t page = 0; page < num_pages; page++) {
        ret = nand_wrap_is_free(flash, page, &is_free);
        if (!is_free) {
            ret = nand_wrap_get_ecc_status(flash, page);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ecc error for page=%" PRIu32 "", page);
                return ret;
            }
            if (flash->chip.ecc_data.ecc_corrected_bits_status) {
                ecc_err_total_count++;
                if (flash->chip.ecc_data.ecc_corrected_bits_status == STAT_ECC_NOT_CORRECTED) {
                    ecc_err_not_corrected_count++;
                    ESP_LOGD(TAG, "ecc error not corrected for page=%" PRIu32 "", page);
                } else if (is_ecc_exceed_threshold(flash)) {
                    ecc_err_exceeding_threshold_count++;
                }
            }
        }
    }

    ESP_LOGI(TAG, "\nTotal number of ECC errors: %"PRIu32"\nECC not corrected count: %"PRIu32"\nECC errors exceeding threshold (%d): %"PRIu32"\n",
             ecc_err_total_count, ecc_err_not_corrected_count, flash->chip.ecc_data.ecc_data_refresh_threshold, ecc_err_exceeding_threshold_count);
    return ret;
}
