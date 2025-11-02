#include "../../../hal/hal.h"
#include <mcp23x17.h>
#include "xmas_img.h"


namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            class Utils {
                public:
                    static void drawCenterString(HAL::HAL* hal, const char *string);
                    static bool checkButton(mcp23x17_t* dev, int pin_button);
                    static void drawImgFrame(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t frameToDraw, uint8_t x, uint8_t y);
                    static void showAnimation(const xmas_img_t* image, LGFX_Sprite* canvas, uint8_t x, uint8_t y, uint8_t fps, int animationTimeMs);
            };
        }
    }
}