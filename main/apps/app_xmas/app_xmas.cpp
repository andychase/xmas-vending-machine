#include "app_xmas.h"
#include "../common_define.h"
#include "sdkconfig.h"

using namespace MOONCAKE::USER_APP;

#define SPI_CLOCK_SPEED_HZ 1000000 // SPI Clock speed (1 MHz)

#ifdef CONFIG_USING_SIMULATOR
#define XMAS_SPI_HOST SPI3_HOST
#define LED_COUNT 576 // Number of LEDs in the strip
#define CLOCK_PIN 9   // GPIO for clock input (CLK)
#define DATA_PIN 8    // GPIO for data input (MOSI)
#else
#define XMAS_SPI_HOST SPI2_HOST
#define LED_COUNT 72 // Number of LEDs in the strip
#define CLOCK_PIN 1   // GPIO for clock input (CLK)
#define DATA_PIN 2    // GPIO for data input (MOSI)
#endif

#define SECTIONS 2

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

void Xmas::playSong(int songId) {
    int duration;
    int pauseDuration;
    int tone;
    for (int thisNote = 0; thisNote < sizeof(XMAS::songs[songId][0]) / sizeof(int); thisNote++) {
        tone = XMAS::songs[songId][0][thisNote];
        duration = XMAS::songs[songId][1][thisNote] * XMAS::songConstants[songId][0] * 0.1;
        pauseDuration = (XMAS::songs[songId][1][thisNote] * XMAS::songConstants[songId][1] * 0.1) - duration;
        if (duration != 0) {
            _data.hal->buzz.tone(tone, duration, pauseDuration);
        }
    }
    _data.hal->buzz.noTone();
}

void Xmas::onSetup() {
    setAppName("Xmas");
    setAllowBgRunning(false);

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

    // Initialize LED strip
    esp_err_t ret;

    // Step 1: Install the driver
    ret = led_strip_spi_install();
    if (ret != ESP_OK) {
        printf("Failed to install SPI LED strip driver. Error: %d\n", ret);
        return;
    }

    // Step 2: Create and initialize the strip descriptor
    led_strip = LED_STRIP_SPI_DEFAULT_ESP32();
    led_strip.length = LED_COUNT;  // Set the number of LEDs
    led_strip.mosi_io_num = DATA_PIN;   // Set the Data pin (DI)
    led_strip.sclk_io_num = CLOCK_PIN; // Set the Clock pin (CI)
    led_strip.max_transfer_sz = LED_STRIP_SPI_BUFFER_SIZE(LED_COUNT);
    led_strip.clock_speed_hz = SPI_CLOCK_SPEED_HZ;
    led_strip.host_device = XMAS_SPI_HOST;

    // Step 3: Initialize the LED strip
    ret = led_strip_spi_init((led_strip_spi_t*)&led_strip);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI LED strip. Error: %d\n", ret);
        return;
    } else {
        printf("LED strip initialized successfully.\n");
    }

    currentSectionAmountTransitioned = 0;
    currentSection = 0;
}

void Xmas::onRunning() {
    int sectionSize = LED_COUNT / SECTIONS;

    if (_data.hal->encoder.wasMoved(true)) {
        if (_data.hal->encoder.getDirection() < 1) {
            currentSection = (currentSection + 1) % SECTIONS;
        } else {
            currentSection = (currentSection - 1 + SECTIONS) % SECTIONS;
        }
        currentSectionAmountTransitioned = 0;
    }

    if (currentSectionAmountTransitioned < sectionSize) {
        currentSectionAmountTransitioned++;
    }

    int currentStart = (currentSection * sectionSize) % LED_COUNT;
    int nextStart = ((currentSection + 1) * sectionSize) % LED_COUNT;

    for (int i = 0; i < LED_COUNT; i++) {
        led_strip_spi_set_pixel(&led_strip, i, {0, 0, 0});
    }

    for (int i = 0; i < sectionSize; i++) {
        int ledIndex = (currentStart + i) % LED_COUNT;
        float blendFactor = 1.0f - (float)currentSectionAmountTransitioned / sectionSize;
        led_strip_spi_set_pixel(&led_strip, ledIndex, {
            (uint8_t)(255 * blendFactor),
            (uint8_t)(255 * blendFactor),
            (uint8_t)(255 * blendFactor)
        });
    }

    for (int i = 0; i < sectionSize; i++) {
        int ledIndex = (nextStart + i) % LED_COUNT;
        float blendFactor = (float)currentSectionAmountTransitioned / sectionSize;
        led_strip_spi_set_pixel(&led_strip, ledIndex, {
            (uint8_t)(255 * blendFactor),
            (uint8_t)(255 * blendFactor),
            (uint8_t)(255 * blendFactor)
        });
    }

    led_strip_spi_flush(&led_strip);

    delay(1);

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read())
            delay(5);
        playSong(currentSong++);
        if (currentSong >= 7)
            currentSong = 0;
    }
}

void Xmas::onDestroy() {
    _log("onDestroy");
    led_strip_spi_free(&led_strip);
}