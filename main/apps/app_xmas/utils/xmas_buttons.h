#pragma once
#include "../../app.h"
#include "../../../hal/hal.h"
#include "../../common_define.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <cstdint>

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

static const uint8_t ADDRESSES[] = {0, 1, 2, 3};

struct PinSelection
{
    uint8_t address;
    uint8_t pin;
};

namespace MOONCAKE {
namespace USER_APP {
namespace XMAS {
    class XmasButtons {
    public:
        bool sensedPinState[16] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true};
        XmasButtons();
        void setupButtons(
            mcp23x17_t* dev,
            const uint8_t (*ACTIVE_PINS)[4],
            const uint8_t (*READ_PINS)[4],
            int MCP23017_PIN_BUTTON,
            mcp23x17_gpio_mode_t MCP23X17_GPIO_INPUT,
            mcp23x17_gpio_mode_t MCP23X17_GPIO_OUTPUT,
            mcp23x17_gpio_intr_t MCP23X17_INT_LOW_EDGE,
            i2c_port_t I2C_NUM_1,
            gpio_num_t SDA_GPIO,
            gpio_num_t SCL_GPIO
        );
        void scanButtons();
        void releaseLatch(int selection);
    private:
        // Add private members as needed
    };
} // namespace XMAS
} // namespace USER_APP
} // namespace MOONCAKE
