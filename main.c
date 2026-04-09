#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#include "app_config.h"
#include "wifi_manager.h"
#include "sensors.h"
#include "cloud_sync.h"

static const char *TAG = "app_main";

/* ================================================================
 *  Sincronización NTP — obtener hora real para timestamps
 * ================================================================ */
static void sync_time(void)
{
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
    esp_netif_sntp_init(&cfg);

    /* Esperar a que se sincronice */
    int retry = 0;
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(NTP_SYNC_WAIT_MS))
           != ESP_OK && retry++ < 5) {
        ESP_LOGW(TAG, "Esperando NTP… (%d/5)", retry);
    }

    /* Configurar zona horaria Colombia UTC-5 */
    setenv("TZ", TIMEZONE, 1);
    tzset();

    time_t now;
    time(&now);
    struct tm ti;
    localtime_r(&now, &ti);
    ESP_LOGI(TAG, "Hora sincronizada: %04d-%02d-%02d %02d:%02d:%02d",
             ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
             ti.tm_hour, ti.tm_min, ti.tm_sec);
}

/* ================================================================
 *  app_main
 * ================================================================ */
void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " Centro Comercial — Sistema de Monitoreo");
    ESP_LOGI(TAG, " Campos y Ondas 2026-1");
    ESP_LOGI(TAG, "==============================================");

    /* 1. WiFi AP+STA */
    esp_err_t wifi_ret = wifi_manager_init();
    if (wifi_ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi OK — sincronizando hora con NTP…");
        sync_time();
    } else {
        ESP_LOGW(TAG, "WiFi STA no disponible — operando sin Internet.");
    }

    /* 2. Sensores + GPIOs */
    ESP_ERROR_CHECK(sensors_init());

    /* 3. Tarea de lectura de sensores e Internet
     *    Stack 4096 bytes, prioridad 5 */
    xTaskCreate(sensors_task,    "sensors",    4096, NULL, 5, NULL);

    /* 4. Tarea de sincronización con Firebase
     *    Stack 6144 bytes (más porque genera JSON y hace HTTP),
     *    prioridad 4 (menor que sensores) */
    xTaskCreate(cloud_sync_task, "cloud_sync", 6144, NULL, 4, NULL);

    ESP_LOGI(TAG, "Sistema iniciado.");
    ESP_LOGI(TAG, "  AP SSID : %s", AP_SSID);
    ESP_LOGI(TAG, "  AP IP   : %s", wifi_manager_get_ap_ip());
    ESP_LOGI(TAG, "  Firebase: %s", FIREBASE_HOST);

    /* app_main puede terminar — las tareas FreeRTOS siguen corriendo */
}
