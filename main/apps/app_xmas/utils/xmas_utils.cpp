#include "xmas_utils.h"


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
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE