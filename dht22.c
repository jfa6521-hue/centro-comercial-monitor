#include "dht22.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG  = "dht22";
static gpio_num_t s_gpio = GPIO_NUM_NC;

/* ---- Helpers de espera con timeout ---- */
#define WAIT_LEVEL(lvl, us_max, label)                      \
    do {                                                     \
        int _t = (us_max);                                   \
        while (gpio_get_level(s_gpio) != (lvl) && --_t)     \
            esp_rom_delay_us(1);                             \
        if (!_t) { ESP_LOGW(TAG, "Timeout en " #label);     \
                   return ESP_ERR_TIMEOUT; }                 \
    } while (0)

/* ================================================================
 *  dht22_init
 * ================================================================ */
void dht22_init(gpio_num_t gpio)
{
    s_gpio = gpio;
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,  /* open-drain */
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(1000)); /* 1 s de estabilización al arrancar */
}

/* ================================================================
 *  dht22_read
 *  ⚠️  Deshabilita interrupciones durante la lectura (≈ 5 ms)
 *     para no perder flancos del protocolo.
 * ================================================================ */
esp_err_t dht22_read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    /* --- Señal de inicio (HOST → DHT22) --- */
    gpio_set_direction(s_gpio, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(s_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(2));   /* LOW ≥ 1 ms */
    gpio_set_level(s_gpio, 1);

    /* Zona crítica — sin interrupciones */
    portDISABLE_INTERRUPTS();

    esp_rom_delay_us(30);
    gpio_set_direction(s_gpio, GPIO_MODE_INPUT);

    /* --- Respuesta del DHT22 --- */
    WAIT_LEVEL(0, 100, "resp-low");
    WAIT_LEVEL(1, 100, "resp-high");
    WAIT_LEVEL(0, 100, "resp-end");

    /* --- Lectura de 40 bits --- */
    for (int i = 0; i < 40; i++) {
        WAIT_LEVEL(1, 100, "bit-start");    /* Flanco de subida = inicio de bit */
        esp_rom_delay_us(40);               /* 26-28 µs → '0' ; >40 µs → '1' */
        data[i / 8] <<= 1;
        if (gpio_get_level(s_gpio)) {
            data[i / 8] |= 1;
        }
        WAIT_LEVEL(0, 100, "bit-end");      /* Flanco de bajada = fin de bit */
    }

    portENABLE_INTERRUPTS();

    /* --- Verificación CRC (byte 5 = suma de los 4 anteriores) --- */
    uint8_t crc = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (crc != data[4]) {
        ESP_LOGW(TAG, "CRC error: calc=0x%02X recv=0x%02X", crc, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    /* --- Decodificación --- */
    *humidity    = ((data[0] << 8) | data[1]) / 10.0f;
    *temperature = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
    if (data[2] & 0x80) *temperature = -*temperature;  /* bit de signo */

    ESP_LOGD(TAG, "T=%.1f°C  HR=%.1f%%", *temperature, *humidity);
    return ESP_OK;
}
