#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* =========================================================
 *  wifi_manager.h
 *  Gestiona el modo AP+STA del ESP32:
 *    - AP  : punto de acceso para clientes del centro comercial
 *    - STA : se conecta a una red externa para dar Internet
 * ========================================================= */

/** Inicializa WiFi en modo AP+STA y bloquea hasta obtener IP en STA. */
esp_err_t wifi_manager_init(void);

/** Retorna el número de dispositivos actualmente conectados al AP. */
uint8_t wifi_manager_get_connected_count(void);

/** Retorna true si el ESP32 (STA) tiene IP asignada. */
bool wifi_manager_is_sta_connected(void);

/** Retorna la dirección IP del AP (para mostrar en logs). */
const char *wifi_manager_get_ap_ip(void);
