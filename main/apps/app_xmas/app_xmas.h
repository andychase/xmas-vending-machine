
#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include <apa102.h>

// const unsigned long speed = 100;
// const byte brightness = 80;

// const byte DATAPIN1 = 5;
// const byte CLOCKPIN1 = 11;
// const byte DATAPIN2 = 12;
// const byte CLOCKPIN2 = 2;
// const byte NUM_STRIPS = 2;
// const size_t NUM_LEDS_PER_STRIP = 6*12; // needs to be a multiple of 6


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
            public:
                XMAS::Data_t _data;

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

