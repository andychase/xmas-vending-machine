/*
 * Copyright (c) 2018 Ruslan V. Uss <unclerus@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of itscontributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file mcp23x17.h
 * @defgroup mcp23x17 mcp23x17
 * @{
 *
 * ESP-IDF driver for I2C/SPI 16 bit GPIO expanders MCP23017/MCP23S17
 *
 * Copyright (c) 2018 Ruslan V. Uss <unclerus@gmail.com>
 *
 * BSD Licensed as described in the file LICENSE
 */
#ifndef __MCP23X17_H__
#define __MCP23X17_H__

#include <stddef.h>
#include <stdbool.h>
#include <i2cdev.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

#define MCP23X17_ADDR_BASE 0x20

#define MCP_REG_IODIRA   0x00
#define MCP_REG_IODIRB   0x01
#define MCP_REG_IPOLA    0x02
#define MCP_REG_IPOLB    0x03
#define MCP_REG_GPINTENA 0x04
#define MCP_REG_GPINTENB 0x05
#define MCP_REG_DEFVALA  0x06
#define MCP_REG_DEFVALB  0x07
#define MCP_REG_INTCONA  0x08
#define MCP_REG_INTCONB  0x09
#define MCP_REG_IOCON    0x0A
#define MCP_REG_GPPUA    0x0C
#define MCP_REG_GPPUB    0x0D
#define MCP_REG_INTFA    0x0E
#define MCP_REG_INTFB    0x0F
#define MCP_REG_INTCAPA  0x10
#define MCP_REG_INTCAPB  0x11
#define MCP_REG_GPIOA    0x12
#define MCP_REG_GPIOB    0x13
#define MCP_REG_OLATA    0x14
#define MCP_REG_OLATB    0x15

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MCP23X17_IFACE_I2C

typedef i2c_dev_t mcp23x17_t;

#else

#define MCP23X17_MAX_SPI_FREQ SPI_MASTER_FREQ_10M // 10MHz

typedef struct
{
    spi_device_interface_config_t spi_cfg;
    spi_device_handle_t spi_dev;
    uint8_t addr;
    bool use_software_cs; //!< Use software CS control instead of hardware.
                          //!< Whether to use a software Chip Select (CS) line instead of the hardware
                          //!< one. This is useful when multiple MCP23S17 chips are sharing the same CS
                          //!< line on the SPI bus.
    gpio_port_t cs_pin;   //!< GPIO pin number for CS.
} mcp23x17_t;

#endif

/**
 * GPIO mode
 */
typedef enum
{
    MCP23X17_GPIO_OUTPUT = 0,
    MCP23X17_GPIO_INPUT
} mcp23x17_gpio_mode_t;

/**
 * INTA/INTB pins mode
 */
typedef enum
{
    MCP23X17_ACTIVE_LOW = 0, //!< Low level on interrupt
    MCP23X17_ACTIVE_HIGH,    //!< High level on interrupt
    MCP23X17_OPEN_DRAIN      //!< Open drain
} mcp23x17_int_out_mode_t;

/**
 * Interrupt mode
 */
typedef enum
{
    MCP23X17_INT_DISABLED = 0, //!< No interrupt
    MCP23X17_INT_LOW_EDGE,     //!< Interrupt on low edge
    MCP23X17_INT_HIGH_EDGE,    //!< Interrupt on high edge
    MCP23X17_INT_ANY_EDGE      //!< Interrupt on any edge
} mcp23x17_gpio_intr_t;

#ifdef CONFIG_MCP23X17_IFACE_I2C

/**
 * @brief Initialize device descriptor
 *
 * Default SCL frequency is 1MHz.
 *
 * @param dev Pointer to device descriptor
 * @param port I2C port number
 * @param addr I2C address
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_init_desc(mcp23x17_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Free device descriptor
 *
 * @param dev Pointer to device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_free_desc(mcp23x17_t *dev);

#else

/**
 * @brief Initialize device descriptor
 *
 * @param dev Pointer to device descriptor
 * @param host SPI host
 * @param clock_speed_hz SPI clock speed, Hz (max `MCP23X17_MAX_SPI_FREQ`)
 * @param addr Device address
 * @param cs_pin CS pin
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_init_desc_spi(mcp23x17_t *dev, spi_host_device_t host, uint32_t clock_speed_hz, uint8_t addr, gpio_num_t cs_pin);

/**
 * @brief Free device descriptor
 *
 * @param dev Pointer to device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_free_desc_spi(mcp23x17_t *dev);

#endif

#ifdef CONFIG_MCP23X17_IFACE_SPI

/**
 * @brief Enable or disable hardware addressing (usage of pins A0..A2).
 *
 * Works only with MCP23S17.
 * Warining! According to the datasheet, hardware addressing is disabled by default.
 *
 * @param dev Pointer to device descriptor
 * @param enable `true` to enable hardware addressing
 * @param new_addr New I2C address (`0b0100<A2><A1><A0>` after the addressing enabled).
 *                 If `enable` is `false`, address will be set automatically
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_setup_hw_addr(mcp23x17_t *dev, bool enable, uint8_t new_addr);

#endif

/**
 * @brief Get INTA/INTB pins mode
 *
 * @param dev Pointer to device descriptor
 * @param[out] mode Buffer to store mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_int_out_mode(mcp23x17_t *dev, mcp23x17_int_out_mode_t *mode);

/**
 * @brief Set INTA/INTB pins mode
 *
 * @param dev Pointer to device descriptor
 * @param mode INTA/INTB pins mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_int_out_mode(mcp23x17_t *dev, mcp23x17_int_out_mode_t mode);

/**
 * @brief Get GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param[out] val Buffer to store mode, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return
 */
esp_err_t mcp23x17_port_get_mode(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Set GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param val Mode, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_mode(mcp23x17_t *dev, uint16_t val);

/**
 * @brief Get GPIO pullups status
 *
 * 0 - pullup disabled, 1 - pullup enabled for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param[out] val Pullup status, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_get_pullup(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Set GPIO pullups status
 *
 * 0 - pullup disabled, 1 - pullup enabled for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param val Pullup status, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_pullup(mcp23x17_t *dev, uint16_t val);

/**
 * @brief Read GPIO port value
 *
 * @param dev Pointer to device descriptor
 * @param[out] val 16-bit GPIO port value, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_read(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Write value to GPIO port
 *
 * @param dev Pointer to device descriptor
 * @param val GPIO port value, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_write(mcp23x17_t *dev, uint16_t val);

/**
 * @brief Get GPIO pin mode
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] mode GPIO pin mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_mode(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_mode_t *mode);

/**
 * @brief Set GPIO pin mode
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param mode GPIO pin mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_mode(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_mode_t mode);

/**
 * @brief Get pullup mode of GPIO pin
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] enable pullup mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_pullup(mcp23x17_t *dev, uint8_t pin, bool *enable);

/**
 * @brief Set pullup mode of GPIO pin
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param enable `true` to enable pullup
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_pullup(mcp23x17_t *dev, uint8_t pin, bool enable);

/**
 * @brief Read GPIO pin level
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_level(mcp23x17_t *dev, uint8_t pin, uint32_t *val);

/**
 * @brief Set GPIO pin level
 *
 * Pin must be set up as output
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_level(mcp23x17_t *dev, uint8_t pin, uint32_t val);

/**
 * @brief Setup interrupt for group of GPIO pins
 *
 * @param dev Pointer to device descriptor
 * @param mask Pins to setup
 * @param intr Interrupt mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_interrupt(mcp23x17_t *dev, uint16_t mask, mcp23x17_gpio_intr_t intr);

/**
 * @brief Setup interrupt for GPIO pin
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param intr Interrupt mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_interrupt(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_intr_t intr);

esp_err_t read_reg_16(mcp23x17_t *dev, uint8_t reg, uint16_t *val);

esp_err_t write_reg_16(mcp23x17_t *dev, uint8_t reg, uint16_t val);

esp_err_t write_reg_bit_16(mcp23x17_t *dev, uint8_t reg, bool val, uint8_t bit);

esp_err_t read_reg_bit_8(mcp23x17_t *dev, uint8_t reg, bool *val, uint8_t bit);

esp_err_t write_reg_bit_8(mcp23x17_t *dev, uint8_t reg, bool val, uint8_t bit);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __MCP23X17_H__ */
