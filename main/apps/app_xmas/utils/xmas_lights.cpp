#include "xmas_lights.h"

static esp_err_t ret;

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {

            XmasLights::XmasLights(uint16_t ledCount)
            {
                this->ledCount = ledCount;
            }

            void XmasLights::startLights(uint8_t clockPin, uint8_t dataPin, uint64_t spiClockSpeedHz, spi_host_device_t hostDevice)
            {
                esp_err_t ret = led_strip_spi_install();
                if (ret != ESP_OK)
                {
                    printf("Failed to install SPI LED strip driver. Error: %d\n", ret);
                    return;
                }
                led_strip = LED_STRIP_SPI_DEFAULT_ESP32();
                led_strip.length = this->ledCount;
                led_strip.sclk_io_num = clockPin;
                led_strip.mosi_io_num = dataPin;
                led_strip.max_transfer_sz = LED_STRIP_SPI_BUFFER_SIZE(this->ledCount);
                led_strip.clock_speed_hz = spiClockSpeedHz;
                led_strip.host_device = hostDevice;
                ret = led_strip_spi_init((led_strip_spi_t*)&led_strip);
                if (ret != ESP_OK)
                {
                    printf("Failed to initialize SPI LED strip. Error: %d\n", ret);
                    return;
                }
                else
                {
                    printf("LED strip initialized successfully.\n");
                }
            }

            rgb_t XmasLights::color_wheel(uint8_t pos, float gamma)
            {
                pos = 255 - pos;
                rgb_t color;
                if (pos < 85)
                {
                    color.red = (uint8_t)(255 - pos * 3);
                    color.green = 0;
                    color.blue = (uint8_t)(pos * 3);
                }
                else if (pos < 170)
                {
                    pos -= 85;
                    color.red = 0;
                    color.green = (uint8_t)(pos * 3);
                    color.blue = (uint8_t)(255 - pos * 3);
                }
                else
                {
                    pos -= 170;
                    color.red = (uint8_t)(pos * 3);
                    color.green = (uint8_t)(255 - pos * 3);
                    color.blue = 0;
                }
                return apply_gamma2rgb(color, gamma);
            }

            LEDSectionStruct XmasLights::calculateSections(int section, int sectionSize)
            {
                LEDSectionStruct result;
                result.startA = (section % 4) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
                result.endA = ((section % 4) + 1) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
                result.startB =
                    (sectionSize * 8) - ((section % 4 + 1) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));
                result.endB = (sectionSize * 8) - ((section % 4) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));                
                return result;
            }

            void XmasLights::onRunningLights(int currentSelection, bool releasingButtonNextLoop)
            {
                if (lastSelection != currentSelection)
                {
                    lastTick = xTaskGetTickCount();
                    idleTickCounter = lastTick;
                    idleTween = (
                        tweeny::from((uint8_t) 255)
                        .to(255).during(5000)
                        .to(0).during(5000).via(tweeny::easing::quarticIn)
                        .to(0).during(1000)
                        .to(7).during(1)
                        .to(20).during(250).via(tweeny::easing::quadraticInOut)
                    );
                    LEDSectionStruct target = {
                        calculateSections(currentSelection - 1, 18).startA,
                        calculateSections(currentSelection - 1, 18).endA,
                        calculateSections(currentSelection - 1, 18).startB,
                        calculateSections(currentSelection - 1, 18).endB
                    };
                    animationSections = {
                        tweeny::from(neverAnimated ? target.startA : animationSections.startA.peek()).to(target.startA).during(50).via(tweeny::easing::linear),
                        tweeny::from(neverAnimated ? target.endA : animationSections.endA.peek()).to(target.endA).during(50).via(tweeny::easing::linear),
                        tweeny::from(neverAnimated ? target.startB : animationSections.startB.peek()).to(target.startB).during(50).via(tweeny::easing::linear),
                        tweeny::from(neverAnimated ? target.endB : animationSections.endB.peek()).to(target.endB).during(50).via(tweeny::easing::linear)
                    };
                    neverAnimated = false;
                    lastSelection = currentSelection;
                }
                if (!animationSections.startA.isFinished()) {
                    uint32_t timeElapsed = _time_since_ms(lastTick);
                    animationSections.startA.step(timeElapsed);
                    animationSections.endA.step(timeElapsed);
                    animationSections.startB.step(timeElapsed);
                    animationSections.endB.step(timeElapsed);
                }
                if (!idleTween.isFinished() && buttonReleaseTween.isFinished()) {
                    idleTween.step(_time_since_ms(lastTick));
                }
                if (releasingButtonNextLoop) {
                    buttonReleaseTween = (
                        tweeny::from((uint8_t) 255)
                        .to(100).during(250).via(tweeny::easing::quarticIn)
                        .to(100).during(3000).via(tweeny::easing::quarticIn)
                        .to(0).during(3000).via(tweeny::easing::quarticIn)
                    );
                    if (idleTween.duration() != 0)
                        idleTween.jump(2);
                }
                
                if (!buttonReleaseTween.isFinished()) {
                    clearLights();
                    lightsBrightness(buttonReleaseTween.step(_time_since_ms(lastTick)));
                }
                else if (idleTween.isFinished() || idleTween.point() > 1) {
                    clearLights();
                    uint8_t brightness = idleTween.peek();
                    if (idleTween.duration() == 0) {
                        brightness = 20;
                    }
                    colorShelvesLights(brightness);
                } else {
                    uint8_t brightness = idleTween.peek();
                    rgb_t color = {brightness, brightness, brightness};
                    clearLights();
                    LEDSectionStruct ledSectionStruct = {
                        animationSections.startA.peek(),
                        animationSections.endA.peek(),
                        animationSections.startB.peek(),
                        animationSections.endB.peek()
                    };
                    for (size_t i = ledSectionStruct.startA; i <= ledSectionStruct.endA; i++)
                    {
                        led_strip_spi_set_pixel(&led_strip, i, color);
                    }
                    for (size_t i = ledSectionStruct.startB; i <= ledSectionStruct.endB; i++)
                    {
                        led_strip_spi_set_pixel(&led_strip, i, color);
                    }
                    blankSectionEdges(ledSectionStruct, 4);
                }
                
                lastTick = xTaskGetTickCount();
                clearEdgeLights();
                led_strip_spi_flush(&led_strip);
        }

            void XmasLights::clearEdgeLights()
            {
                // Clears the edge lights under the staples
                for (int base = 0; base < this->ledCount; base += 144)
                {
                    if (base < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, base, {0, 0, 0});
                    }
                    if ((base + 71) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, base + 71, {0, 0, 0});
                    }
                    if ((base + 72) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, base + 72, {0, 0, 0});
                    }
                    if ((base + 143) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, base + 143, {0, 0, 0});
                    }
                }
            }

            void XmasLights::lightsBrightness(uint8_t brightness)
            {
                for (int i = 0; i < this->ledCount; i++)
                {
                    led_strip_spi_set_pixel_brightness(&led_strip, i, {brightness, brightness, brightness}, brightness);
                }
            }

            void XmasLights::clearLights()
            {
                for (int i = 0; i < this->ledCount; i++)
                {
                    led_strip_spi_set_pixel(&led_strip, i, {0, 0, 0});
                }
            }

            void XmasLights::colorRainbow()
            {
                for (int i = 0; i < this->ledCount; i++)
                {
                    rgb_t color = color_wheel((hue + (i * 256 / this->ledCount)) & 255, 1);
                    color.b *= 0.5;
                    color.g *= 0.5;
                    color.r *= 0.5;
                    led_strip_spi_set_pixel(&led_strip, i, color);
                }
                hue++;
            }

            void XmasLights::colorShelvesLights(uint8_t brightness)
            {
                constexpr uint8_t palSize = 5;
                const rgb_t palette_rgb[palSize] = {
                    {0, 255, 0},     // Green
                    {255, 0, 0},     // Red
                    {255, 255, 0},   // Yellow
                    {0, 0, 255},     // Blue
                    {255, 165, 0}    // Orange
                };

                for (int i = 0; i < this->ledCount; ++i)
                {
                    uint8_t index = std::min((uint8_t)254, (uint8_t)(((hue / 10) + (i * 256 / this->ledCount)) & 0xFF));
                    rgb_t color = color_from_palette_rgb(palette_rgb, palSize, index, brightness / 4, true);
                    led_strip_spi_set_pixel_brightness(&led_strip, i, color, brightness);
                }
                hue++;
            }

            void XmasLights::blankSectionEdges(MOONCAKE::USER_APP::XMAS::LEDSectionStruct& ledSectionStruct, uint8_t edgeSize)
            {
                for (int i = 0; i < edgeSize; i++)
                {
                    if ((ledSectionStruct.startA + i) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, ledSectionStruct.startA + i, {0, 0, 0});
                    }
                    if ((ledSectionStruct.endA - i) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, ledSectionStruct.endA - i, {0, 0, 0});
                    }
                    if ((ledSectionStruct.startB + i) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, ledSectionStruct.startB + i, {0, 0, 0});
                    }
                    if ((ledSectionStruct.endB - i) < this->ledCount)
                    {
                        led_strip_spi_set_pixel(&led_strip, ledSectionStruct.endB - i, {0, 0, 0});
                    }
                }
            }

        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
