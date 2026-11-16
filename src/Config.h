#pragma once
#include <Arduino.h>

namespace cfg
{
    // Pins (SCK=18 and MOSI=23 are the ESP32 hardware SPI defaults)
    constexpr int TFT_CS = 5, TFT_DC = 2, TFT_RST = 4;
    constexpr uint32_t SPI_HZ = 40000000;

    // Status LED (external LED on a free GPIO, active high)
    constexpr int LED_PIN = 25;
    constexpr bool LED_ON_WHEN_SYNCED = false; // true = solid on once NTP has synced

    // Screen layout
    constexpr int SCREEN_W = 240;
    constexpr int HALF_H = 120;  // top half = name, bottom half = price
    constexpr int FOOTER_H = 24; // countdown strip at the bottom of the price half

    // Tag id label (top-left corner)
    constexpr uint16_t ID_FG = 0xBDF7; // light grey
    constexpr int ID_MARGIN = 4;

    // Scrolling
    constexpr int SCROLL_PX = 2, SCROLL_MS = 30, SCROLL_GAP = 40;

    // Colors (RGB565)
    constexpr uint16_t NAME_BG = 0x0010, NAME_FG = 0xFFFF;  // navy / white
    constexpr uint16_t PRICE_BG = 0xFFE0, PRICE_FG = 0x0000; // yellow / black
    constexpr uint16_t SALE_BG = 0xF800, SALE_FG = 0xFFFF;   // red / white

    // Limits
    constexpr size_t MAX_NAME = 96, MAX_PRICE = 24, MAX_UUID = 64;
    constexpr uint32_t VALID_TIME = 1700000000UL; // epoch below this = clock not set

    // Network
    constexpr const char *HOSTNAME = "pricetag"; // fallback only; normally derived from the tag id
    constexpr const char *NTP_1 = "pool.ntp.org";
    constexpr const char *NTP_2 = "time.google.com";

    // CORS: "*" lets any web page call the API. Use a specific origin such as
    // "http://192.168.1.10:5173" if you know where your dashboard is served from.
    constexpr const char *CORS_ORIGIN = "*";
} // namespace cfg
