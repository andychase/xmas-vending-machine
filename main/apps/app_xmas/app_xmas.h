#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include "songs/songs.h"
#include <led_strip_spi.h>
#include "../../hal/arduino/Tone.h"
#include "utils/xmas_ui.h"
#include "utils/xmas_lights.h"
#include "utils/xmas_buttons.h"
#include "utils/xmas_sound.h"


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
                TickType_t lastLatchScanTick = 0;
                TickType_t buttonCheckCooldownTick = 0;
                bool startDelayPassed = false;
                bool releasingButtonNextLoop = false;
                unsigned int currentSelection = 1;
                unsigned int lastSelection = 1;
                uint8_t currentSong = 0;
                
                class XMAS::XmasLights* lights = nullptr;
                class XMAS::XmasButtons* buttons = nullptr;
                class XMAS::UI* ui = nullptr;
                class XMAS::XmasSound* sound = nullptr;
                void scanAndUpdateSelection(bool scanAll = false);
            public:
                XMAS::Data_t _data;

                void playSong(int songId);

                void setCurrentSelection();

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
                void onRunningButtons();
                void onDestroy();
        };
    }
}

