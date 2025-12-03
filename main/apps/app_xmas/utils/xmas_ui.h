#include "../../../hal/hal.h"
#include <mcp23x17.h>
#include "xmas_img.h"


namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            class UI {
                public:
                    UI(HAL::HAL* hal);
                    void drawCenterString(const char *string);
                    void drawImgFrame(const xmas_img_t* image,
                                      uint8_t frameToDraw,
                                      uint8_t x,
                                      uint8_t y,
                                      int backgroundColor,
                                    float scaleX = 1.0f,
                                    float scaleY = 1.0f);
                    void showAnimation(const xmas_img_t* image,
                                       uint8_t x,
                                       uint8_t y,
                                       uint8_t fps,
                                       int animationTimeMs,
                                       int backgroundColor,
                                       float scaleX = 1.0f,
                                       float scaleY = 1.0f);
            
            private:
                LGFX_Sprite* canvas;
                int32_t displayHeight;
                int32_t displayWidth;
            };  
        }
    }
}