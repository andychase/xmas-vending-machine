#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"
#include <i2cdev.h>
#include <mcp23x17.h>
#include <driver/i2c.h>

#include "utils/xmas_img.h"


#define SDA_GPIO GPIO_NUM_13
#define SCL_GPIO GPIO_NUM_15

#define MCP23017_PIN_LED 8    // GPIO pin connected to the LED
#define MCP23017_PIN_BUTTON 0 // GPIO pin connected to the button

#define SPI_CLOCK_SPEED_HZ 100'000
#ifdef CONFIG_USING_SIMULATOR
#define XMAS_SPI_HOST SPI3_HOST
#define LED_COUNT 15 // Number of LEDs in the strip
#define CLOCK_PIN 9   // GPIO for clock input (CLK)
#define DATA_PIN 8    // GPIO for data input (MOSI)
#else
#define XMAS_SPI_HOST SPI2_HOST
#define LED_COUNT 576 // Number of LEDs in the strip
#define CLOCK_PIN 2   // GPIO for clock input (CLK)
#define DATA_PIN 1    // GPIO for data input (MOSI)
#endif

#define SECTIONS 4
#define SECTION_SIZE (LED_COUNT / SECTIONS)
#define TRANSITION_STEP 1

using namespace MOONCAKE::USER_APP;

static esp_err_t ret;
static mcp23x17_t dev[4];
static const int ACTIVE_PINS[][4] = {{8, 9, 10, 11}, {11, 10, 9, 8}, {11, 10, 9, 8}, {11, 10, 9, 8}};
static const int READ_PINS[][4] = {{3, 4, 5, 6}, {3, 4, 5, 6}, {3, 4, 5, 6}, {3, 4, 5, 6}};

static const int ADDRESSES[] = {0, 1, 2, 3};

static TickType_t lastFlushTick = 0;
const TickType_t flushIntervalMs = 33; // ~30 FPS for LEDs

static TickType_t lastFlushTickD = 0;

static bool s_showLeftFrame = true;
static int s_frameToggleCounter = 0;
#define XMAS_IMG_W 216
#define XMAS_IMG_H 93
#define XMAS_HALF_W (XMAS_IMG_W / 2)

struct PinSelection
{
    int address;
    int pin;
};


PinSelection selectPin(int index, const int (*GROUP)[4] = ACTIVE_PINS)
{
    for (int group = 0; group < sizeof(ACTIVE_PINS) / sizeof(ACTIVE_PINS[0]); group++)
    {
        int groupSize = sizeof(GROUP[group]) / sizeof(GROUP[group][0]);
        if (index < groupSize)
        {
            return {ADDRESSES[group], GROUP[group][index]};
        }
        index -= groupSize;
    }
    return {0, 0};
}

rgb_t color_wheel(uint8_t pos, float gamma) {
    pos = 255 - pos; 
    rgb_t color;

    if (pos < 85) {
        color.red = (uint8_t)(255 - pos * 3);
        color.green = 0;
        color.blue = (uint8_t)(pos * 3);
    } else if (pos < 170) {
        pos -= 85;
        color.red = 0;
        color.green = (uint8_t)(pos * 3);
        color.blue = (uint8_t)(255 - pos * 3);
    } else {
        pos -= 170;
        color.red = (uint8_t)(pos * 3);
        color.green = (uint8_t)(255 - pos * 3);
        color.blue = 0;
    }

    return apply_gamma2rgb(color, gamma);
}

struct LEDSectionStruct {
    int startA;
    int endA;
    int startB;
    int endB;
};


LEDSectionStruct calculateSections(int section, int sectionSize) {
    LEDSectionStruct result;
    result.startA = (section % 4) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
    result.endA = ((section % 4) + 1) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
    result.startB = (sectionSize * 8) - ((section % 4 + 1) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));
    result.endB = (sectionSize * 8) - ((section % 4) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));
    return result;
}

void Xmas::startLights() {
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
}

void Xmas::onSetup()
{
    setAppName("Xmas");
    setAllowBgRunning(false);
    XMAS::Data_t default_data;
    _data = default_data;
    _data.hal = (HAL::HAL*)getUserData();
}

void Xmas::onCreate()
{
    _log("onCreate");
    XMAS::Utils::drawCenterString(_data.hal, "XMAS");
    gpio_compat_i2cScan(I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    
    ret = i2cdev_init();
    if (ret != ESP_OK)
    {
        printf("Failed to initialize I2C driver: %s\n", esp_err_to_name(ret));
    }

    for (int addr = 0x21; addr <= 0x24; addr++)
    {
        compat_gpio_dev_t* _dev = &dev[addr - 0x21];
        gpio_compat_init(_dev, addr, I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    }

    gpio_compat_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
    gpio_compat_set_interrupt(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_INT_LOW_EDGE);
    
    // Make sure other control units have their 0 pin off
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
    {
            gpio_compat_set_mode(&dev[i], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
            gpio_compat_set_pullup(&dev[i], MCP23017_PIN_BUTTON, true);
    }
    for (int i = 0; i < sizeof(ACTIVE_PINS) / sizeof(ACTIVE_PINS[0]); i++)
    {
        for (int j = 0; j < sizeof(ACTIVE_PINS[i]) / sizeof(ACTIVE_PINS[i][0]); j++)
        {
            gpio_compat_set_mode(&dev[i], ACTIVE_PINS[i][j], MCP23X17_GPIO_OUTPUT);
        }
    }
    for (int i = 0; i < sizeof(READ_PINS) / sizeof(READ_PINS[0]); i++)
    {
        for (int j = 0; j < sizeof(READ_PINS[i]) / sizeof(READ_PINS[i][0]); j++)
        {
            gpio_compat_set_mode(&dev[i], READ_PINS[i][j], MCP23X17_GPIO_INPUT);
            gpio_compat_set_pullup(&dev[i], READ_PINS[i][j], true);
        }
    }

    startLights();
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

void Xmas::showAnimation() {
    for (int frames = 30; frames > 0; frames--) {
        LGFX_Sprite* canvas = _data.hal->canvas;
        s_frameToggleCounter++;
        if (s_frameToggleCounter >= 2) {
            s_showLeftFrame = !s_showLeftFrame;
            s_frameToggleCounter = 0;
        }
        int offX = s_showLeftFrame ? 0 : XMAS_HALF_W;
        canvas->clear();
        canvas->fillScreen(0xFFFFFF);
        canvas->drawQoi(
            (const uint8_t*)XMASPIMAGE1, 
            sizeof(XMASPIMAGE1), 
            (_data.hal->display.width() / 2) - (XMAS_HALF_W/2), \
            (_data.hal->display.height() / 2) - (XMAS_IMG_H/2), 
            XMAS_HALF_W, 
            XMAS_IMG_H, 
            offX, 
            0
        );
        canvas->pushSprite(0, 0);
        delay(100);
    }
}

void Xmas::onRunningLights() {
    for (int i = 0; i < LED_COUNT; i++) {
        rgb_t color = color_wheel((hue + (i * 256 / LED_COUNT)) & 255, 1);
        color.b *= 0.5;
        color.g *= 0.5;
        color.r *= 0.5;
        led_strip_spi_set_pixel(&led_strip, i, color);
    }
    hue++;

    rainbowTimeCounter++;
    if (rainbowTimeCounter < 500) {
          for (int i = 0; i < LED_COUNT; i++) {
            led_strip_spi_set_pixel(&led_strip, i, {0, 0, 0});
        }
        LEDSectionStruct ledSectionStruct = calculateSections(currentSelection-1, 18);
        for (size_t i = ledSectionStruct.startA; i <= ledSectionStruct.endA; i++) {
            led_strip_spi_set_pixel(&led_strip, i, {100, 100, 100});
        }
        for (size_t i = ledSectionStruct.startB; i <= ledSectionStruct.endB; i++) {
            led_strip_spi_set_pixel(&led_strip, i, {100, 100, 100});
        }
        // Narrow section a bit by blanking first and last few LEDs
        for (int i = 0; i < 4; i++) {
            if ((ledSectionStruct.startA + i) < LED_COUNT) {
                led_strip_spi_set_pixel(&led_strip, ledSectionStruct.startA + i, {0, 0, 0});
            }
            if ((ledSectionStruct.endA - i) < LED_COUNT) {
                led_strip_spi_set_pixel(&led_strip, ledSectionStruct.endA - i, {0, 0, 0});
            }
            if ((ledSectionStruct.startB + i) < LED_COUNT) {
                led_strip_spi_set_pixel(&led_strip, ledSectionStruct.startB + i, {0, 0, 0});
            }
            if ((ledSectionStruct.endB - i) < LED_COUNT) {
                led_strip_spi_set_pixel(&led_strip, ledSectionStruct.endB - i, {0, 0, 0});
            }
        }
    }

    // Turn off edge LEDs so they don't overhead
    for (int base = 0; base < LED_COUNT; base += 144) {
        if (base < LED_COUNT) {
            led_strip_spi_set_pixel(&led_strip, base, {0, 0, 0});
        }
        if ((base + 71) < LED_COUNT) {
            led_strip_spi_set_pixel(&led_strip, base + 71, {0, 0, 0});
        }
        if ((base + 72) < LED_COUNT) {
            led_strip_spi_set_pixel(&led_strip, base + 72, {0, 0, 0});
        }
        if ((base + 143) < LED_COUNT) {
            led_strip_spi_set_pixel(&led_strip, base + 143, {0, 0, 0});
        }
    }

    // Throttle SPI flush to ~30Hz (adjust ms as needed). Flushing every frame
    // is expensive; we only need to push updates at a lower rate.
    TickType_t nowMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((nowMs - lastFlushTick) >= flushIntervalMs) {
        led_strip_spi_flush(&led_strip);
        lastFlushTick = nowMs;
    }
}

void Xmas::onRunningButtons() {
    uint32_t buttonState = 0;
    TickType_t nowMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((nowMs - lastFlushTick) >= 3) {
        gpio_compat_read(&dev[0], MCP23017_PIN_BUTTON, &buttonState);
        lastFlushTickD = nowMs;
    }
    
    if (buttonState == 0)
    { 
        PinSelection selectedPin = selectPin(currentSelection - 1);
        _log("button pushed, address: %u, pin: %u", selectedPin.address, selectedPin.pin);
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 1);
        delay(250);
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 0);
        showAnimation();
        XMAS::Utils::drawCenterString(_data.hal, std::to_string(currentSelection).c_str());
        // playSong(currentSong++);
        // if (currentSong >= 13)
        //     currentSong = 0;
        XMAS::Utils::checkButton(&dev[0], MCP23017_PIN_BUTTON);
    }

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read())
            delay(5);
        playSong(currentSong++);
        if (currentSong >= 13)
            currentSong = 0;
    }

    if (_data.hal->encoder.wasMoved(true)){
        uint oldSection = currentSelection;
        currentSelection = ((((_data.hal->encoder.getCount() / 2) % 16) + 16) % 16 + 1);
        if (oldSection != currentSelection) {
            rainbowTimeCounter = 0;
        }
        PinSelection selectedPin = selectPin(currentSelection - 1, READ_PINS);
        uint32_t val = 0;
        gpio_compat_read(&dev[selectedPin.address], selectedPin.pin, &val);
        if (val) {
            XMAS::Utils::drawCenterString(_data.hal, std::to_string(currentSelection).c_str());
        } else {
            XMAS::Utils::drawCenterString(_data.hal, (std::to_string(currentSelection) + "*").c_str());
        }
    }
}

void Xmas::onRunning()
{
    onRunningLights();
    onRunningButtons();
    delay(1);
}

void Xmas::onDestroy() { _log("onDestroy"); }
