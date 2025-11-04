#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"
#include <i2cdev.h>
#include <mcp23x17.h>
#include <driver/i2c.h>
#include <cmath>

#include "utils/xmas_img.h"
#include "utils/xmas_lights.h"

#define MCP23017_PIN_LED 8    // GPIO pin connected to the LED
#define MCP23017_PIN_BUTTON 0 // GPIO pin connected to the button

#define SECTIONS 4
#define SECTION_SIZE (LED_COUNT / SECTIONS)
#define TRANSITION_STEP 1

#define SCAN_BUTTONS_MS 1000

using namespace MOONCAKE::USER_APP;

static esp_err_t ret;
#ifdef CONFIG_USING_SIMULATOR
static mcp23x17_t dev[1];
static const uint8_t ACTIVE_PINS[1][4] = {{0, 0, 0, 0}};
static const uint8_t READ_PINS[1][4] = {{2, 2, 2, 2}};
#define SDA_GPIO GPIO_NUM_1
#define SCL_GPIO GPIO_NUM_2
#define PIN_GROUP_SIZE 4   
#define TOTAL_PINS 4
#define USE_ENCODER_FOR_SELECTION 1
#define RUN_BUTTON_SCAN 0
#else
static mcp23x17_t dev[4];
static const uint8_t ACTIVE_PINS[4][4] = {{8, 9, 10, 11}, {11, 10, 9, 8}, {11, 10, 9, 8}, {11, 10, 9, 8}};
static const uint8_t READ_PINS[4][4] = {{3, 4, 5, 6}, {3, 4, 5, 6}, {3, 4, 5, 6}, {3, 4, 5, 6}};
#define SDA_GPIO GPIO_NUM_13
#define SCL_GPIO GPIO_NUM_15
#define PIN_GROUP_SIZE 4    
#define TOTAL_PINS 16
#define USE_ENCODER_FOR_SELECTION 0
#define RUN_BUTTON_SCAN 1
#endif

// Info about lights
#define SPI_CLOCK_SPEED_HZ 300'000
#ifdef CONFIG_USING_SIMULATOR
#define XMAS_SPI_HOST SPI3_HOST
#define LED_COUNT 15 // Number of LEDs in the strip
#define CLOCK_PIN 9   // GPIO for clock input (CLK)
#define DATA_PIN 8    // GPIO for data input (MOSI)
#define RUN_LIGHTS 0
#else
#define XMAS_SPI_HOST SPI2_HOST
#define LED_COUNT 576 // Number of LEDs in the strip
#define CLOCK_PIN 2   // GPIO for clock input (CLK)
#define DATA_PIN 1    // GPIO for data input (MOSI)
#define RUN_LIGHTS 0
#endif



static const uint8_t ADDRESSES[] = {0, 1, 2, 3};

struct PinSelection
{
    uint8_t address;
    uint8_t pin;
};


PinSelection selectPin(int index, const uint8_t (*GROUP)[4] = ACTIVE_PINS)
{
    // Assume all groups have the same number of pins
    if (index < 0 || index > TOTAL_PINS)
    {
        return { 0, 0 };
    }
    uint8_t groupIndex = index / PIN_GROUP_SIZE;
    uint8_t pinIndex = index % PIN_GROUP_SIZE;
    return { ADDRESSES[groupIndex], GROUP[groupIndex][pinIndex] };
}


void Xmas::startLights() {
    if (!lights) lights = new XMAS::XmasLights(LED_COUNT);
    lights->startLights(CLOCK_PIN, DATA_PIN, SPI_CLOCK_SPEED_HZ, XMAS_SPI_HOST);
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
    XMAS::Utils::drawCenterString(_data.hal, "");
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

    // Make sure other control units have their 0 pin off
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
    {
        gpio_compat_set_mode(&dev[i], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
        gpio_compat_set_pullup(&dev[i], MCP23017_PIN_BUTTON, true);
    }

    gpio_compat_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
    gpio_compat_set_interrupt(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_INT_LOW_EDGE);
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

    scanButtons();
    lastButtonCheckTick = xTaskGetTickCount();
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
        pauseDuration = std::max(0, pauseDuration);
        if (duration != 0) {
            _data.hal->buzz.tone(tone, duration, pauseDuration);
        }
    }
    _data.hal->buzz.noTone();
}


void Xmas::scanButtons() {
    if (!RUN_BUTTON_SCAN) {
        return;
    }
    for (int i = 0; i < TOTAL_PINS; i++)
    {
        PinSelection sel = selectPin(i, READ_PINS);
        uint32_t val = 0;
        ret = gpio_compat_read(&dev[sel.address], sel.pin, &val);
        if (ret == ESP_OK) {
            sensedPinState[i] = (val == 0);
        }
    }
}

void Xmas::onRunningButtons() {
    LGFX_StampRing display = _data.hal->display;
    bool encoderButtonPressed = false;
    if (!_data.hal->encoder.btn.read())
    {
        for(int i = 0; i < 100; i++) {
            if (!_data.hal->encoder.btn.read()) {
                delay(5);
            } else {
                break;
            }
        } 
        encoderButtonPressed = true;
    }
    uint32_t buttonState = 0;
    if (USE_ENCODER_FOR_SELECTION) {
        buttonState = encoderButtonPressed ? 0 : 1;
        ret = ESP_OK;
    } else {
        ret = gpio_compat_read(&dev[0], MCP23017_PIN_BUTTON, &buttonState);
    }
    
    if (ret == ESP_OK && buttonState == 0)
    { 
        PinSelection selectedPin = selectPin(currentSelection - 1);
        _log("button pushed, address: %u, pin: %u", selectedPin.address, selectedPin.pin);
        sensedPinState[currentSelection - 1] = false;
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 1);
        delay(250);
        gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 0);
        XMAS::Utils::showAnimation(
            &XMASPIMAGE1, 
            _data.hal->canvas,
            (display.width() / 2) - (XMASPIMAGE1.width/2), \
            (display.height() / 2) - (XMASPIMAGE1.height/2), 
            5,
            3000,
            0xFFFFFF
        );
        XMAS::Utils::drawCenterString(_data.hal, std::to_string(currentSelection).c_str());
        // playSong(currentSong++);
        // if (currentSong >= 13)
        //     currentSong = 0;
        XMAS::Utils::checkButton(&dev[0], MCP23017_PIN_BUTTON);
    }

    if (encoderButtonPressed) {
        playSong(currentSong++);
        if (currentSong >= 13)
            currentSong = 0;
    }

    if (_data.hal->encoder.wasMoved(true)) {
        uint oldSection = currentSelection;
        // Get number of active sensedPinState pins
        u_int8_t numberSensed = 0;
        for (int i = 0; i < TOTAL_PINS; i++) {
            if (sensedPinState[i]) {
                numberSensed++;
            }
        }
        if (numberSensed == 0) {
                display.setBrightness(128);
                XMAS::Utils::showAnimation(
                &XMASPIMAGE2, 
                _data.hal->canvas,
                (_data.hal->display.width() / 2) - ((XMASPIMAGE2.width/2)*4),
                (_data.hal->display.height() / 2) - ((XMASPIMAGE2.height/2)*4), 
                20,
                3000,
                0x000000,
                4.0f, 
                4.0f
            );
           display.setBrightness(0);
        } else {
            currentSelection = (((_data.hal->encoder.getCount() / 2) % numberSensed) + numberSensed) % numberSensed + 1;
            for (int i = 0, j = 0; i < TOTAL_PINS; i++) {
                if (sensedPinState[i]) {
                    j++;
                    if (j == currentSelection) {
                        currentSelection = i + 1;
                        break;
                    }
                }
            }
            lights->rainbowTimeCounter = 0;
            display.setBrightness(128);
            XMAS::Utils::drawCenterString(_data.hal, std::to_string(currentSelection).c_str());
        }
    }

    if ((lastButtonCheckTick - xTaskGetTickCount()) > pdMS_TO_TICKS(SCAN_BUTTONS_MS)) {
        scanButtons();
        lastButtonCheckTick = xTaskGetTickCount();
    }
    
    if (lights->rainbowTimeCounter < 500) {
        lights->rainbowTimeCounter++;
    } else {
        display.setBrightness(0);
    }

}

void Xmas::onRunning()
{
    lights->onRunningLights(currentSelection);
    onRunningButtons();
    delay(1);
}

void Xmas::onDestroy() { _log("onDestroy"); }
