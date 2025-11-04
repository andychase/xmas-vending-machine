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
                hue = 0;
                rainbowTimeCounter = 0;
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
                // result.startA = (section % 4) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
                // result.endA = ((section % 4) + 1) * sectionSize + (sectionSize * 8 * std::floor(section / 4));
                // result.startB =
                //     (sectionSize * 8) - ((section % 4 + 1) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));
                // result.endB = (sectionSize * 8) - ((section % 4) * sectionSize) + (sectionSize * 8 * std::floor(section / 4));

                int totalSections = std::max(ledCount / std::max(sectionSize, 1), 1);
                int base = sectionSize * totalSections * std::floor(section / totalSections);

                int startA = (section % totalSections) * sectionSize + base;
                int endA = startA + sectionSize - 1;
                int startB = (sectionSize * totalSections) - ((section % totalSections + 1) * sectionSize) + base;
                int endB = startB + sectionSize - 1;

                // Clamp to ledCount
                result.startA = std::max(0, std::min(startA, (int)ledCount - 1));
                result.endA   = std::max(0, std::min(endA,   (int)ledCount - 1));
                result.startB = std::max(0, std::min(startB, (int)ledCount - 1));
                result.endB   = std::max(0, std::min(endB,   (int)ledCount - 1));
                
                return result;
            }

            void XmasLights::onRunningLights(int currentSelection)
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
                
                if (rainbowTimeCounter < 500)
                {
                    for (int i = 0; i < this->ledCount; i++)
                    {
                        led_strip_spi_set_pixel(&led_strip, i, {0, 0, 0});
                    }
                    LEDSectionStruct ledSectionStruct = calculateSections(currentSelection - 1, 18);
                    for (size_t i = ledSectionStruct.startA; i <= ledSectionStruct.endA; i++)
                    {
                        led_strip_spi_set_pixel(&led_strip, i, {100, 100, 100});
                    }
                    for (size_t i = ledSectionStruct.startB; i <= ledSectionStruct.endB; i++)
                    {
                        led_strip_spi_set_pixel(&led_strip, i, {100, 100, 100});
                    }
                    for (int i = 0; i < 4; i++)
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
                led_strip_spi_flush(&led_strip);
            }

        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE
