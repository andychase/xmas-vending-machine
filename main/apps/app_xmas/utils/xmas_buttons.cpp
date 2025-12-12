
#include "xmas_buttons.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <i2cdev.h>
#include <cstdio>

#define SCAN_BUTTONS_MS 20

PinSelection selectPin(int index, const uint8_t (*GROUP)[PIN_GROUP_SIZE])
{
    if (index < 0 || index >= TOTAL_PINS)
    {
        return {0, 0};
    }
    uint8_t groupIndex = index / PIN_GROUP_SIZE;
    uint8_t pinIndex = index % PIN_GROUP_SIZE;
    return {groupIndex, GROUP[groupIndex][pinIndex]};
}

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            XmasButtons::XmasButtons()
            {}
            void XmasButtons::setupButtons(mcp23x17_t* dev,
                                           const uint8_t (*ACTIVE_PINS)[PIN_GROUP_SIZE],
                                           const uint8_t (*READ_PINS)[PIN_GROUP_SIZE],
                                           int MCP23017_PIN_BUTTON,
                                           mcp23x17_gpio_mode_t MCP23X17_GPIO_INPUT,
                                           mcp23x17_gpio_mode_t MCP23X17_GPIO_OUTPUT,
                                           mcp23x17_gpio_intr_t MCP23X17_INT_LOW_EDGE,
                                           i2c_port_t I2C_NUM_1,
                                           gpio_num_t SDA_GPIO,
                                           gpio_num_t SCL_GPIO,
                                           OnButtonChangeCallback onButtonChangeCallback,
                                           OnErrorFlagCallback onErrorFlagCallback,
                                           OnReleaseButtonPressedCallback onReleaseButtonPressed)
            {
                this->dev = dev;
                this->ACTIVE_PINS = ACTIVE_PINS;
                this->READ_PINS = READ_PINS;
                this->MCP23017_PIN_BUTTON = MCP23017_PIN_BUTTON;
                this->onButtonChangeCallback = onButtonChangeCallback;
                this->onErrorFlagCallback = onErrorFlagCallback;
                this->onReleaseButtonPressed = onReleaseButtonPressed;
                
                gpio_compat_i2cScan(I2C_NUM_1, SDA_GPIO, SCL_GPIO);
                esp_err_t ret = i2cdev_init();
                PinSelection sel;
                if (ret != ESP_OK)
                {
                    printf("Failed to initialize I2C driver: %s\n", esp_err_to_name(ret));
                    errorFlag = true;
                }
                for (int addr = 0x21; addr <= 0x24; addr++)
                {
                    compat_gpio_dev_t* _dev = &dev[addr - 0x21];
                    CHECK(gpio_compat_init(_dev, addr, I2C_NUM_1, SDA_GPIO, SCL_GPIO));
                }

                // Make sure other control units have their 0 pin off
                for (int i = 0; i < 4; i++)
                {
                    CHECK(gpio_compat_set_mode(&dev[i], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT));
                    CHECK(gpio_compat_set_pullup(&dev[i], MCP23017_PIN_BUTTON, true));
                }
                // Lowest shelf control unit handles the button
                CHECK(gpio_compat_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT));
                // Set latch pins as outputs 
                for (int i = 0; i < TOTAL_PINS; i++)
                {
                    sel = selectPin(i, ACTIVE_PINS);
                    CHECK(gpio_compat_set_mode(&dev[sel.address], sel.pin, MCP23X17_GPIO_OUTPUT));
                }
                // Sensing buttons
                for (int i = 0; i < TOTAL_PINS; i++)
                {
                    sel = selectPin(i, READ_PINS);
                    CHECK(gpio_compat_set_mode(&dev[sel.address], sel.pin, MCP23X17_GPIO_INPUT));
                    CHECK(gpio_compat_set_pullup(&dev[sel.address], sel.pin, true));
                }
            }
            void XmasButtons::updateLatchState(uint8_t index, bool isClosed)
            {
                if (index >= TOTAL_PINS)
                {
                    return;
                }
                sensedLatchClosed[index] = isClosed;
            }
            PinScanResult XmasButtons::scanNextButton()
            {
                esp_err_t ret;
                lastScannedButton = (lastScannedButton + 1) % TOTAL_PINS;
                PinSelection sel = selectPin(lastScannedButton, READ_PINS);
                uint32_t val = 0;
                ret = gpio_compat_read(&dev[sel.address], sel.pin, &val);
                CHECK(ret);
                if (ret == ESP_OK)
                {
                    // Closed latches are low (0)
                    updateLatchState(lastScannedButton, val == 0);
                }
                if (onButtonChangeCallback)
                    onButtonChangeCallback({lastScannedButton, val == 0});
                return {lastScannedButton, val == 0};
            }
            void XmasButtons::scanAllButtons()
            {
                for (int i = 0; i < TOTAL_PINS; i++)
                {
                    scanNextButton();
                }
            }
            bool XmasButtons::checkLatchIsClosed(uint8_t index)
            {
                if (index >= TOTAL_PINS)
                {
                    return false;
                }
                return sensedLatchClosed[index];
            }
            uint8_t XmasButtons::numberofLatches() {
                return TOTAL_PINS;
            }
            uint8_t XmasButtons::numberOfClosedLatches()
            {
                uint8_t count = 0;
                for (int i = 0; i < TOTAL_PINS; i++)
                    if (checkLatchIsClosed(i))
                        count++;
                return count;
            }
            uint8_t XmasButtons::getnthClosedLatch(uint8_t nthClosedIndex) {
                uint8_t count = 0;
                for (int i = 0; i < TOTAL_PINS; i++) {
                    if (checkLatchIsClosed(i)) {
                        if (count == nthClosedIndex) {
                            return i;
                        }
                        count++;
                    }
                }
                return 0;
            }
            uint8_t XmasButtons::getCurrentSelection(int64_t encoderIndex) {
                // The extra modulo and addition handle negative values correctly
                    uint8_t numberSensed = numberOfClosedLatches();
                        if (numberSensed == 0) {
                        return 1;
                    }
                    encoderIndex = ((encoderIndex % numberSensed) + numberSensed) % numberSensed;
                    return getnthClosedLatch(encoderIndex) + 1;
            }

            void XmasButtons::releaseLatch(int selection)
            {
                PinSelection selectedPin = selectPin(selection - 1, ACTIVE_PINS);
                printf("button pushed, address: %u, pin: %u\n", selectedPin.address, selectedPin.pin);
                sensedLatchClosed[selection - 1] = false;
                gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 1);
                delay(250);
                gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 0);
            }
            bool XmasButtons::checkReleaseButton()
            {
                esp_err_t ret;
                uint32_t val = 0;
                ret = gpio_compat_read(&dev[0], MCP23017_PIN_BUTTON, &val);
                CHECK(ret);
                if (ret == ESP_OK)
                {
                    // Closed latches are low (0)
                    return (val == 0);
                }
                return false;
            }
            void XmasButtons::onRunning(uint8_t currentSelection)
            {
                bool cooldownPassed = (_time_since_ms(buttonCheckCooldownTick) > SCAN_BUTTONS_MS);
                if (checkReleaseButton()) {
                    if (cooldownPassed && !buttonDown) {
                        if (onReleaseButtonPressed)
                            onReleaseButtonPressed();
                        // Gives time from light power spike to dissipate
                        delay(10);
                        releaseLatch(currentSelection);
                        scanAllButtons();
                        buttonDown = true;
                    }
                } else if (buttonDown) {
                    buttonDown = false;
                    buttonCheckCooldownTick = xTaskGetTickCount();
                    lastLatchScanTick = buttonCheckCooldownTick;
                }

                if (_time_since_ms(lastLatchScanTick) > SCAN_BUTTONS_MS) {
                    scanNextButton();
                    lastLatchScanTick = xTaskGetTickCount();
                }
                if (errorFlag && !uiSentErrorFlag) {
                    if (onErrorFlagCallback)
                        onErrorFlagCallback();
                    uiSentErrorFlag = true;
                }
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
