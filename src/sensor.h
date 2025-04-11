#ifndef SENSOR_H
#define SENSOR_H


/**
 * @brief Führt eine vollständige Messung durch.
 *
 * Diese Funktion weckt den Sensor auf, startet eine Messung, parst die
 * Rohdaten und speichert die berechneten CO2-, Temperatur- und Feuchtigkeitswerte.
 *
 * @param co2 Zeiger auf die Variable, in der der CO2-Wert gespeichert wird.
 * @param temperature Zeiger auf die Variable, in der der Temperaturwert gespeichert wird.
 * @param humidity Zeiger auf die Variable, in der der Feuchtigkeitswert gespeichert wird.
 */
void sensor_measure(float *co2, float *temperature, float *humidity);
#endif