
#include "xmas_buttons.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <i2cdev.h>
#include <cstdio>

PinSelection selectPin(int index, const uint8_t (*GROUP)[4] = ACTIVE_PINS)
{
    // Assume all groups have the same number of pins
    if (index < 0 || index > TOTAL_PINS)
    {
        return {0, 0};
    }
    uint8_t groupIndex = index / PIN_GROUP_SIZE;
    uint8_t pinIndex = index % PIN_GROUP_SIZE;
    return {ADDRESSES[groupIndex], GROUP[groupIndex][pinIndex]};
}

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            XmasButtons::XmasButtons()
            {
                // Constructor implementation
            }
            void XmasButtons::setupButtons(mcp23x17_t* dev,
                                           const uint8_t (*ACTIVE_PINS)[4],
                                           const uint8_t (*READ_PINS)[4],
                                           int MCP23017_PIN_BUTTON,
                                           mcp23x17_gpio_mode_t MCP23X17_GPIO_INPUT,
                                           mcp23x17_gpio_mode_t MCP23X17_GPIO_OUTPUT,
                                           mcp23x17_gpio_intr_t MCP23X17_INT_LOW_EDGE,
                                           i2c_port_t I2C_NUM_1,
                                           gpio_num_t SDA_GPIO,
                                           gpio_num_t SCL_GPIO)
            {
                // Button logic:
                gpio_compat_i2cScan(I2C_NUM_1, SDA_GPIO, SCL_GPIO);
                esp_err_t ret = i2cdev_init();
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
                for (int i = 0; i < 4; i++)
                {
                    gpio_compat_set_mode(&dev[i], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
                    gpio_compat_set_pullup(&dev[i], MCP23017_PIN_BUTTON, true);
                }

                gpio_compat_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
                gpio_compat_set_interrupt(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_INT_LOW_EDGE);
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        gpio_compat_set_mode(&dev[i], ACTIVE_PINS[i][j], MCP23X17_GPIO_OUTPUT);
                    }
                }
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        gpio_compat_set_mode(&dev[i], READ_PINS[i][j], MCP23X17_GPIO_INPUT);
                        gpio_compat_set_pullup(&dev[i], READ_PINS[i][j], true);
                    }
                }
                scanButtons();
            }
            void XmasButtons::scanButtons()
            {
                esp_err_t ret;
                for (int i = 0; i < TOTAL_PINS; i++)
                {
                    PinSelection sel = selectPin(i, READ_PINS);
                    uint32_t val = 0;
                    ret = gpio_compat_read(&dev[sel.address], sel.pin, &val);
                    if (ret == ESP_OK)
                    {
                        sensedPinState[i] = (val == 0);
                    }
                }
            }
            void XmasButtons::releaseLatch(int selection)
            {
                PinSelection selectedPin = selectPin(selection - 1);
                printf("button pushed, address: %u, pin: %u", selectedPin.address, selectedPin.pin);
                sensedPinState[selection - 1] = false;
                gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 1);
                delay(250);
                gpio_compat_write(&dev[selectedPin.address], selectedPin.pin, 0);
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
