#pragma once
#include <Arduino.h>

namespace cfg
{
    // Badge (top-right corner): New! / Promo! / Sale!
    constexpr uint16_t BADGE_NEW = 0x2BE6, BADGE_PROMO = 0xF3E0, BADGE_SALE = 0xD165; // green / orange / red
    constexpr uint16_t BADGE_FG = 0xFFFF;
    constexpr int BADGE_MARGIN = 4, BADGE_PAD_X = 8, BADGE_PAD_Y = 4;

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

    // CORS: off by default. The web UI's *server* talks to the tags, so no browser page needs it.
    // Only set an origin (e.g. "http://192.168.1.10:5173") if you call the tags from browser JS.
    constexpr const char *CORS_ORIGIN = "";

    // API auth: protected endpoints need this header to match the key stored in NVS (device/apikey).
    constexpr const char *AUTH_HEADER = "X-Tag-Key";
    constexpr size_t API_KEY_MIN_LEN = 16, API_KEY_MAX_LEN = 64;
} // namespace cfg
