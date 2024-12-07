#include "app_xmas.h"
#include "../common_define.h"


using namespace MOONCAKE::USER_APP;

#define LED_COUNT 15 // Number of LEDs in the strip
#define CLOCK_PIN 1   // GPIO for clock input (CLK)
#define DATA_PIN 2    // GPIO for data input (MOSI)
#define SPI_CLOCK_SPEED_HZ 1000000 // SPI Clock speed (1 MHz)

// APA102 device configuration


// Function to generate rainbow colors
void color_wheel(uint8_t pos, uint8_t &red, uint8_t &green, uint8_t &blue) {
    pos = 255 - pos;
    if (pos < 85) {
        red = (uint8_t)(255 - pos * 3);
        green = 0;
        blue = (uint8_t)(pos * 3);
    } else if (pos < 170) {
        pos -= 85;
        red = 0;
        green = (uint8_t)(pos * 3);
        blue = (uint8_t)(255 - pos * 3);
    } else {
        pos -= 170;
        red = (uint8_t)(pos * 3);
        green = (uint8_t)(255 - pos * 3);
        blue = 0;
    }
}

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

    // Initialize the APA102 device
    led_strip_device.clock_speed_hz = SPI_CLOCK_SPEED_HZ;
    led_strip_device.mosi = DATA_PIN;
    led_strip_device.clk = CLOCK_PIN;
    led_strip_device.cs = -1; // Not used for APA102

    apa102_init(&led_strip_device);

    // Set all LEDs to white initially
    for (int i = 0; i < LED_COUNT; i++) {
        apa102_set_pixel(i, 31, 255, 255, 255); // Brightness: 31, RGB: White
    }
    apa102_flush();

    printf("LEDs initialized and set to white successfully.\n");
}

void Xmas::onRunning() {
    LGFX_Sprite *canvas = _data.hal->canvas;
    canvas->clear();
    canvas->setTextSize(1.5);
    canvas->setTextColor((uint32_t)0xF3E9D2);
    canvas->setFont(&fonts::efontCN_24);
    canvas->drawCenterString("XMAS", _data.hal->display.width() / 2, _data.hal->display.height() / 2 - 24);
    canvas->pushSprite(0, 0);
    canvas->setTextSize(1);

    // Set each LED to a color from the color wheel
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t red, green, blue;
        color_wheel((hue + (i * 256 / LED_COUNT)) & 255, red, green, blue);
        apa102_set_pixel(i, 31, red, green, blue); // Brightness: 31
    }

    // Flush the buffer to update LEDs
    apa102_flush();

    // Increment hue for the next frame
    hue++;

    // Delay to control the animation speed
    delay(100);

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
