#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Storage.h"

// The tag's own identity, e.g. "PT-48213907". Lives in NVS (device/id).
// If it's missing, empty or "0", a new one is generated and saved.
class Identity
{
public:
    void begin(Storage &store)
    {
        id_ = store.loadId();
        id_.trim();
        if (!id_.length() || id_ == "0")
        {
            id_ = generate();
            store.saveId(id_);
        }
    }

    const String &id() const { return id_; }

    // DNS-safe form of the id, e.g. "pt-48213907" (DHCP hostname + mDNS name).
    String hostname() const
    {
        String h = id_;
        h.toLowerCase();
        for (size_t i = 0; i < h.length(); i++)
        {
            char c = h[i];
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
                h[i] = '-';
        }
        return h.length() ? h : String(cfg::HOSTNAME);
    }

private:
    static String generate()
    {
        // 8 digits: hardware RNG mixed with the chip's unique MAC bytes.
        uint32_t chip = (uint32_t)(ESP.getEfuseMac() >> 24);
        uint32_t r = ((uint32_t)random(0, 90000000L) + chip) % 90000000UL;
        char b[16];
        snprintf(b, sizeof b, "PT-%lu", (unsigned long)(10000000UL + r));
        return String(b);
    }

    String id_;
};
