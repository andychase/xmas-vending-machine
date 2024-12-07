#include "apa102.h"

// Driver includes
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"

// FreeRTOS includes
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define LEDS_COUNT 15

static const char *TAG = "APA102_DRIVER";

// Start Frame: 32 bits of 0
// Each LED has 32 bits of data
// SK9822 reset frame
// End Frame: at least leds_count/2 bits of 0
#define BUFFER_SIZE ((LEDS_COUNT + 2) * 4 + (LEDS_COUNT / 16) + 1)

// Place data into DRAM. Constant data gets placed into DROM by default, which
// is not accessible by DMA. This array contains two buffers.
DRAM_ATTR static uint8_t buffer[BUFFER_SIZE];

// Transaction descriptors. Declared static so they're not allocated on the
// stack; we need this memory even when this function is finished because the
// SPI driver needs access to it even while we're already calculating the next
// line.
static spi_transaction_t trans_buf = {
    .flags = 0,
    .cmd = 0,
    .addr = 0,
    .length = 8 * BUFFER_SIZE,  // in bits
    .rxlength = 0,
    .tx_buffer = &buffer,
    .rx_buffer = NULL,
};

static spi_device_handle_t spi;

void apa102_init(apa102_spi_device_t *device) {
    // SPI device configuration structs
    spi_bus_config_t buscfg = {
        .mosi_io_num = device->mosi,     // Master Out Slave In pin
        .miso_io_num = -1,              // Master In Slave Out pin (not used)
        .sclk_io_num = device->clk,     // SPI Clock pin
        .quadwp_io_num = -1,             // WP pin (not used)
        .quadhd_io_num = -1,            // HD pin (not used)
        .max_transfer_sz = BUFFER_SIZE, // Maximum transfer size
        .flags = 0,                      // No specific flags
        .isr_cpu_id = (intr_cpu_id_t) 0,  // Default ISR CPU
        .intr_flags = 0                 // Default interrupt flags
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,               // Number of bits in the command phase
        .address_bits = 0,               // Number of bits in the address phase
        .dummy_bits = 0,                 // Number of dummy bits
        .mode = 0,                       // SPI mode
        .clock_source = SPI_CLK_SRC_DEFAULT, // Default clock source
        .duty_cycle_pos = 128,           // 50% duty cycle
        .cs_ena_pretrans = 0,            // CS activation before transmission
        .cs_ena_posttrans = 0,           // CS activation after transmission
        .clock_speed_hz = device->clock_speed_hz, // Clock speed in Hz
        .input_delay_ns = 0,             // Input delay in nanoseconds
        .spics_io_num = -1,              // CS GPIO pin
        .flags = 0,                      // Flags
        .queue_size = 10,                // Transaction queue size
        .pre_cb = NULL,                  // Pre-transmission callback
        .post_cb = NULL                  // Post-transmission callback
    };

    esp_err_t ret;

    ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "SPI Bus initialized!\n");

    // Preamble: 32 bits of 0
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;

    // SK9822 reset frame + end frame:
    for (int k = 4 * LEDS_COUNT + 4; k < 4 * (LEDS_COUNT + 2); k++) {
        buffer[k] = 0xF;
    }
    // 32 bits of 0 + leds_count/2 bits of 0
    for (int k = 4 * (LEDS_COUNT + 1); k < BUFFER_SIZE; k++) {
        buffer[k] = 0;
    }
}

void apa102_set_pixel(int index, int brightness, int red, int green, int blue) {
    int bytes_offset = (1 + index) * 4;

    buffer[bytes_offset] = 0xE0 | brightness;
    buffer[bytes_offset + 1] = blue;
    buffer[bytes_offset + 2] = green;
    buffer[bytes_offset + 3] = red;
}

void apa102_flush() {
    static int first_flush = 1;
    esp_err_t ret;
    spi_transaction_t *trans;
    // Wait for last transmission to be successful before swapping
    // buffers. Exception for first world transmission
    if (first_flush == 0) {
        ret = spi_device_get_trans_result(spi, &trans, portMAX_DELAY);
        assert(ret == ESP_OK);
    }
    first_flush = 0;
    ret = spi_device_queue_trans(spi, &trans_buf, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
}
