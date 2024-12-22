#ifndef __GPIO_COMPAT_H__
#define __GPIO_COMPAT_H__

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>
#include <pcf8575.h>
#include <mcp23x17.h>

#ifdef CONFIG_USING_SIMULATOR
typedef i2c_dev_t compat_gpio_dev_t;
#else
typedef mcp23x17_t compat_gpio_dev_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

esp_err_t gpio_compat_free(compat_gpio_dev_t *dev);

esp_err_t gpio_compat_read(compat_gpio_dev_t *dev, uint8_t pin, uint32_t *val);

esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint8_t pin, uint32_t val);

void gpio_compat_i2cScan(i2c_port_t i2cPort, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

void gpio_compat_set_mode(compat_gpio_dev_t *dev, uint8_t pin, mcp23x17_gpio_mode_t mode);
void gpio_compat_set_pullup(compat_gpio_dev_t *dev, uint8_t pin, bool enable);
void gpio_compat_set_interrupt(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_intr_t intr);

#ifdef __cplusplus
}
#endif

#endif // __GPIO_COMPAT_H__
