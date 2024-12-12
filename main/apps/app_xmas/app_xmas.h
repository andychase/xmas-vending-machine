
#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include "songs/songs.h"
#include <apa102.h>
#include "../../hal/arduino/Tone.h"

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
                apa102_spi_device_t led_strip_device;
                uint8_t hue = 0;
                uint8_t currentSong = 0;
                uint currentSection = 0;
                uint currentSectionAmountTransitioned = 0;
            public:
                XMAS::Data_t _data;

                void playSong(int songId);

                /**
                 * @brief Lifecycle callbacks for derived to override
                 *
                 */
                /* Setup App configs, called when App "install()" */
                void onSetup();

                /* Life cycle */
                void onCreate();
                void onRunning();
                void onDestroy();
        };
    }
}

