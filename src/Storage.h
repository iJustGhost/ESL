#pragma once
#include <Preferences.h>
#include "PriceTag.h"

// Everything that touches NVS lives here.
//   namespace "wifi":   ssid, pass
//   namespace "tag":    uuid, name, price, sprice, sale, slen
//   namespace "clock":  time
//   namespace "device": id, apikey
class Storage
{
public:
    // ---- product / tag data ----
    PriceTag loadTag()
    {
        PriceTag t;
        p_.begin("tag", false);
        t.uuid = p_.getString("uuid", "");
        t.name = p_.getString("name", "");
        t.price = p_.getString("price", "");
        t.salePrice = p_.getString("sprice", "");
        t.saleStart = p_.getUInt("sale", 0);
        t.saleLen = p_.getUInt("slen", 0);
        p_.end();
        return t;
    }

    void saveTag(const PriceTag &t)
    {
        p_.begin("tag", false);
        p_.putString("uuid", t.uuid);
        p_.putString("name", t.name);
        p_.putString("price", t.price);
        p_.putString("sprice", t.salePrice);
        p_.putUInt("sale", t.saleStart);
        p_.putUInt("slen", t.saleLen);
        p_.end();
    }

    // ---- WiFi ----
    bool loadWifi(String &ssid, String &pass)
    {
        p_.begin("wifi", false);
        ssid = p_.getString("ssid", "");
        pass = p_.getString("pass", "");
        p_.end();
        return ssid.length() > 0;
    }

    void saveWifi(const String &ssid, const String &pass)
    {
        p_.begin("wifi", false);
        p_.putString("ssid", ssid);
        p_.putString("pass", pass);
        p_.end();
    }

    // ---- clock ----
    uint32_t loadClock()
    {
        p_.begin("clock", false);
        uint32_t t = p_.getUInt("time", 0);
        p_.end();
        return t;
    }

    void saveClock(uint32_t t)
    {
        p_.begin("clock", false);
        p_.putUInt("time", t);
        p_.end();
    }

    // ---- device identity ----
    String loadId()
    {
        p_.begin("device", false);
        String s = p_.getString("id", "");
        p_.end();
        return s;
    }

    void saveId(const String &id)
    {
        p_.begin("device", false);
        p_.putString("id", id);
        p_.end();
    }

    // ---- API key (shared secret for the HTTP API) ----
    String loadApiKey()
    {
        p_.begin("device", false);
        String s = p_.getString("apikey", "");
        p_.end();
        return s;
    }

    void saveApiKey(const String &key)
    {
        p_.begin("device", false);
        p_.putString("apikey", key);
        p_.end();
    }

private:
    Preferences p_;
};
