#include "st7796s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* https://www.lcdwiki.com/4.0inch_SPI_Module_ST7796 */

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting posture monitor...");

    st7796s_t lcd = {
        .dc_pin  = LCD_DC,
        .rst_pin = LCD_RST,
        .bl_pin  = LCD_BL
    };

    esp_err_t ret = st7796s_init(&lcd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "LCD initialization failed");
        return;
    }

    ESP_LOGI(TAG, "LCD initialized");

    /* Test: black screen */

    st7796s_fill_screen(
        &lcd,
        ST7796S_BLACK
    );

    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );


    /* Red rectangle */

    st7796s_fill_rect(
        &lcd,
        50,
        50,
        150,
        100,
        ST7796S_RED
    );


    /* Green rectangle */

    st7796s_fill_rect(
        &lcd,
        250,
        50,
        150,
        100,
        ST7796S_GREEN
    );


    /* Blue rectangle */

    st7796s_fill_rect(
        &lcd,
        50,
        200,
        150,
        80,
        ST7796S_BLUE
    );


    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}
