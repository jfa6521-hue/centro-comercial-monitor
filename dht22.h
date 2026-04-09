#pragma once
#include "esp_err.h"
#include "driver/gpio.h"

/* =========================================================
 *  dht22.h — Driver bit-bang para sensor DHT22
 *  Protocolo: señal de inicio → 40 bits (16 RH + 16 T + 8 CRC)
 * ========================================================= */

/** Inicializa el GPIO del DHT22 como open-drain. */
void dht22_init(gpio_num_t gpio);

/**
 * Lee temperatura y humedad.
 * @param[out] temperature  Temperatura en °C
 * @param[out] humidity     Humedad relativa en %
 * @return ESP_OK | ESP_ERR_TIMEOUT | ESP_ERR_INVALID_CRC
 */
esp_err_t dht22_read(float *temperature, float *humidity);
