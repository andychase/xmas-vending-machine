#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <driver/i2c.h>

#define _get_encoder_count() ((((_data.hal->encoder.getCount() / 2) % 16) + 16) % 16 + 1)

#define SDA_GPIO GPIO_NUM_13
#define SCL_GPIO GPIO_NUM_15

#define MCP23017_PIN_LED 8    // GPIO pin connected to the LED
#define MCP23017_PIN_BUTTON 9 // GPIO pin connected to the button

using namespace MOONCAKE::USER_APP;

static esp_err_t ret;
static mcp23x17_t dev[2];
static const int ACTIVE_PINS[][8] = {{2, 3, 4, 5, 6, 7, 14, 15}, {0, 1, 2, 3, 4, 5, 6, 7}};

static const int ADDRESSES[] = {0, 1};

struct PinSelection
{
    int address;
    int pin;
};

PinSelection selectPin(int index)
{
    for (int group = 0; group < 2; ++group)
    {
        int groupSize = sizeof(ACTIVE_PINS[group]) / sizeof(ACTIVE_PINS[group][0]);
        if (index < groupSize)
        {
            return {ADDRESSES[group], ACTIVE_PINS[group][index]};
        }
        index -= groupSize;
    }
    return {0, 0};
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

    for (int addr = 0x20; addr < 0x22; addr++)
    {
        compat_gpio_dev_t* _dev = &dev[addr - 0x20];
        gpio_compat_init(_dev, addr, I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    }

    gpio_compat_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
    gpio_compat_set_pullup(&dev[0], MCP23017_PIN_BUTTON, true);
    gpio_compat_set_interrupt(&dev[0], 9, MCP23X17_INT_LOW_EDGE);
    for (int i = 0; i < sizeof(ACTIVE_PINS) / sizeof(ACTIVE_PINS[0]); i++)
    {
        for (int j = 0; j < sizeof(ACTIVE_PINS[i]) / sizeof(ACTIVE_PINS[i][0]); j++)
        {
            gpio_compat_set_mode(&dev[i], ACTIVE_PINS[i][j], MCP23X17_GPIO_OUTPUT);
        }
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

void Xmas::onRunning()
{
    bool buttonPushed = XMAS::Utils::checkButton(&dev[0], MCP23017_PIN_BUTTON);
    if (buttonPushed)
    {
        PinSelection selectedPin = selectPin(_get_encoder_count() - 1);
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 1);
        delay(250);
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 0);
        delay(1000);
        XMAS::Utils::checkButton(&dev[0], MCP23017_PIN_BUTTON);
    }

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read())
            delay(5);
        playSong(currentSong++);
        if (currentSong >= 13)
            currentSong = 0;
    }

    if (_data.hal->encoder.wasMoved(true))
    {
        XMAS::Utils::drawCenterString(_data.hal, std::to_string((_get_encoder_count())).c_str());
    }

    delay(20);
}

void Xmas::onDestroy() { _log("onDestroy"); }
