#include "app_xmas.h"
#include "../common_define.h"
#include "sdkconfig.h"

using namespace MOONCAKE::USER_APP;

#define SPI_CLOCK_SPEED_HZ 1000000 // SPI Clock speed (1 MHz)

// APA102 device configuration
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

#define SECTIONS 4


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
    for (int thisNote = 0; thisNote < sizeof(XMAS::songs[songId][0]) / sizeof(int); thisNote++)
    {   
        tone = XMAS::songs[songId][0][thisNote];
        duration = XMAS::songs[songId][1][thisNote] * XMAS::songConstants[songId][0] * 0.1;
        pauseDuration = (XMAS::songs[songId][1][thisNote] * XMAS::songConstants[songId][1] * 0.1) - duration;
        // If duration is 0 the tone is played forever
        if (duration != 0) {
            _data.hal->buzz.tone(tone, duration, pauseDuration);    
        }
    }
    _data.hal->buzz.noTone();
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

    // // Initialize the APA102 device
    led_strip_device.clock_speed_hz = SPI_CLOCK_SPEED_HZ;
    led_strip_device.mosi = DATA_PIN;
    led_strip_device.clk = CLOCK_PIN;
    led_strip_device.cs = -1; // Not used for APA102

    apa102_init(&led_strip_device, XMAS_SPI_HOST);

    printf("LEDs initialized and set to white successfully.\n");
    currentSectionAmountTransitioned = 0;
    currentSection = 0;
}

void Xmas::onRunning() {
    // Determine section size
    int sectionSize = LED_COUNT / SECTIONS;

    // Check encoder movement to update the target section
    if (_data.hal->encoder.wasMoved(true)) {
        if (_data.hal->encoder.getDirection() < 1) {
            currentSection = (currentSection + 1) % SECTIONS;
        } else {
            currentSection = (currentSection - 1 + SECTIONS) % SECTIONS;
        }
        currentSectionAmountTransitioned = 0; // Reset transition progress
    }

    // Smoothly transition between sections
    if (currentSectionAmountTransitioned < sectionSize) {
        currentSectionAmountTransitioned++;
    }

    // Calculate starting LED indices for the current and next sections
    int currentStart = (currentSection * sectionSize) % LED_COUNT;
    int nextStart = ((currentSection + 1) * sectionSize) % LED_COUNT;

    // Clear all LEDs
    for (int i = 0; i < LED_COUNT; i++) {
        apa102_set_pixel(i, 0, 0, 0, 0); // Turn off
    }

    // Light up the current section with decreasing brightness
    for (int i = 0; i < sectionSize; i++) {
        int ledIndex = (currentStart + i) % LED_COUNT;
        float blendFactor = 1.0f - (float)currentSectionAmountTransitioned / sectionSize;
        apa102_set_pixel(ledIndex, 31 * blendFactor, 255 * blendFactor, 255 * blendFactor, 255 * blendFactor);
    }

    // Light up the next section with increasing brightness
    for (int i = 0; i < sectionSize; i++) {
        int ledIndex = (nextStart + i) % LED_COUNT;
        float blendFactor = (float)currentSectionAmountTransitioned / sectionSize;
        apa102_set_pixel(ledIndex, 31 * blendFactor, 255 * blendFactor, 255 * blendFactor, 255 * blendFactor);
    }

    // Flush the buffer to update LEDs
    apa102_flush();

    // Delay to control the animation speed
    delay(1);

    /* If button pressed */
    if (!_data.hal->encoder.btn.read()) {
        /* Hold until button release */
        while (!_data.hal->encoder.btn.read())
            delay(5);
        playSong(currentSong++);
        if (currentSong >= 7)
            currentSong = 0;
        /* Bye */
        //destroyApp();
    }
}

void Xmas::onDestroy() {
    _log("onDestroy");
}
