#include "app_xmas.h"
#include "../common_define.h"


using namespace MOONCAKE::USER_APP;


void Xmas::onSetup() {
    setAppName("Xmas");
    setAllowBgRunning(false);

    /* Copy default value */
    XMAS::Data_t default_data;
    _data = default_data;

    _data.hal = (HAL::HAL *)getUserData();
}

/* Life cycle */
void Xmas::onCreate() {
    _log("onCreate");
    LGFX_Sprite *canvas = _data.hal->canvas;
    canvas->clear();
    canvas->setTextSize(1.5);
    canvas->setTextColor((uint32_t)0xF3E9D2);
    canvas->setFont(&fonts::efontCN_24);
    canvas->drawCenterString("XMAS", _data.hal->display.width() / 2, _data.hal->display.height() / 2 - 24);
    canvas->pushSprite(0, 0);
    canvas->setTextSize(1);
}

void Xmas::onRunning() {

    /* If button pressed */
    if (!_data.hal->encoder.btn.read()) {
        /* Hold until button release */
        while (!_data.hal->encoder.btn.read())
            delay(5);

        /* Bye */
        destroyApp();
    }
}

void Xmas::onDestroy() {
    _log("onDestroy");
}
