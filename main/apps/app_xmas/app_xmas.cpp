
#include "app_xmas.h"
#include "../common_define.h"
#define HELPER_TARGET_IS_ESP32
#include <led_strip_spi.h>

using namespace MOONCAKE::USER_APP;

#define LED_COUNT 15             // Number of LEDs in the strip
#define CLOCK_PIN 9              // GPIO for clock input (CI)
#define DATA_PIN 8               // GPIO for data input (DI)
#define SPI_CLOCK_SPEED_HZ 3000000 // SPI Clock speed (3 MHz)


void Xmas::onSetup()
{
    setAppName("Xmas");
    setAllowBgRunning(false);

    /* Copy default value */
    XMAS::Data_t default_data;
    _data = default_data;

    _data.hal = (HAL::HAL*)getUserData();
    
}


/* Life cycle */
void Xmas::onCreate()
{
    _log("onCreate");
    LGFX_Sprite* canvas = _data.hal->canvas;
    canvas->clear();
    canvas->setTextSize(1.5);
    canvas->setTextColor((uint32_t)0xF3E9D2);
    canvas->setFont(&fonts::efontCN_24);
    canvas->drawCenterString("XMAS", _data.hal->display.width() / 2, _data.hal->display.height() / 2 - 24);
    canvas->pushSprite(0, 0);
    canvas->setTextSize(1);

    esp_err_t ret;

    // Step 1: Install the driver
    ret = led_strip_spi_install();
    if (ret != ESP_OK) {
        printf("Failed to install SPI LED strip driver. Error: %d\n", ret);
        return;
    }

    // Step 2: Create and initialize the strip descriptor
    led_strip_spi_esp32_t strip = LED_STRIP_SPI_DEFAULT_ESP32();
    strip.length = LED_COUNT;  // Set the number of LEDs
    strip.mosi_io_num = DATA_PIN;   // Set the Data pin (DI)
    strip.sclk_io_num = CLOCK_PIN; // Set the Clock pin (CI)
    strip.max_transfer_sz = LED_STRIP_SPI_BUFFER_SIZE(LED_COUNT);
    strip.clock_speed_hz = SPI_CLOCK_SPEED_HZ;
    strip.host_device = SPI3_HOST;

    // Step 3: Initialize the LED strip
    ret = led_strip_spi_init((led_strip_spi_t*)&strip);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI LED strip. Error: %d\n", ret);
        return;
    }

    // Step 4: Set all LEDs to white
    rgb_t white_color = { .red = 255, .green = 255, .blue = 255 }; // RGB value for white
    ret = led_strip_spi_fill((led_strip_spi_t*)&strip, 0, LED_COUNT, white_color);
    if (ret != ESP_OK) {
        printf("Failed to set LED colors. Error: %d\n", ret);
        return;
    }

    // Step 5: Flush the buffer to apply the color
    ret = led_strip_spi_flush((led_strip_spi_t*)&strip);
    if (ret != ESP_OK) {
        printf("Failed to flush LED buffer. Error: %d\n", ret);
        return;
    }

    printf("LEDs initialized and set to white successfully.\n");


    
// #define NUMPIXELS 15 // Number of LEDs in strip

// // Here's how to control the LEDs from any two pins:
// #define DATAPIN    8
// #define CLOCKPIN   9
// Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
// // The last parameter is optional -- this is the color data order of the
// // DotStar strip, which has changed over time in different production runs.
// // Your code just uses R,G,B colors, the library then reassigns as needed.
// // Default is DOTSTAR_BRG, so change this if you have an earlier strip.

// // Hardware SPI is a little faster, but must be wired to specific pins
// // (Arduino Uno = pin 11 for data, 13 for clock, other boards are different).
// //Adafruit_DotStar strip(NUMPIXELS, DOTSTAR_BRG);


// strip.begin(); // Initialize pins for output
// strip.show();  // Turn all LEDs off ASAP

// // Runs 10 LEDs at a time along strip, cycling through red, green and blue.
// // This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.

// int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
// uint32_t color = 0xFF0000;      // 'On' color (starts red)

//     while (1) {

//     strip.setPixelColor(head, color); // 'On' pixel at head
//     strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
//     strip.show();                     // Refresh strip
//     delay(20);                        // Pause 20 milliseconds (~50 FPS)

//     if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
//         head = 0;                       //  Yes, reset head index to start
//         if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
//         color = 0xFF0000;             //   Yes, reset to red
//     }
//     if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
//     }
}


void Xmas::onRunning()
{
    
}


void Xmas::onDestroy()
{
    _log("onDestroy");

}
