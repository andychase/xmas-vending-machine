#pragma once
#include <cstdint>
#include <i2cdev.h>

namespace MOONCAKE {
namespace USER_APP {
namespace XMAS {

// Define music commands in the XMAS namespace with a fixed underlying type.
enum Music_Command_t : uint8_t {
    NOP_COMMAND   = 0,
    STOP_ALL      = 1,
    STOP_MUSIC    = 2,
    STOP_MOTOR    = 3,
    START_MOTOR   = 4,

    // Reserved command values 4-9
    CMD_RESERVED_START = 5,
    CMD_RESERVED_END   = 9,

    // Playback commands
    PLAY_SONG_1  = 10,
    PLAY_SONG_2  = 11,
    PLAY_SONG_3  = 12,
    PLAY_SONG_4  = 13,
    PLAY_SONG_5  = 14,
    PLAY_SONG_6  = 15,
    PLAY_SONG_7  = 16,
    PLAY_SONG_8  = 17,
    PLAY_SONG_9  = 18,
    PLAY_SONG_10 = 19,
    PLAY_SONG_11 = 20,
    PLAY_SONG_12 = 21
};

class XmasSound {
public:
    XmasSound(
            i2c_port_t i2cPort, 
            gpio_num_t gpioSDA,
            gpio_num_t gpioSCL
    );
    void stopSound();
    void onRunning(uint8_t currentSelection);
    void startMotor();
    void stopMotor();
    void selectionSound(uint8_t currentSelection);
    void playSound(uint8_t selection);
    void onButtonPressed();
    uint8_t lastSelection = 1;

private:
    i2c_dev_t m_dev;
    TickType_t motorStarted;
    TickType_t buttonPressed;
    bool buttonSequenceRunning = false;
    bool motorRunning;
    bool m_initialized = false;
    uint8_t currentSong = 1;
};
} // namespace XMAS
} // namespace USER_APP
} // namespace MOONCAKE
