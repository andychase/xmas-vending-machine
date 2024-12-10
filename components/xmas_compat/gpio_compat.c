#include "gpio_compat.h"

#ifdef CONFIG_USING_SIMULATOR
// Using PCF8575
esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    dev->port = port;
    dev->cfg.mode = I2C_MODE_MASTER;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    dev->cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    dev->cfg.master.clk_speed = 400000; // 400 kHz I2C clock frequency
    dev->cfg.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    dev->addr = addr;
    dev->mutex = NULL; // Mutex will be initialized later
    dev->timeout_ticks = 0; // Default timeout
    return pcf8575_init_desc(dev, addr, port, sda_gpio, scl_gpio);
}

esp_err_t gpio_compat_free(compat_gpio_dev_t *dev) {
    return pcf8575_free_desc(dev);
}

esp_err_t gpio_compat_read(compat_gpio_dev_t *dev, uint16_t *val) {
    return pcf8575_port_read(dev, val);
}

esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint16_t val) {
    return pcf8575_port_write(dev, val);
}

#else
// Using MCP23X17
esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    dev->port = port;
    dev->cfg.mode = I2C_MODE_MASTER;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    dev->cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    dev->cfg.master.clk_speed = 400000; // 400 kHz I2C clock frequency
    dev->cfg.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    dev->addr = addr;
    dev->mutex = NULL; // Mutex will be initialized later
    dev->timeout_ticks = 0; // Default timeout
    return mcp23x17_init_desc(dev, addr, port, sda_gpio, scl_gpio);
}

esp_err_t gpio_compat_free(compat_gpio_dev_t *dev) {
    return mcp23x17_free_desc(dev);
}

esp_err_t gpio_compat_read(compat_gpio_dev_t *dev, uint16_t *val) {
    return mcp23x17_port_read(dev, val);
}

esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint16_t val) {
    mcp23x17_set_mode(dev, 0, MCP23X17_GPIO_OUTPUT);
    return mcp23x17_set_level(dev, 0, val);
}
#endif
    