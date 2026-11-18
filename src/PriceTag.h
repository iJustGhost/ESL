#pragma once
#include <Arduino.h>
#include "Badge.h"

enum class SalePhase
{
    None,     // no sale configured
    NoClock,  // sale configured but the clock isn't set yet
    Upcoming, // before the sale starts: normal price + "SALE IN" countdown
    Active,   // sale running: sale price + "ENDS IN" countdown
    Ended     // sale over: normal price again
};

// The product data and the rules for which price/footer to show.
// `now` is epoch seconds, or 0 when the clock isn't valid.
struct PriceTag
{
    String uuid, name, price, salePrice;
    String badge; // "", "new", "promo" or "sale"
    uint32_t saleStart = 0; // epoch seconds the sale starts, 0 = none
    uint32_t saleLen = 0;   // sale duration in seconds, 0 = never ends

    bool configured() const { return name.length() > 0; }

    // New! and Promo! always show; Sale! only while the sale is running.
    Badge shownBadge(SalePhase p) const
    {
        if (badge == "new")
            return Badge::New;
        if (badge == "promo")
            return Badge::Promo;
        if (badge == "sale" && p == SalePhase::Active)
            return Badge::Sale;
        return Badge::None;
    }

    SalePhase phase(uint32_t now) const
    {
        if (!saleStart || !salePrice.length())
            return SalePhase::None;
        if (now == 0)
            return SalePhase::NoClock;
        if (now < saleStart)
            return SalePhase::Upcoming;
        if (saleLen && now >= (uint64_t)saleStart + saleLen)
            return SalePhase::Ended;
        return SalePhase::Active;
    }

    String shownPrice(SalePhase p) const
    {
        String s = (p == SalePhase::Active) ? salePrice : price;
        return s.length() ? s : String("--");
    }

    String footer(SalePhase p, uint32_t now) const
    {
        switch (p)
        {
        case SalePhase::NoClock:
            return "SYNCING CLOCK";
        case SalePhase::Upcoming:
            return countdown("SALE IN", (int64_t)saleStart - now);
        case SalePhase::Active:
            if (!saleLen)
                return "SALE NOW";
            return countdown("ENDS IN", (int64_t)saleStart + saleLen - now);
        default:
            return "";
        }
    }

private:
    static String countdown(const char *label, int64_t s)
    {
        if (s < 0)
            s = 0;
        char b[40];
        snprintf(b, sizeof b, "%s %lldd %02lld:%02lld:%02lld", label,
                s / 86400, (s % 86400) / 3600, (s % 3600) / 60, s % 60);
        return String(b);
    }
};