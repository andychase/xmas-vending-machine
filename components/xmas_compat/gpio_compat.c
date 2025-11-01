#include "gpio_compat.h"
#include <string.h>

#ifdef CONFIG_USING_SIMULATOR
// Using PCF8575
esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    dev->port = port;
    dev->cfg.mode = I2C_MODE_MASTER;
    dev->cfg.sda_io_num = GPIO_NUM_1;
    dev->cfg.scl_io_num = GPIO_NUM_2;
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

esp_err_t gpio_compat_read(compat_gpio_dev_t *dev, uint8_t pin, uint32_t *val) {
    uint32_t temp_val = 0;
    esp_err_t err = pcf8575_port_read(dev, &temp_val);
    if (err != ESP_OK) {
        return err;
    }

    *val = (temp_val >> pin) & 0x1; // Extract the specific pin's value
    return ESP_OK;
}

esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint8_t pin, uint32_t val) {
    uint32_t temp_val = 0;
    esp_err_t err = pcf8575_port_read(dev, &temp_val);
    if (err != ESP_OK) {
        return err;
    }
    if (val) {
        temp_val |= (1 << pin);
    } else {
        temp_val &= ~(1 << pin);
    }
    return pcf8575_port_write(dev, temp_val);
}

void gpio_compat_set_mode(compat_gpio_dev_t *dev, uint8_t pin, mcp23x17_gpio_mode_t mode) {}


void gpio_compat_set_pullup(compat_gpio_dev_t *dev, uint8_t pin, bool enable) {}


void gpio_compat_set_interrupt(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_intr_t intr) {}

#else
// Using MCP23X17
esp_err_t gpio_compat_init(compat_gpio_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    dev->port = I2C_NUM_1;
    dev->cfg.mode = I2C_MODE_MASTER;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.sda_pullup_en = GPIO_PULLUP_DISABLE;
    dev->cfg.scl_pullup_en = GPIO_PULLUP_DISABLE;
    dev->cfg.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    dev->addr = addr;
    dev->mutex = NULL;
    dev->timeout_ticks = 0;
    esp_err_t ret = mcp23x17_init_desc(dev, dev->addr, dev->port, sda_gpio, scl_gpio);
    dev->cfg.master.clk_speed = 100000;
    return ret;
}

esp_err_t gpio_compat_free(compat_gpio_dev_t *dev) {
    return mcp23x17_free_desc(dev);
}

esp_err_t gpio_compat_read(compat_gpio_dev_t *dev,  uint8_t pin, uint32_t *val) {
    return mcp23x17_get_level(dev, pin, val);
}

esp_err_t gpio_compat_write(compat_gpio_dev_t *dev, uint8_t pin, uint32_t val) {
    return mcp23x17_set_level(dev, pin, val);
}

void gpio_compat_set_mode(compat_gpio_dev_t *dev, uint8_t pin, mcp23x17_gpio_mode_t mode) {
    mcp23x17_set_mode(dev, pin, mode);
}


void gpio_compat_set_pullup(compat_gpio_dev_t *dev, uint8_t pin, bool enable) {
    mcp23x17_set_pullup(dev, pin, enable);
}


void gpio_compat_set_interrupt(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_intr_t intr) {
    mcp23x17_set_interrupt(dev, pin, intr);
}


#endif

void gpio_compat_i2cScan(i2c_port_t i2cPort, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    i2c_config_t conf;
    esp_err_t ret;
    /* I2C config */
    memset(&conf, 0, sizeof(i2c_config_t));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_gpio;
    conf.scl_io_num = scl_gpio;
    conf.master.clk_speed = 100000;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    ret = i2c_param_config(i2cPort, &conf);
    if (ret != ESP_OK)
    {
        printf("I2C config failed");
    }
    printf("I2C config ok");

    /* Install driver */
    ret = i2c_driver_install(i2cPort, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK)
    {
        printf("I2C driver install failed");
    }
    printf("I2C driver install ok");

    uint8_t WRITE_BIT = I2C_MASTER_WRITE; /*!< I2C master write */
    // uint8_t READ_BIT = I2C_MASTER_READ;    /*!< I2C master read */
    uint8_t ACK_CHECK_EN = 0x1; /*!< I2C master will check ack from slave*/
    // uint8_t ACK_CHECK_DIS = 0x0;           /*!< I2C master will not check ack from slave */

    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16)
    {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++)
        {
            fflush(stdout);
            address = i + j;
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
            i2c_master_stop(cmd);
            esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(50));
            i2c_cmd_link_delete(cmd);
            if (ret == ESP_OK)
            {
                printf("%02x ", address);
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                printf("UU ");
            }
            else
            {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
    i2c_driver_delete(i2cPort);
}