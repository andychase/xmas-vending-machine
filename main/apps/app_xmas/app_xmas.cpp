#include "app_xmas.h"
#include "../common_define.h"
#include "gpio_compat.h"
#include <mcp23x17.h>
#include <driver/i2c.h>


#define SDA_GPIO GPIO_NUM_13
#define SCL_GPIO GPIO_NUM_15

#define MCP23017_PIN_LED     8    // GPIO pin connected to the LED
#define MCP23017_PIN_BUTTON  9    // GPIO pin connected to the button


using namespace MOONCAKE::USER_APP;

static esp_err_t ret;
static mcp23x17_t dev[2];
static const int ACTIVE_PINS[][8] = {{2,3,4,5, 6, 7, 14, 15}, {0, 1, 2, 3, 4, 5, 6, 7}};

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
    XMAS::Utils::drawCenterString(_data.hal, "XMAS");

    gpio_compat_i2cScan(I2C_NUM_1, SDA_GPIO, SCL_GPIO);

    ret = i2cdev_init();
    if (ret != ESP_OK)
    {
        printf("Failed to initialize I2C driver: %s\n", esp_err_to_name(ret));
        return;
    }

    for (int addr = 0x20; addr < 0x22; addr++)
    {
        compat_gpio_dev_t * _dev = &dev[addr - 0x20];
        gpio_compat_init(_dev, addr, I2C_NUM_1, SDA_GPIO, SCL_GPIO);
    }


    mcp23x17_set_mode(&dev[0], MCP23017_PIN_BUTTON, MCP23X17_GPIO_INPUT);
    mcp23x17_set_pullup(&dev[0], MCP23017_PIN_BUTTON, true);
    mcp23x17_set_interrupt(&dev[0], 9, MCP23X17_INT_LOW_EDGE); // Interrupt on high edge
    // For pin in ACTIVE_PINS (whereas ACTIVE_PIN[0] is 0x20), set each pin to output
    for (int i = 0; i < sizeof(ACTIVE_PINS) / sizeof(ACTIVE_PINS[0]); i++)
    {
        for (int j = 0; j < sizeof(ACTIVE_PINS[i]) / sizeof(ACTIVE_PINS[i][0]); j++)
        {
            mcp23x17_set_mode(&dev[i], ACTIVE_PINS[i][j], MCP23X17_GPIO_OUTPUT);
        }
    }
}

void Xmas::onRunning() {
    uint32_t val;
    mcp23x17_get_level(&dev[0], MCP23017_PIN_BUTTON, &val);
    for (int i = 0; i < sizeof(ACTIVE_PINS) / sizeof(ACTIVE_PINS[0]); i++)
    {
        for (int j = 0; j < sizeof(ACTIVE_PINS[i]) / sizeof(ACTIVE_PINS[i][0]); j++)
        {
            mcp23x17_set_level(&dev[i], ACTIVE_PINS[i][j], val);
        }
    }

    delay(2000);

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read())
            delay(5);
        destroyApp();
    }
}

void Xmas::onDestroy() {
    _log("onDestroy");
}
