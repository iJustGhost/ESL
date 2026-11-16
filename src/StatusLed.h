#pragma once
#include <Arduino.h>

// Non-blocking LED: set a mode, call tick() every loop.
class StatusLed
{
public:
    enum class Mode
    {
        Off,
        On,
        SlowBlink,
        FastBlink
    };

    void begin(int pin)
    {
        pin_ = pin;
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
    }

    void set(Mode m) { mode_ = m; }

    void tick()
    {
        uint32_t ms = millis();
        bool on = false;
        switch (mode_)
        {
        case Mode::Off:
            on = false;
            break;
        case Mode::On:
            on = true;
            break;
        case Mode::SlowBlink:
            on = (ms / 500) % 2; // 1 Hz
            break;
        case Mode::FastBlink:
            on = (ms / 100) % 2; // 5 Hz
            break;
        }
        digitalWrite(pin_, on ? HIGH : LOW);
    }

private:
    int pin_ = -1;
    Mode mode_ = Mode::Off;
};
