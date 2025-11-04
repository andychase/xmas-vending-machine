#pragma once
#include "../../common_define.h"
#include <cstdint>
#include <cmath>
#include <led_strip_spi.h>

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

            class XmasLights
            {
            public:
                XmasLights(uint16_t ledCount);
                void startLights(uint8_t clockPin, uint8_t dataPin, uint64_t spiClockSpeedHz, spi_host_device_t hostDevice);
                void onRunningLights(int currentSelection);
                rgb_t color_wheel(uint8_t pos, float gamma);
                LEDSectionStruct calculateSections(int section, int sectionSize);
                uint16_t rainbowTimeCounter = 0;

            private:
                int hue = 0;
                uint16_t ledCount = 0;
                
                led_strip_spi_t led_strip;
            };
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
