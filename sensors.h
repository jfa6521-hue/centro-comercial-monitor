#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

/* =========================================================
 *  sensors.h
 *  Gestiona:
 *    - DHT22  (temperatura / humedad)
 *    - Contador de personas (sensor IR — GPIO interrupt)
 *    - Eventos NFC        (pulso GPIO del módulo NFC)
 *    - Verificación de Internet (ping HTTP)
 * ========================================================= */

/** Estado global compartido del sistema */
typedef struct {
    float    temperature;       /**< °C — última lectura DHT22           */
    float    humidity;          /**< %  — última lectura DHT22           */
    bool     dht_ok;            /**< false si el DHT22 falla             */

    uint32_t people_inside;     /**< Personas dentro del comercial       */
    uint32_t nfc_total;         /**< Accesos NFC acumulados              */
    char     nfc_last_ts[32];   /**< Timestamp ISO último acceso NFC     */

    bool     internet_ok;       /**< true = hay conexión a Internet      */
} system_state_t;

/** Inicializa sensores y GPIOs de interrupción. */
esp_err_t sensors_init(void);

/**
 * Tarea FreeRTOS de sensores — lanza desde app_main con xTaskCreate.
 * Lee DHT22 y verifica Internet periódicamente.
 */
void sensors_task(void *pvParam);

/** Acceso thread-safe al estado del sistema. */
void sensors_get_state(system_state_t *out);
