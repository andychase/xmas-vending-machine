
#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include "songs/songs.h"
#include <led_strip_spi.h>
#include "../../hal/arduino/Tone.h"
#include "utils/xmas_utils.h"

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            struct Data_t
            {
                HAL::HAL* hal = nullptr;
            };
        }

        class Xmas : public APP_BASE
        {
            private:
                const char* _tag = "XMAS";
                led_strip_spi_esp32_t led_strip;
                uint8_t hue = 0;
                uint8_t currentSong = 6;
                uint startCount = 0;
                uint currentSectionAmountTransitioned = 0;
                uint currentSection = 0;
            public:
                XMAS::Data_t _data;

                void playSong(int songId);

                /**
                 * @brief Lifecycle callbacks for derived to override
                 *
                 */
                void startLights();
                /* Setup App configs, called when App "install()" */
                void onSetup();

                /* Life cycle */
                void onCreate();
                void onRunning();
                void onDestroy();
        };
    }
}

