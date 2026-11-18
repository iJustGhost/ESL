#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Storage.h"

// Shared-secret check for the HTTP API. The key lives in NVS (device/apikey).
// No key (or one shorter than cfg::API_KEY_MIN_LEN) = nothing is authorised: the API fails closed.
class Auth
{
public:
    void begin(Storage &store)
    {
        key_ = store.loadApiKey();
        key_.trim();
    }

    bool configured() const { return key_.length() >= cfg::API_KEY_MIN_LEN; }

    // Constant-time comparison, so the response time doesn't reveal how much of a guess was right.
    bool allows(const String &presented) const
    {
        if (!configured())
            return false;
        size_t n = key_.length() > presented.length() ? key_.length() : presented.length();
        uint32_t diff = (uint32_t)(key_.length() ^ presented.length());
        for (size_t i = 0; i < n; i++)
        {
            uint8_t a = i < key_.length() ? (uint8_t)key_[i] : 0;
            uint8_t b = i < presented.length() ? (uint8_t)presented[i] : 0;
            diff |= (uint32_t)(a ^ b);
        }
        return diff == 0;
    }

private:
    String key_;
};
