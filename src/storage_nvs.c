/*******************************************************************************
 *  _____       ______   ____
 * |_   _|     |  ____|/ ____|  Institute of Embedded Systems
 *   | |  _ __ | |__  | (___    Zurich University of Applied Sciences
 *   | | | '_ \|  __|  \___ \   8401 Winterthur, Switzerland
 *  _| |_| | | | |____ ____) |
 * |_____|_| |_|______|_____/
 *
 *******************************************************************************
  * Application to Store data in NVS
 *
 * 26.02.2025, wyssala1@students.zhaw.ch
 ******************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>

#include "storage_nvs.h"

LOG_MODULE_REGISTER(storage_nvs, LOG_LEVEL_INF);

static struct nvs_fs fs;

#define NVS_PARTITION           storage_partition
#define NVS_PARTITION_DEVICE    FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET    FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define ADDRESS_ID 1
#define KEY_ID 2
#define RBT_CNT_ID 3
#define STRING_ID 4
#define LONG_ID 5

static uint32_t reboot_counter = 0U;

/**
 * @brief Initialisiert das NVS-Subsystem.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_init(void) {
    int rc;
    struct flash_pages_info info;

    fs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs.flash_device)) {
        LOG_ERR("Flash device %s is not ready", fs.flash_device->name);
        return -ENODEV;
    }

    fs.offset = NVS_PARTITION_OFFSET;
    rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (rc) {
        LOG_ERR("Unable to get page info, rc=%d", rc);
        return rc;
    }
    
    fs.sector_size = info.size;
    fs.sector_count = 3;

    rc = nvs_mount(&fs);
    if (rc) {
        LOG_ERR("NVS mount failed, rc=%d", rc);
        return rc;
    }

    LOG_INF("NVS successfully mounted");

    /* Lese den Reboot-Counter */
    rc = storage_nvs_read(RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
    if (rc > 0) {
        LOG_INF("Reboot Counter: %u", reboot_counter);
    } else {
        LOG_INF("No Reboot Counter found, initializing it.");
        reboot_counter = 0;
        storage_nvs_write(RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
    }

    return 0;
}

/**
 * @brief Speichert eine Variable im Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @param data Pointer auf die zu speichernden Daten.
 * @param length Länge der zu speichernden Daten in Bytes.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_write(uint16_t id, const void *data, size_t length) {
    int rc = nvs_write(&fs, id, data, length);
    if (rc < 0) {
        LOG_ERR("NVS write failed: %d", rc);
    } else {
        LOG_INF("Stored %zu bytes under ID %u", length, id);
    }
    return rc;
}

/**
 * @brief Liest eine Variable aus dem Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @param data Pointer auf den Speicher, in den die Daten gelesen werden.
 * @param length Maximale Länge der zu lesenden Daten.
 * @return Größe der gelesenen Daten bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_read(uint16_t id, void *data, size_t length) {
    int rc = nvs_read(&fs, id, data, length);
    if (rc < 0) {
        LOG_ERR("NVS read failed: %d", rc);
    } else {
        LOG_INF("Read %d bytes from ID %u", rc, id);
    }
    return rc;
}

/**
 * @brief Löscht eine Variable aus dem Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_delete(uint16_t id) {
    int rc = nvs_delete(&fs, id);
    if (rc < 0) {
        LOG_ERR("Failed to delete ID %u, rc=%d", id, rc);
    } else {
        LOG_INF("Deleted ID %u", id);
    }
    return rc;
}

/**
 * @brief Erhöht den Reboot-Counter und speichert ihn.
 */
void storage_nvs_increment_reboot_counter(void) {
    reboot_counter++;
    storage_nvs_write(RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
    LOG_INF("Reboot Counter updated: %u", reboot_counter);
}
