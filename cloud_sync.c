#include "cloud_sync.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "app_config.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "cloud_sync";

/* Certificado raíz para HTTPS con Firebase (Google Trust Services) */
/* Incluir si se usa esp_http_client con HTTPS y verificación de certificado.
   Para prototipo de laboratorio se puede omitir con skip_cert_common_name_check. */

/* ================================================================
 *  build_json — construye el payload JSON a enviar
 * ================================================================ */
static int build_json(char *buf, size_t buflen,
                      const system_state_t *st, uint8_t ap_clients)
{
    time_t now = time(NULL);
    return snprintf(buf, buflen,
        "{"
        "\"internet\":%s,"
        "\"connected_ap\":%u,"
        "\"temperature\":%.1f,"
        "\"humidity\":%.1f,"
        "\"people_inside\":%lu,"
        "\"nfc_total\":%lu,"
        "\"nfc_last_access\":\"%s\","
        "\"dht_ok\":%s,"
        "\"timestamp\":%lld"
        "}",
        st->internet_ok ? "true" : "false",
        ap_clients,
        st->temperature,
        st->humidity,
        (unsigned long)st->people_inside,
        (unsigned long)st->nfc_total,
        st->nfc_last_ts,
        st->dht_ok ? "true" : "false",
        (long long)now
    );
}

/* ================================================================
 *  cloud_sync_task
 * ================================================================ */
void cloud_sync_task(void *pvParam)
{
    char json_buf[512];

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(CLOUD_SYNC_MS));

        /* Solo sincronizar si hay STA con Internet */
        if (!wifi_manager_is_sta_connected()) {
            ESP_LOGW(TAG, "Sin STA — omitiendo sync.");
            continue;
        }

        /* Obtener estado actual */
        system_state_t st;
        sensors_get_state(&st);
        uint8_t ap_clients = wifi_manager_get_connected_count();

        int json_len = build_json(json_buf, sizeof(json_buf), &st, ap_clients);
        if (json_len <= 0) {
            ESP_LOGE(TAG, "Error construyendo JSON");
            continue;
        }

        ESP_LOGI(TAG, "Enviando a Firebase: %s", json_buf);

        /* Configurar cliente HTTP */
        esp_http_client_config_t cfg = {
            .url                       = FIREBASE_URL,
            .method                    = HTTP_METHOD_PUT,
            .timeout_ms                = 5000,
            .skip_cert_common_name_check = true,  /* Para prototipo; en producción usar cert */
        };

        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (!client) {
            ESP_LOGE(TAG, "No se pudo crear cliente HTTP");
            continue;
        }

        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, json_buf, json_len);

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "Firebase → HTTP %d", status);
        } else {
            ESP_LOGE(TAG, "Error HTTP: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
    }
}
