#include "xmas_sound.h"

#include "gpio_compat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "../../common_define.h"

#define MOTOR_RUN_DURATION_MS 5'000
#define MOTOR_START_DELAY_MS 5'000

using namespace MOONCAKE::USER_APP::XMAS;

static constexpr const char* TAG = "XmasSound";

// Default I2C parameters - adjust to your hardware wiring if needed
static constexpr uint8_t DEFAULT_I2C_ADDR = 0x30;
static constexpr TickType_t I2C_TICKS_TO_WAIT = pdMS_TO_TICKS(100);

XmasSound::XmasSound(i2c_port_t i2cPort, gpio_num_t gpioSDA, gpio_num_t gpioSCL) {
    // initialize compat device descriptor using provided helper
    // gpio_compat_init sets dev->port and other fields as shown in the prompt
    m_dev.port = I2C_NUM_1;
    m_dev.cfg.mode = I2C_MODE_MASTER;
    m_dev.cfg.sda_io_num = gpioSDA;
    m_dev.cfg.scl_io_num = gpioSCL;
    m_dev.cfg.sda_pullup_en = GPIO_PULLUP_DISABLE;
    m_dev.cfg.scl_pullup_en = GPIO_PULLUP_DISABLE;
    m_dev.cfg.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    m_dev.addr = DEFAULT_I2C_ADDR;
    m_dev.mutex = NULL;
    m_dev.timeout_ticks = 0;
    m_dev.cfg.master.clk_speed = 100'000;
    m_initialized = true;
    ESP_LOGI(TAG, "XmasSound initialized (addr=0x%02x)", m_dev.addr);
}

// small helper to send a single byte command to the slave
static esp_err_t send_cmd(const i2c_dev_t &dev, uint8_t cmd) {
    // Use i2cdev wrapper for thread-safe device writes
    esp_err_t err = i2c_dev_write(&dev, NULL, 0, &cmd, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c write failed (addr=0x%02x): %d", dev.addr, err);
    }
    return err;
}

void XmasSound::stopSound() {
    send_cmd(m_dev, static_cast<uint8_t>(STOP_MUSIC));
}

void XmasSound::onRunning(uint8_t currentSelection) {
    selectionSound(currentSelection);
    if(motorRunning && _time_since_ms(motorStarted) > MOTOR_RUN_DURATION_MS) {
        stopMotor();
    }
    if (buttonSequenceRunning && _time_since_ms(buttonPressed) > MOTOR_START_DELAY_MS) {
        startMotor();
        buttonSequenceRunning = false;
    }
}

void XmasSound::startMotor() {
    send_cmd(m_dev, static_cast<uint8_t>(START_MOTOR));
    motorRunning = true;
    motorStarted = xTaskGetTickCount();
}

void XmasSound::stopMotor() {
    send_cmd(m_dev, static_cast<uint8_t>(STOP_MOTOR));
    motorStarted = 0;
    motorRunning = false;
}

void XmasSound::selectionSound(uint8_t currentSelection) {
    if (!buttonSequenceRunning) {
        if (currentSelection > lastSelection) {
            playSound(11);
        } else if (currentSelection < lastSelection) {
            playSound(12);
        }
    }
    lastSelection = currentSelection;
}

void XmasSound::playSound(uint8_t selection) {
    // selection is 1-based: selection 1 -> PLAY_SONG_1
    uint8_t cmd = static_cast<uint8_t>(PLAY_SONG_1) + (selection - 1);
    // ensure we don't overflow beyond PLAY_SONG_12
    if (cmd > static_cast<uint8_t>(PLAY_SONG_12)) {
        ESP_LOGW(TAG, "playSound selection out of range: %u", selection);
        return;
    }
    send_cmd(m_dev, cmd);
}

void XmasSound::onButtonPressed() {
    playSound(currentSong++);
    if (currentSong > 12)
        currentSong = 1;
    buttonSequenceRunning = true;
    buttonPressed = xTaskGetTickCount();
}