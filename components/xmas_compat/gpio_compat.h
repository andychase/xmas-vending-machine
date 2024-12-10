#ifndef __GPIO_COMPAT_H__
#define __GPIO_COMPAT_H__

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_USING_SIMULATOR
#include <pcf8575.h>
typedef i2c_dev_t compat_gpio_dev_t;
#else
#include <mcp23x17.h>
typedef mcp23x17_t compat_gpio_dev_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GPIO device descriptor
 *
 * @param dev Pointer to device descriptor
 * @param addr I2C address of the device
 * @param port I2C port number
 * @param sda_gpio SDA GPIO pin number
 * @param scl_gpio SCL GPIO pin number
 * @return `ESP_OK` on success
 */
esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Free the GPIO device descriptor
 *
 * @param dev Pointer to device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t gpio_compat_free(compat_gpio_dev_t *dev);

/**
 * @brief Read the GPIO pins
 *
 * @param dev Pointer to device descriptor
 * @param[out] val 16-bit GPIO pin values
 * @return `ESP_OK` on success
 */
esp_err_t gpio_compat_read(compat_gpio_dev_t *dev, uint16_t *val);

/**
 * @brief Write to the GPIO pins
 *
 * @param dev Pointer to device descriptor
 * @param val 16-bit GPIO pin values to write
 * @return `ESP_OK` on success
 */
esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint16_t val);

#ifdef __cplusplus
}
#endif

#endif // __GPIO_COMPAT_H__
