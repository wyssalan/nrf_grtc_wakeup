/*
 * Copyright (c) 2023, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>
 #include <zephyr/drivers/sensor.h>
 #include <zephyr/drivers/i2c.h>
 #include <zephyr/logging/log.h>
 #include <zephyr/pm/device.h>
 #include <zephyr/device.h>
 
 LOG_MODULE_REGISTER(SENSOR);
 
 #define SENSOR_I2C_ADDR 0x62               // Ersetze mit der echten I2C-Adresse
 #define SENSOR_CMD_LEN 2
 
 const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c21)); // Passe dein I2C-Device an
 
 #define CRC8_POLYNOMIAL 0x31
 #define CRC8_INIT 0xff
 
 static uint8_t sensirion_common_generate_crc(const uint8_t *data, uint16_t count)
 {
     uint16_t current_byte;
     uint8_t crc = CRC8_INIT;
     uint8_t crc_bit;
     /* calculates 8-Bit checksum with given polynomial */
     for (current_byte = 0; current_byte < count; ++current_byte)
     {
         crc ^= (data[current_byte]);
         for (crc_bit = 8; crc_bit > 0; --crc_bit)
         {
             if (crc & 0x80)
                 crc = (crc << 1) ^ CRC8_POLYNOMIAL;
             else
                 crc = (crc << 1);
         }
     }
     return crc;
 }
 
 static void sensor_shut_down(void)
 {
     if (!device_is_ready(i2c_dev))
     {
         LOG_ERR("I2C device not ready");
         return;
     }
     uint8_t cmd[2] = {0x36, 0xe0};
     int ret = i2c_write(i2c_dev, cmd, sizeof(cmd), SENSOR_I2C_ADDR);
     if (ret < 0)
     {
         LOG_ERR("Failed to send shutdown message (err %d)", ret);
     }
     else
     {
         LOG_INF("Sensor shut down.");
         k_msleep(500); // Warte 500ms nach dem Befehl
     }
     return;
 }
 
 static void sensor_wake_up(void)
 {
     if (!device_is_ready(i2c_dev))
     {
         LOG_ERR("I2C device not ready");
         return;
     }
     uint8_t cmd[2] = {0x36, 0xf6};
     i2c_write(i2c_dev, cmd, sizeof(cmd), SENSOR_I2C_ADDR);
     k_msleep(30);
     int ret = i2c_write(i2c_dev, cmd, sizeof(cmd), SENSOR_I2C_ADDR);
     if (ret < 0)
     {
         LOG_ERR("Failed to send wakeup message (err %d)", ret);
     }
     else
     {
         LOG_INF("Sensor wakeup");
         k_msleep(500); // Warte 500ms nach dem Befehl
     }
     return;
 }
 
 static void sensor_singleshot(uint8_t *data)
 {
     while (!device_is_ready(i2c_dev))
     {
         LOG_ERR("I2C device not ready");
         k_msleep(50);
     }
     uint8_t cmd[2] = {0x21, 0x9d};
     int ret = i2c_write(i2c_dev, cmd, sizeof(cmd), SENSOR_I2C_ADDR);
     if (ret < 0)
     {
         LOG_ERR("Failed to send measure_single_shot  (err %d)", ret);
     }
     else
     {
         LOG_INF("Sensor measure_single_shot ");
         k_msleep(5000);
         uint8_t cmd[2] = {0xec, 0x05};
         int ret = i2c_write(i2c_dev, cmd, sizeof(cmd), SENSOR_I2C_ADDR);
         if (ret < 0)
         {
             LOG_ERR("Failed to send read_measurement  (err %d)", ret);
         }
         else
         {
             LOG_INF("Sensor read_measurement ");
             k_msleep(1);
             ret = i2c_read(i2c_dev, data, 9, SENSOR_I2C_ADDR);
             if (ret < 0)
             {
                 LOG_ERR("PPM not readed");
             }
         }
     }
     return;
 }
 // Hilfsfunktion, um drei Bytes zu einem 16-Bit-Wert zu konvertieren
 static uint16_t bytes_to_word(uint8_t msb, uint8_t lsb)
 {
     return (msb << 8) | lsb;
 }
 static void parse_sensor_data(uint8_t meas_data[9], float *co2, float *temperature, float *humidity)
 {
     uint16_t co2_raw = bytes_to_word(meas_data[0], meas_data[1]);
     uint16_t temp_raw = bytes_to_word(meas_data[3], meas_data[4]);
     uint16_t hum_raw = bytes_to_word(meas_data[6], meas_data[7]);
 
     // Umwandlung gemäß Formeln aus der Tabelle
     *co2 = (float)co2_raw; // direkt in ppm
     *temperature = -45 + 175 * ((float)temp_raw / 65535);
     *humidity = 100 * ((float)hum_raw / 65535);
     return;
 }
 void sensor_measure(float *co2, float *temperature, float *humidity)
 {
     // struct sensor_value co2, temperature, humidity;
     const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(co2));
 
     while (!device_is_ready(dev))
     {
         LOG_ERR("Sensor %s is not ready", dev->name);
         k_msleep(50);
     }
 
     uint8_t meas_data[9];
     sensor_wake_up();
     sensor_singleshot(meas_data);
     // float co2, temperature, humidity;
 
     parse_sensor_data(meas_data, co2, temperature, humidity);
 
     sensor_shut_down();
     return;
 }