#include "app_xmas.h"
#include "../common_define.h"
#include <pcf8575.h>
#include "i2cdev.h"

#define SDA_GPIO GPIO_NUM_1
#define SCL_GPIO GPIO_NUM_2

using namespace MOONCAKE::USER_APP;

void Xmas::onSetup() {
    setAppName("Xmas");
    setAllowBgRunning(false);

    /* Copy default value */
    XMAS::Data_t default_data;
    _data = default_data;

    _data.hal = (HAL::HAL *)getUserData();
}

/* Life cycle */
void Xmas::onCreate() {
    _log("onCreate");

    LGFX_Sprite *canvas = _data.hal->canvas;
    canvas->clear();
    canvas->setTextSize(1.5);
    canvas->setTextColor((uint32_t)0xF3E9D2);
    canvas->setFont(&fonts::efontCN_24);
    canvas->drawCenterString("XMAS", _data.hal->display.width() / 2, _data.hal->display.height() / 2 - 24);
    canvas->pushSprite(0, 0);
    canvas->setTextSize(1);

    // Initialize the I2C driver
    esp_err_t ret = i2cdev_init();
    if (ret != ESP_OK) {
        printf("Failed to initialize I2C driver: %s\n", esp_err_to_name(ret));
        return;
    }

    // Define the PCF8575 device descriptor
    i2c_dev_t pcf_device = {
        .port = I2C_NUM_1,
        .cfg = {
            .mode = I2C_MODE_MASTER,            // I2C master mode
            .sda_io_num = SDA_GPIO,            // GPIO for SDA
            .scl_io_num = SCL_GPIO,            // GPIO for SCL
            .sda_pullup_en = GPIO_PULLUP_ENABLE, // Enable SDA pull-up
            .scl_pullup_en = GPIO_PULLUP_ENABLE, // Enable SCL pull-up
            .master = {
                .clk_speed = 400000,           // Set I2C clock frequency to 400 kHz
            },
            .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL // Use normal clock source
        },
        .addr = PCF8575_I2C_ADDR_BASE,        // Unshifted I2C address for PCF8575
        .mutex = NULL,                        // Mutex will be initialized later
        .timeout_ticks = 0                    // Default timeout; uses I2CDEV_MAX_STRETCH_TIME
    };

    ret = pcf8575_init_desc(&pcf_device, PCF8575_I2C_ADDR_BASE, I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    if (ret != ESP_OK) {
        printf("Failed to initialize PCF8575 descriptor: %s\n", esp_err_to_name(ret));
        return;
    }

    // Set all GPIOs to output initially (write all zeros)
    ret = pcf8575_port_write(&pcf_device, 0x0000); // Set all pins low
    if (ret != ESP_OK) {
        printf("Failed to write to PCF8575 GPIOs: %s\n", esp_err_to_name(ret));
        pcf8575_free_desc(&pcf_device); // Free the descriptor before exiting
        return;
    }

    printf("PCF8575 device initialized and all pins set to LOW.\n");

    // Example: Set specific GPIO pin (P00)
    uint16_t gpio_state = 0x0000; // Current GPIO state (all pins low)
    gpio_state |= (1 << 0); // Set bit for P00 (LSB)
    ret = pcf8575_port_write(&pcf_device, gpio_state);
    if (ret != ESP_OK) {
        printf("Failed to set P00: %s\n", esp_err_to_name(ret));
        pcf8575_free_desc(&pcf_device); // Free the descriptor before exiting
        return;
    }

    printf("Successfully set P00 to HIGH.\n");

    // Clean up
    ret = pcf8575_free_desc(&pcf_device);
    if (ret != ESP_OK) {
        printf("Failed to free PCF8575 descriptor: %s\n", esp_err_to_name(ret));
    }

    // Deinitialize the I2C driver
    ret = i2cdev_done();
    if (ret != ESP_OK) {
        printf("Failed to deinitialize I2C driver: %s\n", esp_err_to_name(ret));
    }
}


void Xmas::onRunning() {
    /* If button pressed */
    if (!_data.hal->encoder.btn.read()) {
        /* Hold until button release */
        while (!_data.hal->encoder.btn.read())
            delay(5);

        /* Bye */
        destroyApp();
    }
}

void Xmas::onDestroy() {
    _log("onDestroy");
}
