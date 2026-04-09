#pragma once

/* =========================================================
 *  app_config.h — Configuración central del proyecto
 *  Centro Comercial - Campos y Ondas 2026-1
 * ========================================================= */

/* ---------- WiFi — Access Point (AP) ---------- */
#define AP_SSID             "CentroComercial_AP"
#define AP_PASS             "comercial2026"   /* mín. 8 chars */
#define AP_CHANNEL          6
#define AP_MAX_CONNECTIONS  4

/* ---------- WiFi — Station (STA, acceso a Internet) ---------- */
#define STA_SSID            "mordecool"
#define STA_PASS            "12345679"
#define STA_MAX_RETRY       5

/* ---------- Firebase Realtime Database ----------
 *  1. Crea un proyecto en https://console.firebase.google.com
 *  2. Activa Realtime Database en modo "test" (o con reglas)
 *  3. Copia la URL y el secret de base de datos
 * ------------------------------------------------------- */
#define FIREBASE_HOST       "TU_PROYECTO-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET     "TU_DATABASE_SECRET"     /* Configuración > Cuentas de servicio */
#define FIREBASE_PATH       "/monitor"               /* Nodo raíz en la DB */

/* URL completa para PUT/PATCH */
#define FIREBASE_URL        "https://" FIREBASE_HOST FIREBASE_PATH \
                            ".json?auth=" FIREBASE_SECRET

/* ---------- URL ping para verificar Internet ---------- */
#define PING_URL            "http://clients3.google.com/generate_204"
#define PING_TIMEOUT_MS     3000

/* ---------- GPIO Sensores ---------- */
#define DHT22_GPIO          GPIO_NUM_4   /* Temperatura/Humedad */
#define PEOPLE_IR_GPIO      GPIO_NUM_5   /* Sensor IR — flanco bajada = persona */
#define NFC_EVENT_GPIO      GPIO_NUM_18  /* Pulso LOW del módulo NFC al conceder acceso */

/* ---------- Tiempos ---------- */
#define CLOUD_SYNC_MS       5000    /* Sincronizar con Firebase cada 5 s */
#define SENSOR_READ_MS      3000    /* Leer DHT22 cada 3 s */
#define INTERNET_CHECK_MS   10000   /* Verificar Internet cada 10 s */
#define NTP_SYNC_WAIT_MS    5000    /* Espera inicial NTP */

/* ---------- NTP ---------- */
#define NTP_SERVER          "pool.ntp.org"
#define TIMEZONE            "COT-5"  /* Colombia Time UTC-5 */
