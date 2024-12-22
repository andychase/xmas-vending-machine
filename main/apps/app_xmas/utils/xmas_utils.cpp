#include "xmas_utils.h"

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            void Utils::drawCenterString(HAL::HAL* hal, const char *string) {
                    LGFX_Sprite *canvas = hal->canvas;
                    canvas->clear();
                    canvas->setTextSize(1.5);
                    canvas->setTextColor((uint32_t)0xF3E9D2);
                    canvas->setFont(&fonts::efontCN_24);
                    canvas->drawCenterString(string, hal->display.width() / 2, hal->display.height() / 2 - 24);
                    canvas->pushSprite(0, 0);
                    canvas->setTextSize(1);
            }
        }
    }
}