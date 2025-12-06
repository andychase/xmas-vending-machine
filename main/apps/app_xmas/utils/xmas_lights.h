#pragma once
#include "../../../hal/hal.h"
#include "../../common_define.h"
#include <cstdint>
#include <cmath>
#include <led_strip_spi.h>
#include "tweeny/tweeny.h"

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            struct LEDSectionStruct
            {
                int startA;
                int endA;
                int startB;
                int endB;
            };

            struct LEDSectionsAnimationStruct
            {
                tweeny::tween<int> startA;
                tweeny::tween<int> endA;
                tweeny::tween<int> startB;
                tweeny::tween<int> endB;
            };

            class XmasLights
            {
            public:
                XmasLights(uint16_t ledCount);
                void startLights(uint8_t clockPin, uint8_t dataPin, uint64_t spiClockSpeedHz, spi_host_device_t hostDevice);
                void onRunningLights(int currentSelection);
                rgb_t color_wheel(uint8_t pos, float gamma);
                LEDSectionStruct calculateSections(int section, int sectionSize);
                uint16_t rainbowTimeCounter = 0;
                LEDSectionsAnimationStruct animationSections;
                bool neverAnimated = true;
                int lastSelection = 1;
                TickType_t lastTick = 0;
            private:
                int hue = 0;
                uint16_t ledCount = 0;
                
                led_strip_spi_t led_strip;
            };
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
