#include "wifi_manager.h"
#include "app_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_manager";

/* Bits del EventGroup */
#define STA_CONNECTED_BIT   BIT0
#define STA_FAIL_BIT        BIT1

static EventGroupHandle_t s_wifi_event_group;
static int                s_retry_num = 0;
static uint8_t            s_connected_count = 0;
static bool               s_sta_connected   = false;
static char               s_ap_ip[16]       = "192.168.4.1";

/* ---- Handler de eventos WiFi ---- */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {

        /* AP: cliente conectado */
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *e = event_data;
            s_connected_count++;
            ESP_LOGI(TAG, "Cliente conectado al AP — MAC: " MACSTR " — Total: %d",
                     MAC2STR(e->mac), s_connected_count);
            break;
        }

        /* AP: cliente desconectado */
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *e = event_data;
            if (s_connected_count > 0) s_connected_count--;
            ESP_LOGI(TAG, "Cliente desconectado — MAC: " MACSTR " — Total: %d",
                     MAC2STR(e->mac), s_connected_count);
            break;
        }

        /* STA: iniciando conexión */
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(TAG, "STA iniciando conexión a '%s'…", STA_SSID);
            break;

        /* STA: desconectado — reintenta */
        case WIFI_EVENT_STA_DISCONNECTED:
            s_sta_connected = false;
            if (s_retry_num < STA_MAX_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGW(TAG, "Reintento STA %d/%d", s_retry_num, STA_MAX_RETRY);
            } else {
                xEventGroupSetBits(s_wifi_event_group, STA_FAIL_BIT);
                ESP_LOGE(TAG, "No se pudo conectar a la red externa.");
            }
            break;

        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&e->ip_info.ip));
        s_retry_num    = 0;
        s_sta_connected = true;
        xEventGroupSetBits(s_wifi_event_group, STA_CONNECTED_BIT);
    }
}

/* ================================================================
 *  wifi_manager_init
 * ================================================================ */
esp_err_t wifi_manager_init(void)
{
    /* NVS (requerido por WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Crear interfaces de red */
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    /* Inicializar WiFi con config por defecto */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Registrar handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    /* ---- Configuración AP ---- */
    wifi_config_t ap_config = {
        .ap = {
            .ssid            = AP_SSID,
            .ssid_len        = strlen(AP_SSID),
            .channel         = AP_CHANNEL,
            .password        = AP_PASS,
            .max_connection  = AP_MAX_CONNECTIONS,
            .authmode        = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg         = { .required = false },
        },
    };

    /* ---- Configuración STA ---- */
    wifi_config_t sta_config = {
        .sta = {
            .ssid     = STA_SSID,
            .password = STA_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    /* Modo APSTA */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,  &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP iniciado — SSID: '%s'  Pass: '%s'  Canal: %d  Max: %d",
             AP_SSID, AP_PASS, AP_CHANNEL, AP_MAX_CONNECTIONS);

    /* Esperar hasta conectar STA o agotar reintentos */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        STA_CONNECTED_BIT | STA_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(30000));   /* espera máx. 30 s */

    if (bits & STA_CONNECTED_BIT) {
        ESP_LOGI(TAG, "STA conectado a '%s' con acceso a Internet.", STA_SSID);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "STA sin conexión — operando sólo en modo AP.");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
}

uint8_t wifi_manager_get_connected_count(void) {
    return s_connected_count;
}

bool wifi_manager_is_sta_connected(void) {
    return s_sta_connected;
}

const char *wifi_manager_get_ap_ip(void) {
    return s_ap_ip;
}
