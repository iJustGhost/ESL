#pragma once
#include <WiFi.h>
#include <ArduinoJson.h>
#include "Storage.h"
#include "Identity.h"
#include "WifiLink.h"

// USB-serial provisioning: one JSON object per line, one JSON reply per line.
//   {"cmd":"info"}
//   {"cmd":"wifi","ssid":"My WiFi","pass":"secret"}   saves, then reboots
//   {"cmd":"id","id":"PT-12345678"}                    saves, then reboots
//   {"cmd":"apikey","key":"<16-64 chars>"}             saves, then reboots (never echoed back)
// Lines that don't start with '{' are ignored. Remove serialConsole from main.cpp to disable.
class SerialConsole
{
public:
    SerialConsole(Storage &store, Identity &identity, WifiLink &wifi)
        : store_(store), identity_(identity), wifi_(wifi) {}

    void handle()
    {
        while (Serial.available())
        {
            char c = (char)Serial.read();
            if (c == '\n')
            {
                run(line_);
                line_ = "";
            }
            else if (c != '\r' && line_.length() < 256)
            {
                line_ += c;
            }
        }
    }

private:
    void run(const String &line)
    {
        if (!line.length() || line[0] != '{')
            return;

        JsonDocument in;
        if (deserializeJson(in, line))
            return fail("invalid json");

        String cmd = in["cmd"] | "";
        if (cmd == "info")
            return info();

        if (cmd == "wifi")
        {
            String ssid = in["ssid"] | "";
            String pass = in["pass"] | "";
            if (!ssid.length() || ssid.length() > 32)
                return fail("ssid must be 1-32 chars");
            if (pass.length() > 63)
                return fail("pass too long");
            store_.saveWifi(ssid, pass);
            return okAndReboot();
        }

        if (cmd == "id")
        {
            String id = in["id"] | "";
            id.trim();
            if (!id.length() || id.length() > 24)
                return fail("id must be 1-24 chars");
            store_.saveId(id);
            return okAndReboot();
        }

        if (cmd == "apikey")
        {
            String key = in["key"] | "";
            if (key.length() < cfg::API_KEY_MIN_LEN || key.length() > cfg::API_KEY_MAX_LEN)
                return fail("key must be 16-64 chars");
            for (size_t i = 0; i < key.length(); i++)
                if (key[i] <= ' ' || key[i] > '~')
                    return fail("key must be printable ASCII without spaces");
            store_.saveApiKey(key);
            return okAndReboot();
        }

        fail("unknown cmd");
    }

    void info()
    {
        String ssid, pass; // pass is never sent back
        store_.loadWifi(ssid, pass);

        JsonDocument d;
        d["ok"] = true;
        d["id"] = identity_.id();
        d["ssid"] = ssid;
        d["apikey_set"] = store_.loadApiKey().length() >= cfg::API_KEY_MIN_LEN; // the key itself is never sent
        d["connected"] = wifi_.connected();
        d["ip"] = wifi_.connected() ? WiFi.localIP().toString() : "";
        send(d);
    }

    void okAndReboot()
    {
        JsonDocument d;
        d["ok"] = true;
        d["rebooting"] = true;
        send(d);
        Serial.flush();
        delay(200);
        ESP.restart();
    }

    void fail(const char *msg)
    {
        JsonDocument d;
        d["ok"] = false;
        d["error"] = msg;
        send(d);
    }

    void send(JsonDocument &d)
    {
        serializeJson(d, Serial);
        Serial.println();
    }

    Storage &store_;
    Identity &identity_;
    WifiLink &wifi_;
    String line_;
};
