#include <Arduino.h>
#include "Config.h"
#include "PriceTag.h"
#include "Storage.h"
#include "Identity.h"
#include "Auth.h"
#include "Clock.h"
#include "WifiLink.h"
#include "Display.h"
#include "StatusLed.h"
#include "ApiServer.h"
#include "SerialConsole.h"

void refreshDisplay();

Storage storage;
Identity identity;
Auth auth;
PriceTag tag;
Clock rtc(storage);
WifiLink wifi;
Display display;
StatusLed led;
ApiServer api(tag, storage, rtc, identity, auth, refreshDisplay);
SerialConsole serialConsole(storage, identity, wifi);

// Turns the current state into something the display can draw.
View buildView()
{
    View v;
    v.tagId = identity.id();
    if (!tag.configured())
    { // nothing flashed/posted yet: show connection info
        v.title = "Waiting for data";
        v.price = wifi.status();
        v.compact = true;
        return v;
    }
    uint32_t now = rtc.now();
    SalePhase phase = tag.phase(now);
    v.title = tag.name;
    v.price = tag.shownPrice(phase);
    v.footer = tag.footer(phase, now);
    v.onSale = (phase == SalePhase::Active);
    return v;
}

void refreshDisplay() { display.show(buildView()); }

// Fast blink = clock not set, slow blink = manual/NVS time only, off = NTP synced.
void updateLed()
{
    using Mode = StatusLed::Mode;
    switch (rtc.state())
    {
    case ClockState::NotSet:
        led.set(Mode::FastBlink);
        break;
    case ClockState::Manual:
        led.set(Mode::SlowBlink);
        break;
    case ClockState::Synced:
        led.set(cfg::LED_ON_WHEN_SYNCED ? Mode::On : Mode::Off);
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    if (!display.begin())
    {
        Serial.println("display canvas alloc failed");
        while (true)
            delay(1000);
    }

    tag = storage.loadTag();
    identity.begin(storage);       // before WiFi: the hostname is derived from the id
    auth.begin(storage);           // API key from NVS (device/apikey)
    rtc.begin();                   // seeds from NVS, then NTP takes over when it can
    led.begin(cfg::LED_PIN);
    wifi.begin(storage, identity); // non-blocking
    api.begin();
    updateLed();
    refreshDisplay();
}

void loop()
{
    api.handle();
    serialConsole.handle();
    display.tick(); // scrolling
    led.tick();

    static uint32_t last = 0;
    if (millis() - last >= 1000)
    {
        last = millis();
        wifi.service();
        updateLed();
        refreshDisplay(); // countdown tick + sale phase changes + status text
    }
}
