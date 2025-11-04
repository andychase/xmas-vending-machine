#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include "songs/songs.h"
#include <led_strip_spi.h>
#include "../../hal/arduino/Tone.h"
#include "utils/xmas_utils.h"
#include "utils/xmas_lights.h"

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
                uint8_t currentSong = 0;
                unsigned int startCount = 0;
                TickType_t lastButtonCheckTick = 0;
                unsigned int currentSelection = 1;
                bool sensedPinState[16] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true};
                class XMAS::XmasLights* lights = nullptr;
            public:
                XMAS::Data_t _data;

                void playSong(int songId);

                /**
                 * @brief Lifecycle callbacks for derived to override
                 *
                 */
                void startLights(); // will delegate to XmasLights
                /* Setup App configs, called when App "install()" */
                void onSetup();

                /* Life cycle */
                void onCreate();
                void onRunning();
                void scanButtons();
                void onRunningButtons();
                void onDestroy();
        };
    }
}

