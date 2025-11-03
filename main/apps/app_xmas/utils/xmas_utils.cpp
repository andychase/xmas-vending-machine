#include "xmas_utils.h"
#include "../../common_define.h"

#define XMAS_FONT_SCALE 7
#define XMAS_FONT_SIZE 10

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
                // #000000 background
                canvas->fillScreen(canvas->color565(0x00, 0x00, 0x00));
                canvas->setTextSize(XMAS_FONT_SCALE);
                canvas->setFont(&fonts::efontTomorrowNight10);
                int textHeight = canvas->fontHeight();
                int y = ((hal->display.height() - textHeight) / 2) - 5;
                // #525252 shadow
                canvas->setTextColor(canvas->color565(0x52, 0x52, 0x52));
                canvas->drawCenterString(string, (hal->display.width() / 2), y + 5);
                // #FFFDFE main text
                canvas->setTextColor(canvas->color565(0xFF, 0xFD, 0xFE));
                canvas->drawCenterString(string, (hal->display.width() / 2), y);

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

            void Utils::drawImgFrame(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t frameToDraw, uint8_t x, uint8_t y, int backgroundColor, float scaleX, float scaleY)
            {
                frameToDraw = frameToDraw % image->framecount;
                int offX = frameToDraw * image->width;
                canvas->clear();
                canvas->fillScreen(backgroundColor);
                canvas->drawQoi((const uint8_t*)image->data, image->data_size, x, y, image->width*scaleX, image->height*scaleY, offX*scaleX, 0, scaleX, scaleY);
                canvas->pushSprite(0, 0);
            }

            void Utils::showAnimation(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t x, uint8_t y, uint8_t fps, int animationTimeMs, int backgroundColor, float scaleX, float scaleY) {
                uint8_t totalFrames = image->framecount;
                uint8_t frameDurationMs = 1000 / fps;
                uint8_t totalAnimationFrames = animationTimeMs / frameDurationMs;

                for (uint8_t frame = 0; frame < totalAnimationFrames; frame++) {
                    uint8_t currentFrame = frame % totalFrames;
                    drawImgFrame(image, canvas, currentFrame, x, y, backgroundColor, scaleX, scaleY);
                    delay(frameDurationMs);
                }
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE