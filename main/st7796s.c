#include "st7796s.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include <string.h>



/*
 * ESP32 -> LCD
 *
 * GPIO23 -> MOSI
 * GPIO18 -> SCLK
 * GPIO5  -> CS
 * GPIO21 -> DC
 * GPIO22 -> RST
 * GPIO4  -> BL
 */


/* -------------------------------------------------------
 * Low-level SPI write
 * ------------------------------------------------------- */

static esp_err_t st7796s_spi_write(
    st7796s_t *lcd,
    const uint8_t *data,
    size_t length
)
{
    spi_transaction_t transaction = {
        .flags = 0,
        .length = length * 8,
        .tx_buffer = data,
        .rx_buffer = NULL
    };

    return spi_device_transmit(lcd->spi, &transaction);
}


/* -------------------------------------------------------
 * Send command
 * ------------------------------------------------------- */

static esp_err_t st7796s_write_command(
    st7796s_t *lcd,
    uint8_t command
)
{
    gpio_set_level(lcd->dc_pin, 0);

    return st7796s_spi_write(
        lcd,
        &command,
        1
    );
}


/* -------------------------------------------------------
 * Send data
 * ------------------------------------------------------- */

static esp_err_t st7796s_write_data(
    st7796s_t *lcd,
    const uint8_t *data,
    size_t length
)
{
    gpio_set_level(lcd->dc_pin, 1);

    return st7796s_spi_write(
        lcd,
        data,
        length
    );
}


/* -------------------------------------------------------
 * Hardware reset
 * ------------------------------------------------------- */

static void st7796s_reset(st7796s_t *lcd)
{
    gpio_set_level(lcd->rst_pin, 0);

    esp_rom_delay_us(100000);

    gpio_set_level(lcd->rst_pin, 1);

    esp_rom_delay_us(150000);
}


/* -------------------------------------------------------
 * Set drawing window
 *
 * Tells the LCD:
 *
 * "I want to write pixels from
 *  (x1,y1) to (x2,y2)"
 * ------------------------------------------------------- */

static esp_err_t st7796s_set_window(
    st7796s_t *lcd,
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2
)
{
    uint8_t data[4];

    /* Column address */

    st7796s_write_command(
        lcd,
        ST7796S_CASET
    );

    data[0] = x1 >> 8;
    data[1] = x1 & 0xFF;
    data[2] = x2 >> 8;
    data[3] = x2 & 0xFF;

    st7796s_write_data(
        lcd,
        data,
        4
    );


    /* Page/row address */

    st7796s_write_command(
        lcd,
        ST7796S_PASET
    );

    data[0] = y1 >> 8;
    data[1] = y1 & 0xFF;
    data[2] = y2 >> 8;
    data[3] = y2 & 0xFF;

    st7796s_write_data(
        lcd,
        data,
        4
    );


    /* Start writing pixels */

    return st7796s_write_command(
        lcd,
        ST7796S_RAMWR
    );
}


/* -------------------------------------------------------
 * LCD initialization
 * ------------------------------------------------------- */

static esp_err_t st7796s_panel_init(
    st7796s_t *lcd
)
{
    esp_err_t ret;

    /* Software reset */

    ret = st7796s_write_command(
        lcd,
        ST7796S_SWRESET
    );

    if (ret != ESP_OK)
        return ret;

    esp_rom_delay_us(150000);


    /* Exit sleep mode */

    ret = st7796s_write_command(
        lcd,
        ST7796S_SLPOUT
    );

    if (ret != ESP_OK)
        return ret;

    esp_rom_delay_us(120000);


    /*
     * Pixel format:
     *
     * 0x55 = 16-bit/pixel RGB565
     */

    ret = st7796s_write_command(
        lcd,
        ST7796S_COLMOD
    );

    if (ret != ESP_OK)
        return ret;

    uint8_t pixel_format = 0x55;

    ret = st7796s_write_data(
        lcd,
        &pixel_format,
        1
    );

    if (ret != ESP_OK)
        return ret;


    /*
     * Memory access control
     *
     * 0x48 is a common landscape configuration.
     *
     * You may need to change this depending
     * on how your particular module is mounted.
     */

    ret = st7796s_write_command(
        lcd,
        ST7796S_MADCTL
    );

    if (ret != ESP_OK)
        return ret;

    uint8_t madctl = 0x48;

    ret = st7796s_write_data(
        lcd,
        &madctl,
        1
    );

    if (ret != ESP_OK)
        return ret;


    /* Display inversion */

    ret = st7796s_write_command(
        lcd,
        ST7796S_INVON
    );

    if (ret != ESP_OK)
        return ret;


    /* Turn display on */

    ret = st7796s_write_command(
        lcd,
        ST7796S_DISPON
    );

    if (ret != ESP_OK)
        return ret;

    esp_rom_delay_us(100000);

    return ESP_OK;
}


/* -------------------------------------------------------
 * Public initialization function
 * ------------------------------------------------------- */

esp_err_t st7796s_init(st7796s_t *lcd)
{
    esp_err_t ret;


    /* ---------- Configure GPIO ---------- */

    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << lcd->dc_pin) |
            (1ULL << lcd->rst_pin) |
            (1ULL << lcd->bl_pin),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ret = gpio_config(&io_conf);

    if (ret != ESP_OK)
        return ret;


    /* ---------- Configure SPI bus ---------- */

    spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCLK,

        .quadwp_io_num = -1,
        .quadhd_io_num = -1,

        .max_transfer_sz =
            ST7796S_WIDTH * 40 * 2
    };


    ret = spi_bus_initialize(
        SPI2_HOST,
        &bus_config,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK)
        return ret;


    /* ---------- Configure LCD as SPI device ---------- */

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 40 * 1000 * 1000,

        .mode = 0,

        .spics_io_num = LCD_CS,

        .queue_size = 7,

        .flags = SPI_DEVICE_HALFDUPLEX
    };


    ret = spi_bus_add_device(
        SPI2_HOST,
        &device_config,
        &lcd->spi
    );

    if (ret != ESP_OK)
        return ret;


    /* ---------- Reset LCD ---------- */

    st7796s_reset(lcd);


    /* ---------- Initialize LCD controller ---------- */

    ret = st7796s_panel_init(lcd);

    if (ret != ESP_OK)
        return ret;


    /* ---------- Enable backlight ---------- */

    st7796s_backlight(
        lcd,
        true
    );


    return ESP_OK;
}


/* -------------------------------------------------------
 * Backlight
 * ------------------------------------------------------- */

void st7796s_backlight(
    st7796s_t *lcd,
    bool on
)
{
    gpio_set_level(
        lcd->bl_pin,
        on ? 1 : 0
    );
}


/* -------------------------------------------------------
 * Fill rectangle
 * ------------------------------------------------------- */

esp_err_t st7796s_fill_rect(
    st7796s_t *lcd,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint16_t color
)
{
    if (x >= ST7796S_WIDTH ||
        y >= ST7796S_HEIGHT)
    {
        return ESP_ERR_INVALID_ARG;
    }


    uint16_t x2 = x + width - 1;
    uint16_t y2 = y + height - 1;


    if (x2 >= ST7796S_WIDTH)
        x2 = ST7796S_WIDTH - 1;

    if (y2 >= ST7796S_HEIGHT)
        y2 = ST7796S_HEIGHT - 1;


    esp_err_t ret;

    ret = st7796s_set_window(
        lcd,
        x,
        y,
        x2,
        y2
    );

    if (ret != ESP_OK)
        return ret;


    /*
     * RGB565:
     *
     * 16 bits per pixel
     */

    uint8_t pixel[2];

    pixel[0] = color >> 8;
    pixel[1] = color & 0xFF;


    uint32_t pixel_count =
        (x2 - x + 1) *
        (y2 - y + 1);


    /*
     * Send pixels in chunks.
     *
     * Don't allocate the entire 480x320
     * framebuffer just to test the LCD.
     */

    uint8_t buffer[64];

    for (int i = 0; i < 64; i += 2)
    {
        buffer[i] = pixel[0];
        buffer[i + 1] = pixel[1];
    }


    while (pixel_count > 0)
    {
        uint32_t pixels_to_send =
            pixel_count > 32 ? 32 : pixel_count;

        ret = st7796s_write_data(
            lcd,
            buffer,
            pixels_to_send * 2
        );

        if (ret != ESP_OK)
            return ret;

        pixel_count -= pixels_to_send;
    }


    return ESP_OK;
}


/* -------------------------------------------------------
 * Fill entire screen
 * ------------------------------------------------------- */

esp_err_t st7796s_fill_screen(
    st7796s_t *lcd,
    uint16_t color
)
{
    return st7796s_fill_rect(
        lcd,
        0,
        0,
        ST7796S_WIDTH,
        ST7796S_HEIGHT,
        color
    );
}


/* -------------------------------------------------------
 * Draw one pixel
 * ------------------------------------------------------- */

esp_err_t st7796s_draw_pixel(
    st7796s_t *lcd,
    uint16_t x,
    uint16_t y,
    uint16_t color
)
{
    return st7796s_fill_rect(
        lcd,
        x,
        y,
        1,
        1,
        color
    );
}