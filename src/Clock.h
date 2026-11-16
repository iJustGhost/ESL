#pragma once
#include <sys/time.h>
#include <esp_sntp.h>
#include "Config.h"
#include "Storage.h"

static volatile bool g_ntpSynced = false;
static void onNtpSync(struct timeval *) { g_ntpSynced = true; }

// NotSet: no valid time | Manual: valid (NVS/API) but no NTP yet | Synced: NTP has synced
enum class ClockState
{
    NotSet,
    Manual,
    Synced
};

// Time source: seeded from NVS at boot, overridden by NTP whenever it syncs,
// and settable manually (which also stores it in NVS).
class Clock
{
public:
    explicit Clock(Storage &store) : store_(store) {}

    void begin()
    {
        uint32_t t = store_.loadClock();
        if (t >= cfg::VALID_TIME)
            apply(t);
        sntp_set_time_sync_notification_cb(onNtpSync); // must be before configTime
        configTime(0, 0, cfg::NTP_1, cfg::NTP_2);
    }

    // Epoch seconds, or 0 if the clock hasn't been set yet.
    uint32_t now() const
    {
        time_t t = time(nullptr);
        return t >= (time_t)cfg::VALID_TIME ? (uint32_t)t : 0;
    }

    void set(uint32_t t)
    {
        apply(t);
        store_.saveClock(t);
    }

    bool ntpSynced() const { return g_ntpSynced; }

    ClockState state() const
    {
        if (g_ntpSynced)
            return ClockState::Synced;
        return now() ? ClockState::Manual : ClockState::NotSet;
    }

private:
    static void apply(uint32_t t)
    {
        struct timeval tv = {(time_t)t, 0};
        settimeofday(&tv, nullptr);
    }
    Storage &store_;
};
