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
                uint16_t startA;
                uint16_t endA;
                uint16_t startB;
                uint16_t endB;
            };

            struct LEDSectionsAnimationStruct
            {
                tweeny::tween<uint16_t> startA;
                tweeny::tween<uint16_t> endA;
                tweeny::tween<uint16_t> startB;
                tweeny::tween<uint16_t> endB;
            };

            class XmasLights
            {
            public:
                XmasLights(uint16_t ledCount);
                void startLights(uint8_t clockPin, uint8_t dataPin, uint64_t spiClockSpeedHz, spi_host_device_t hostDevice);
                void onRunningLights(int currentSelection);
                void clearEdgeLights();
                void clearLights();
                void colorRainbow();
                void blankSectionEdges(LEDSectionStruct& ledSectionStruct, uint8_t edgeSize);
                rgb_t color_wheel(uint8_t pos, float gamma);
                LEDSectionStruct calculateSections(int section, int sectionSize);
                LEDSectionsAnimationStruct animationSections;
                bool neverAnimated = true;
                int lastSelection = 1;
                TickType_t lastTick = 0;
                TickType_t idleTickCounter = 0;
                tweeny::tween<uint8_t> idleTween;
            private:
                int hue = 0;
                uint16_t ledCount = 0;
                
                led_strip_spi_t led_strip;
            };
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
