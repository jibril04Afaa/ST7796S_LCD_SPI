#pragma once

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

/* ---------- LCD dimensions ---------- */

#define ST7796S_WIDTH   480
#define ST7796S_HEIGHT  320


#define LCD_MOSI  23
#define LCD_SCLK  18
#define LCD_CS    5
#define LCD_DC    21
#define LCD_RST   22
#define LCD_BL    4

/* ---------- ST7796S commands ---------- */

#define ST7796S_SWRESET     0x01
#define ST7796S_SLPOUT      0x11
#define ST7796S_INVOFF      0x20
#define ST7796S_INVON       0x21
#define ST7796S_DISPOFF     0x28
#define ST7796S_DISPON      0x29
#define ST7796S_CASET       0x2A
#define ST7796S_PASET       0x2B
#define ST7796S_RAMWR       0x2C
#define ST7796S_MADCTL      0x36
#define ST7796S_COLMOD      0x3A

/* RGB565 colors */

#define ST7796S_BLACK   0x0000
#define ST7796S_WHITE   0xFFFF
#define ST7796S_RED     0xF800
#define ST7796S_GREEN   0x07E0
#define ST7796S_BLUE    0x001F
#define ST7796S_YELLOW  0xFFE0

typedef struct
{
    spi_device_handle_t spi;

    gpio_num_t dc_pin;
    gpio_num_t rst_pin;
    gpio_num_t bl_pin;

} st7796s_t;


/* Initialize SPI + GPIO + LCD */
esp_err_t st7796s_init(st7796s_t *lcd);

/* Turn backlight on/off */
void st7796s_backlight(st7796s_t *lcd, bool on);

/* Fill entire display with one color */
esp_err_t st7796s_fill_screen(st7796s_t *lcd, uint16_t color);

/* Draw one pixel */
esp_err_t st7796s_draw_pixel(
    st7796s_t *lcd,
    uint16_t x,
    uint16_t y,
    uint16_t color
);

/* Draw a filled rectangle */
esp_err_t st7796s_fill_rect(
    st7796s_t *lcd,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint16_t color
);
