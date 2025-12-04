#include "xmas_ui.h"
#include "../../common_define.h"

#define XMAS_FONT_SCALE 7
#define XMAS_FONT_SIZE 10

static void ui_task_trampoline(void* pv)
{
    auto* inst = static_cast<MOONCAKE::USER_APP::XMAS::UI*>(pv);
    inst->run_task_loop();
    vTaskDelete(nullptr);
}

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            UI::UI(HAL::HAL* hal)
            {
                canvas = hal->canvas;
                display = hal->display;
                encoder = hal->encoder;
                displayHeight = hal->display.height();
                displayWidth = hal->display.width();
                // Just used to track which buttons are latched
                buttons = XmasButtons();
                if (!cmdQueue) {
                    cmdQueue = xQueueCreate(16, sizeof(UICommand));
                }
                if (!s_uiTask) {
                    xTaskCreate(ui_task_trampoline, "ui_task_runner", 4096, this, 5, &s_uiTask);
                }
            }
            void UI::sendCommand(UICommand cmd) {
                xQueueSend(cmdQueue, &cmd, portMAX_DELAY);
            }
            void UI::run_task_loop()
            {
                    while (true) {
                        vTaskDelay(pdMS_TO_TICKS(17)); // 60ish FPS
                        if (cmdQueue) {
                            UICommand cmd;
                            while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) {
                                switch (cmd.command) {
                                    case UI_BUTTON_PRESSED:
                                        buttonPressed();
                                        break;
                                    case UPDATE_LATCH_STATE:
                                        buttons.updateLatchState(cmd.latchIndex, cmd.latchIsClosed);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    if (encoder.wasMoved(true)) {
                        // Get number of active sensedPinState pins
                        currentSelection = buttons.getCurrentSelection(encoder.getCount() / 2);
                        u_int8_t numberSensed = buttons.numberOfClosedLatches();
                        if (numberSensed == 0) {
                                display.setBrightness(128);
                                showAnimation(
                                    &XMASPIMAGE2, 
                                    (displayWidth / 2) - ((XMASPIMAGE2.width/2)*4),
                                    (displayHeight / 2) - ((XMASPIMAGE2.height/2)*4), 
                                    20,
                                    3000,
                                    0x000000,
                                    4.0f, 
                                    4.0f
                            );
                            display.setBrightness(0);
                            encoder.wasMoved(true);
                        } else {
                            display.setBrightness(128);
                            drawCenterString(std::to_string(currentSelection).c_str());
                        }
                    }
                }
            }
            void UI::buttonPressed()
            {
                showAnimation(
                    &XMASPIMAGE1, 
                    (displayWidth / 2) - (XMASPIMAGE1.width/2), \
                    (displayHeight / 2) - (XMASPIMAGE1.height/2), 
                    5,
                    3000,
                    0xFFFFFF
                );
                drawCenterString(std::to_string(currentSelection).c_str());
            }
            void UI::drawCenterString(const char* string)
            {
                canvas->clear();
                // #000000 background
                canvas->fillScreen(canvas->color565(0x00, 0x00, 0x00));
                canvas->setTextSize(XMAS_FONT_SCALE);
                canvas->setFont(&fonts::efontTomorrowNight10);
                int textHeight = canvas->fontHeight();
                int y = ((displayHeight - textHeight) / 2) - 5;
                // #525252 shadow
                canvas->setTextColor(canvas->color565(0x52, 0x52, 0x52));
                canvas->drawCenterString(string, (displayWidth / 2), y + 5);
                // #FFFDFE main text
                canvas->setTextColor(canvas->color565(0xFF, 0xFD, 0xFE));
                canvas->drawCenterString(string, (displayWidth / 2), y);

                canvas->pushSprite(0, 0);
                canvas->setTextSize(1);
            }

            void UI::drawImgFrame(const xmas_img_t* image, uint8_t frameToDraw, uint8_t x, uint8_t y, int backgroundColor, float scaleX, float scaleY)
            {
                frameToDraw = frameToDraw % image->framecount;
                int offX = frameToDraw * image->width;
                canvas->clear();
                canvas->fillScreen(backgroundColor);
                canvas->drawQoi((const uint8_t*)image->data, image->data_size, x, y, image->width*scaleX, image->height*scaleY, offX*scaleX, 0, scaleX, scaleY);
                canvas->pushSprite(0, 0);
            }

            void UI::showAnimation(const xmas_img_t* image, uint8_t x, uint8_t y, uint8_t fps, int animationTimeMs, int backgroundColor, float scaleX, float scaleY) {
                uint8_t totalFrames = image->framecount;
                uint8_t frameDurationMs = 1000 / fps;
                uint8_t totalAnimationFrames = animationTimeMs / frameDurationMs;

                for (uint8_t frame = 0; frame < totalAnimationFrames; frame++) {
                    uint8_t currentFrame = frame % totalFrames;
                    drawImgFrame(image, currentFrame, x, y, backgroundColor, scaleX, scaleY);
                    delay(frameDurationMs);
                }
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE