#include "sensors.h"
#include "dht22.h"
#include "app_config.h"

#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "sensors";

/* Estado global protegido por mutex */
static system_state_t  s_state  = {0};
static SemaphoreHandle_t s_mutex = NULL;

/* ================================================================
 *  ISR — Sensor IR (contador de personas)
 *  Flanco de bajada = rayo IR interrumpido → persona ingresó
 * ================================================================ */
static void IRAM_ATTR ir_isr_handler(void *arg)
{
    /* Nota: no usar ESP_LOG dentro de ISR; usar BaseType_t si se necesita
       yield, pero aquí sólo incrementamos un entero atómico. */
    s_state.people_inside++;
}

/* ================================================================
 *  ISR — Evento NFC
 *  El módulo NFC pulsa LOW este GPIO cuando concede acceso.
 * ================================================================ */
static void IRAM_ATTR nfc_isr_handler(void *arg)
{
    s_state.nfc_total++;
    /* El timestamp se actualiza en la tarea principal (no en ISR) */
}

/* ================================================================
 *  Verificación de Internet (HTTP GET → código 204)
 * ================================================================ */
static bool check_internet(void)
{
    esp_http_client_config_t cfg = {
        .url              = PING_URL,
        .timeout_ms       = PING_TIMEOUT_MS,
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;

    esp_err_t err    = esp_http_client_perform(client);
    int       status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    /* Google responde 204 No Content cuando hay internet */
    return (err == ESP_OK && status == 204);
}

/* ================================================================
 *  sensors_init
 * ================================================================ */
esp_err_t sensors_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return ESP_ERR_NO_MEM;

    /* DHT22 */
    dht22_init(DHT22_GPIO);

    /* GPIO sensor IR — interrupción en flanco de bajada */
    gpio_config_t ir_cfg = {
        .pin_bit_mask = (1ULL << PEOPLE_IR_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&ir_cfg);

    /* GPIO evento NFC — interrupción en flanco de bajada */
    gpio_config_t nfc_cfg = {
        .pin_bit_mask = (1ULL << NFC_EVENT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&nfc_cfg);

    /* Instalar servicio de interrupciones GPIO */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PEOPLE_IR_GPIO, ir_isr_handler, NULL);
    gpio_isr_handler_add(NFC_EVENT_GPIO, nfc_isr_handler,  NULL);

    ESP_LOGI(TAG, "Sensores inicializados OK");
    return ESP_OK;
}

/* ================================================================
 *  sensors_task  — tarea periódica
 * ================================================================ */
void sensors_task(void *pvParam)
{
    TickType_t last_sensor   = xTaskGetTickCount();
    TickType_t last_internet = xTaskGetTickCount();

    for (;;) {
        TickType_t now = xTaskGetTickCount();

        /* --- Leer DHT22 --- */
        if ((now - last_sensor) >= pdMS_TO_TICKS(SENSOR_READ_MS)) {
            last_sensor = now;
            float t, h;
            esp_err_t err = dht22_read(&t, &h);

            xSemaphoreTake(s_mutex, portMAX_DELAY);
            if (err == ESP_OK) {
                s_state.temperature = t;
                s_state.humidity    = h;
                s_state.dht_ok      = true;
                ESP_LOGI(TAG, "DHT22 → T=%.1f°C  HR=%.1f%%", t, h);
            } else {
                s_state.dht_ok = false;
                ESP_LOGW(TAG, "DHT22 error: %s", esp_err_to_name(err));
            }

            /* Actualizar timestamp del último evento NFC si hubo cambio */
            time_t now_t = time(NULL);
            struct tm ti;
            localtime_r(&now_t, &ti);
            strftime(s_state.nfc_last_ts, sizeof(s_state.nfc_last_ts),
                     "%Y-%m-%dT%H:%M:%S", &ti);

            xSemaphoreGive(s_mutex);
        }

        /* --- Verificar Internet --- */
        if ((now - last_internet) >= pdMS_TO_TICKS(INTERNET_CHECK_MS)) {
            last_internet = now;
            bool ok = check_internet();

            xSemaphoreTake(s_mutex, portMAX_DELAY);
            s_state.internet_ok = ok;
            xSemaphoreGive(s_mutex);

            ESP_LOGI(TAG, "Internet: %s", ok ? "✓ OK" : "✗ Sin conexión");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ================================================================
 *  sensors_get_state — copia thread-safe del estado
 * ================================================================ */
void sensors_get_state(system_state_t *out)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    memcpy(out, &s_state, sizeof(system_state_t));
    xSemaphoreGive(s_mutex);
}
