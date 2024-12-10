#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"

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

    // Define the GPIO compatibility device descriptor
    compat_gpio_dev_t gpio_device;

    ret = gpio_compat_init(&gpio_device, 0x20, I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    if (ret != ESP_OK) {
        printf("Failed to initialize GPIO device descriptor: %s\n", esp_err_to_name(ret));
        return;
    }

    // Set all GPIOs to output initially (write all zeros)
    ret = gpio_compat_write(&gpio_device, 0x0000); // Set all pins low
    if (ret != ESP_OK) {
        printf("Failed to write to GPIOs: %s\n", esp_err_to_name(ret));
        gpio_compat_free(&gpio_device); // Free the descriptor before exiting
        return;
    }

    printf("GPIO device initialized and all pins set to LOW.\n");

    uint16_t gpio_state = 0x0000;
    ret = gpio_compat_write(&gpio_device, gpio_state);
    if (ret != ESP_OK) {
        printf("Failed to set P00: %s\n", esp_err_to_name(ret));
        gpio_compat_free(&gpio_device); // Free the descriptor before exiting
        return;
    }

    printf("Successfully set P00 to HIGH.\n");

    // Clean up
    ret = gpio_compat_free(&gpio_device);
    if (ret != ESP_OK) {
        printf("Failed to free GPIO device descriptor: %s\n", esp_err_to_name(ret));
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
