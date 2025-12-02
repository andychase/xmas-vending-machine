#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"
#include <i2cdev.h>
#include <mcp23x17.h>
#include <driver/i2c.h>
#include <cmath>

#include "utils/xmas_img.h"
#include "utils/xmas_lights.h"
#include "utils/xmas_buttons.h"

#define MCP23017_PIN_LED 8    // GPIO pin connected to the LED
#define MCP23017_PIN_BUTTON 0 // GPIO pin connected to the button

#define SECTIONS 4
#define SECTION_SIZE (LED_COUNT / SECTIONS)
#define TRANSITION_STEP 1

#define SCAN_BUTTONS_MS 3000

using namespace MOONCAKE::USER_APP;

static esp_err_t ret;

// Info about buttons
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
#define SPI_CLOCK_SPEED_HZ 400'000
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
    // Button logic moved to XmasButtons
    if (!buttons) buttons = new XMAS::XmasButtons();
    buttons->setupButtons(
        dev,
        ACTIVE_PINS,
        READ_PINS,
        MCP23017_PIN_BUTTON,
        MCP23X17_GPIO_INPUT,
        MCP23X17_GPIO_OUTPUT,
        MCP23X17_INT_LOW_EDGE,
        I2C_NUM_1,
        SDA_GPIO,
        SCL_GPIO
    );
    delay(10);
    buttons->scanButtons();
    setCurrentSelection();
    lastButtonCheckTick = xTaskGetTickCount();
    if (!lights) lights = new XMAS::XmasLights(LED_COUNT);
    lights->startLights(CLOCK_PIN, DATA_PIN, SPI_CLOCK_SPEED_HZ, XMAS_SPI_HOST);
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

void Xmas::setCurrentSelection() {
    int64_t encoderIndex = _data.hal->encoder.getCount() / 2;
    // The extra modulo and addition handle negative values correctly
    uint8_t numberSensed = buttons->numberOfClosedLatches();
    encoderIndex = ((encoderIndex % numberSensed) + numberSensed) % numberSensed;
    currentSelection = buttons->getnthClosedLatch(encoderIndex) + 1;
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
        buttons->releaseLatch(currentSelection);
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
        // Get number of active sensedPinState pins
        u_int8_t numberSensed = buttons->numberOfClosedLatches();
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
           _data.hal->encoder.wasMoved(true);
        } else {
            
            lights->rainbowTimeCounter = 0;
            display.setBrightness(128);
            XMAS::Utils::drawCenterString(_data.hal, std::to_string(currentSelection).c_str());
        }
    }

    if ((lastButtonCheckTick - xTaskGetTickCount()) > pdMS_TO_TICKS(SCAN_BUTTONS_MS)) {
        buttons->scanButtons();
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
