#include "xmas_utils.h"
#include "../../common_define.h"

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            void Utils::drawCenterString(HAL::HAL* hal, const char* string)
            {
                LGFX_Sprite* canvas = hal->canvas;
                canvas->clear();
                canvas->setTextSize(5);
                canvas->setTextColor((uint32_t)0xF3E9D2);
                canvas->setFont(&fonts::efontCN_24);
                canvas->drawCenterString(string, hal->display.width() / 2, hal->display.height() / 2 - 60);
                canvas->pushSprite(0, 0);
                canvas->setTextSize(1);
            }

            bool Utils::checkButton(mcp23x17_t* dev, int pin_button)
            {
                esp_err_t ret;
                uint16_t intf;
                // Read the interrupt flags (INTF register)
                ret = read_reg_16(dev, MCP_REG_INTFA, &intf);
                if (ret != ESP_OK)
                {
                    printf("Failed to read interrupt flags: %s\n", esp_err_to_name(ret));
                    return false;
                }

                // Check if BUTTON_PIN caused an interrupt
                if (intf & (1 << pin_button))
                {
                    uint16_t intcap;

                    // Read the interrupt capture register (INTCAP) to get the latched state
                    ret = read_reg_16(dev, MCP_REG_INTCAPA, &intcap);
                    if (ret != ESP_OK)
                    {
                        printf("Failed to read interrupt capture register: %s\n", esp_err_to_name(ret));
                        return false;
                    }

                    // Clear level
                    uint32_t val;
                    mcp23x17_get_level(dev, pin_button, &val);

                    // Check if BUTTON_PIN was low (reversed)
                    if (!(intcap & (0 << pin_button)))
                    {
                        return true;
                    }
                }
                return false;
            }

            void Utils::drawImgFrame(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t frameToDraw, uint8_t x, uint8_t y)
            {
                frameToDraw = frameToDraw % image->framecount;
                int offX = frameToDraw * image->width;
                canvas->clear();
                canvas->fillScreen(0xFFFFFF);
                canvas->drawQoi((const uint8_t*)image->data, image->data_size, x, y, image->width, image->height, offX, 0);
                canvas->pushSprite(0, 0);
            }

            void Utils::showAnimation(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t x, uint8_t y, uint8_t fps, int animationTimeMs) {
                uint8_t totalFrames = image->framecount;
                uint8_t frameDurationMs = 1000 / fps;
                uint8_t totalAnimationFrames = animationTimeMs / frameDurationMs;

                for (uint8_t frame = 0; frame < totalAnimationFrames; frame++) {
                    uint8_t currentFrame = frame % totalFrames;
                    drawImgFrame(image, canvas, currentFrame, x, y);
                    delay(frameDurationMs);
                }
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE