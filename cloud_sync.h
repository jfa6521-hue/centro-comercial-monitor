#pragma once
#include "esp_err.h"

/* =========================================================
 *  cloud_sync.h
 *  Envía el estado del sistema a Firebase Realtime Database
 *  mediante HTTP PUT (JSON) cada CLOUD_SYNC_MS milisegundos.
 *
 *  Estructura JSON enviada:
 *  {
 *    "internet"        : true,
 *    "connected_ap"    : 3,
 *    "temperature"     : 24.5,
 *    "humidity"        : 61.2,
 *    "people_inside"   : 8,
 *    "nfc_total"       : 23,
 *    "nfc_last_access" : "2026-04-09T10:30:00",
 *    "dht_ok"          : true,
 *    "timestamp"       : 1744200000
 *  }
 * ========================================================= */

/** Tarea FreeRTOS — lanzar con xTaskCreate desde app_main. */
void cloud_sync_task(void *pvParam);
