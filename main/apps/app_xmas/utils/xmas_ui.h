#pragma once
#include "../../../hal/hal.h"
#include <mcp23x17.h>
#include "xmas_img.h"
#include "xmas_buttons.h"


namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            enum UI_COMMANDS {
                UI_BUTTON_PRESSED,
                UPDATE_LATCH_STATE
            };

            struct UICommand {
                UI_COMMANDS command;
                uint8_t latchIndex;
                bool latchIsClosed;
            };

            class UI {
                public:
                    UI(HAL::HAL* hal);
                    void sendCommand(UICommand cmd);
                    void run_task_loop();
                    void buttonPressed();
                    void drawCenterString(const char* string);
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
                TaskHandle_t s_uiTask = nullptr;
                QueueHandle_t cmdQueue = nullptr;
                XmasButtons buttons;
                uint8_t currentSelection = 1;
                LGFX_Sprite* canvas;
                ESP32Encoder encoder;
                LGFX_StampRing display;
                int32_t displayHeight;
                int32_t displayWidth;
            };  
        }
    }
}