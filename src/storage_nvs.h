#ifndef STORAGE_NVS_H
#define STORAGE_NVS_H

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Initialisiert das NVS-Subsystem.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_init(void);

/**
 * @brief Speichert eine Variable im Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @param data Pointer auf die zu speichernden Daten.
 * @param length Länge der zu speichernden Daten in Bytes.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_write(uint16_t id, const void *data, size_t length);

/**
 * @brief Liest eine Variable aus dem Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @param data Pointer auf den Speicher, in den die Daten gelesen werden.
 * @param length Maximale Länge der zu lesenden Daten.
 * @return Größe der gelesenen Daten bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_read(uint16_t id, void *data, size_t length);

/**
 * @brief Löscht eine Variable aus dem Flash.
 * @param id Eindeutige ID des Speicherobjekts.
 * @return 0 bei Erfolg, negativer Fehlercode bei Misserfolg.
 */
int storage_nvs_delete(uint16_t id);

/**
 * @brief Erhöht den Reboot-Counter und speichert ihn.
 */
void storage_nvs_increment_reboot_counter(void);

#endif // STORAGE_NVS_H
