#pragma once
#include "../../app.h"
#include "../../../hal/hal.h"
#include "../../common_define.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <cstdint>

#define PIN_GROUP_SIZE 4    
#define TOTAL_PINS 16
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) {errorFlag = true; } } while (0)

struct PinSelection
{
    uint8_t address;
    uint8_t pin;
};

struct PinScanResult 
{
    uint8_t pin;
    bool isClosed;
};

namespace MOONCAKE {
namespace USER_APP {
namespace XMAS {
    class XmasButtons {
    public:
        
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
        void updateLatchState(uint8_t index, bool isClosed);
        void scanAllButtons();
        PinScanResult scanNextButton();
        void releaseLatch(int selection);
        bool checkReleaseButton();
        bool checkLatchIsClosed(uint8_t index);
        uint8_t numberofLatches();
        uint8_t numberOfClosedLatches();
        uint8_t getnthClosedLatch(uint8_t nthClosedIndex);
        uint8_t getCurrentSelection(int64_t encoderIndex);
        bool buttonDown = false;
        bool errorFlag = false;
        bool uiSentErrorFlag = false;

    private:
        mcp23x17_t* dev;
        const uint8_t (*ACTIVE_PINS)[4];
        const uint8_t (*READ_PINS)[4];
        bool sensedLatchClosed[16] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true};
        int MCP23017_PIN_BUTTON;
        uint8_t lastScannedButton = 0;
        // Add private members as needed
    };
} // namespace XMAS
} // namespace USER_APP
} // namespace MOONCAKE
