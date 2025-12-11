#include "xmas_ui.h"

#include "../../common_define.h"

#define XMAS_FONT_SCALE 7
#define XMAS_FONT_SIZE 10

using tweeny::easing;

const int8_t XOFFSETS[] = {-20, -10, 10, 20};

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
                    cmdQueue = xQueueCreate(16 * 3, sizeof(UICommand));
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
                TickType_t animationStartTicks = xTaskGetTickCount();
                animationBkg = tweeny::from((uint8_t)0).to((uint8_t)0).during(100);
                int32_t currentXPosition = displayWidth / 2;
                int32_t yOffset = 0;
                uint8_t lastShelf = (currentSelection - 1) / 4;
                uint8_t currentShelf = (currentSelection - 1) / 4;
                while (true) {
                    vTaskDelay(pdMS_TO_TICKS(7)); // 144ish FPS
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
                                case SET_ERROR_FLAG:
                                    errorFlag = true;
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
                            displayOn(true);
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
                            encoder.wasMoved(true);
                            displayOff();
                        } else {
                            displayOn();
                            animationStartTicks = xTaskGetTickCount();
                            int32_t targetPosition = (displayWidth / 2) + XOFFSETS[(currentSelection - 1) % 4];
                            animationX = tweeny::from(currentXPosition).to(targetPosition).during(100).via(easing::elasticOut);
                            currentShelf = (currentSelection - 1) / 4;
                            if (currentShelf != lastShelf) {
                                int8_t direction = (currentShelf > lastShelf) ? 1 : -1;
                                animationY = tweeny::from((int32_t)20 * direction).to(0).during(100).via(easing::cubicInOut);
                                uint8_t bkgStart = animationBkg.peek();
                                animationBkg = tweeny::from(bkgStart).to((uint8_t)(20 * currentShelf)).during(100).via(easing::cubicInOut);
                                lastShelf = currentShelf;
                            }
                        }
                    }
                    if (!animationX.isFinished()) {
                        currentXPosition = animationX.step(_time_since_ms(animationStartTicks));
                        if (!animationY.isFinished())
                            yOffset = animationY.step(_time_since_ms(animationStartTicks));
                        uint8_t bkgValue = animationBkg.peek();
                        if (!animationBkg.isFinished())
                            bkgValue = animationBkg.step(_time_since_ms(animationStartTicks));
                        animationStartTicks = xTaskGetTickCount();
                        rgb_t backgroundColor  = hsv2rgb_raw({.h = 0, .s = 0, .v = bkgValue});
                        drawCenterString(std::to_string(currentSelection).c_str(), currentXPosition, yOffset, backgroundColor);
                    }
                    if(_time_since_ms(animationStartTicks) > 6'000) {
                        if (!idleDisplayOffTriggered) {
                            idleDisplayOffTriggered = true;
                            displayOff();
                        }
                    } else {
                        idleDisplayOffTriggered = false;
                    }
                    displayBrightnessAnimate();
                }
            }
            void UI::buttonPressed()
            {
                displayOn(true);
                showAnimation(
                    &XMASPIMAGE1, 
                    (displayWidth / 2) - (XMASPIMAGE1.width/2), \
                    (displayHeight / 2) - (XMASPIMAGE1.height/2), 
                    5,
                    3000,
                    0xFFFFFF
                );
                display.clear();
                displayOff();
            }
            void UI::drawCenterString(const char* string, int32_t x, int32_t yOffset)
            {
                UI::drawCenterString(string, x, yOffset, {.r = 0x00, .g = 0x00, .b = 0x00});
            }
            void UI::drawCenterString(const char* string, int32_t x, int32_t yOffset, rgb_t color)
            {
                canvas->clear();
                // background
                canvas->fillScreen(canvas->color565(color.r, color.g, color.b));
                canvas->setTextSize(XMAS_FONT_SCALE);
                canvas->setFont(&fonts::efontTomorrowNight10);
                int textHeight = canvas->fontHeight();
                int y = (((displayHeight - textHeight) / 2) - 5) + yOffset;
                // #525252 shadow
                canvas->setTextColor(canvas->color565(0x52, 0x52, 0x52));
                canvas->drawCenterString(string, x, y + 5);
                // #FFFDFE main text
                canvas->setTextColor(canvas->color565(0xFF, 0xFD, 0xFE));
                canvas->drawCenterString(string, x, y);
                if (errorFlag) {
                    canvas->setTextSize(XMAS_FONT_SCALE / 2);
                    canvas->setTextColor(canvas->color565(0xFF, 0x00, 0x00));
                    canvas->drawCenterString("!", displayWidth / 2, (displayHeight / 2) + 50);
                }
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

            void UI::displayOn(bool instant) {
                displayOnStart = xTaskGetTickCount();
                if(instant) {
                    display.setBrightness(128);
                } else {
                    displayBrightnessAnimation = tweeny::from((uint8_t)display.getBrightness()).to((uint8_t)128).during(250).via(easing::quarticInOut);
                }
            }
            void UI::displayOff() {
                displayOnStart = xTaskGetTickCount();
                displayBrightnessAnimation = tweeny::from((uint8_t)display.getBrightness()).to((uint8_t)0).during(250).via(easing::quarticInOut);
            }
            void UI::displayBrightnessAnimate() {
                if(!displayBrightnessAnimation.isFinished()) {
                    display.setBrightness(displayBrightnessAnimation.seek(std::min(displayBrightnessAnimation.duration(), _time_since_ms(displayOnStart))));
                } else if (display.getBrightness() < 5) {
                    display.clear();
                    display.setBrightness(0);
                }
            }
        } // namespace XMAS
    } // namespace USER_APP
} // namespace MOONCAKE