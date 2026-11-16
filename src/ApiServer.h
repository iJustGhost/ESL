#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <functional>
#include "Config.h"
#include "PriceTag.h"
#include "Storage.h"
#include "Clock.h"
#include "Identity.h"

// HTTP API (port 80). Accepts JSON bodies or form/query args. CORS enabled.
//   GET/POST/PUT /api/tag   uuid, name, price, sale_price, sale, sale_len  (partial updates OK)
//   GET/POST/PUT /api/time  time (epoch seconds)
//   GET          /api/id    {"device":"pricetag","id":"PT-...","ip":"..."}  (for scanners)
class ApiServer
{
public:
    ApiServer(PriceTag &tag, Storage &store, Clock &clock, Identity &identity,
              std::function<void()> onChange)
        : tag_(tag), store_(store), clock_(clock), identity_(identity), onChange_(onChange) {}

    void begin()
    {
        server_.on("/", HTTP_GET, [this]
                   { reply(200, "text/plain", "ESP32 price tag. /api/tag, /api/time, /api/id"); });
        server_.on("/api/tag", HTTP_GET, [this]
                   { tagGet(); });
        server_.on("/api/tag", HTTP_POST, [this]
                   { tagPost(); });
        server_.on("/api/tag", HTTP_PUT, [this]
                   { tagPost(); });
        server_.on("/api/time", HTTP_GET, [this]
                   { timeGet(); });
        server_.on("/api/time", HTTP_POST, [this]
                   { timePost(); });
        server_.on("/api/time", HTTP_PUT, [this]
                   { timePost(); });
        server_.on("/api/id", HTTP_GET, [this]
                   { idGet(); });
        server_.onNotFound([this]
                           {
            if (server_.method() == HTTP_OPTIONS) // CORS preflight
            {
                preflight();
                return;
            }
            reply(404, "application/json", "{\"error\":\"not found\"}"); });
        server_.begin();
    }

    void handle() { server_.handleClient(); }

private:
    // ---- /api/tag ----
    void tagGet()
    {
        JsonDocument d;
        d["id"] = identity_.id();
        d["uuid"] = tag_.uuid;
        d["name"] = tag_.name;
        d["price"] = tag_.price;
        d["sale_price"] = tag_.salePrice;
        d["sale"] = tag_.saleStart;
        d["sale_len"] = tag_.saleLen;
        sendJson(200, d);
    }

    void tagPost()
    {
        JsonDocument in;
        if (!readBody(in))
            return;

        PriceTag next = tag_;
        bool any = false;
        String v;

        if (field(in, "uuid", v))
        {
            v.trim();
            if (v.length() > cfg::MAX_UUID)
                return fail("uuid too long");
            next.uuid = v;
            any = true;
        }
        if (field(in, "name", v))
        {
            v.trim();
            if (v.length() > cfg::MAX_NAME)
                return fail("name too long");
            next.name = v;
            any = true;
        }
        if (field(in, "price", v))
        {
            v.trim();
            if (v.length() > cfg::MAX_PRICE)
                return fail("price too long");
            next.price = v;
            any = true;
        }
        if (field(in, "sale_price", v))
        {
            v.trim();
            if (v.length() > cfg::MAX_PRICE)
                return fail("sale_price too long");
            next.salePrice = v;
            any = true;
        }
        if (field(in, "sale", v))
        {
            if (!parseU32(v, next.saleStart))
                return fail("sale must be unix epoch seconds");
            any = true;
        }
        if (field(in, "sale_len", v))
        {
            if (!parseU32(v, next.saleLen))
                return fail("sale_len must be seconds");
            any = true;
        }
        if (!any)
            return fail("no fields provided");

        tag_ = next;
        store_.saveTag(tag_);
        onChange_();
        tagGet();
    }

    // ---- /api/time ----
    void timeGet()
    {
        JsonDocument d;
        d["time"] = (uint32_t)time(nullptr);
        d["ntp"] = clock_.ntpSynced(); // true once NTP has synced since boot
        sendJson(200, d);
    }

    void timePost()
    {
        JsonDocument in;
        if (!readBody(in))
            return;
        String v;
        uint32_t t = 0;
        if (!field(in, "time", v) || !parseU32(v, t) || t < cfg::VALID_TIME)
            return fail("time must be unix epoch seconds");
        clock_.set(t);
        onChange_();
        timeGet();
    }

    // ---- /api/id : cheap, for IP scanners ----
    void idGet()
    {
        JsonDocument d;
        d["device"] = "pricetag";
        d["id"] = identity_.id();
        d["ip"] = WiFi.localIP().toString();
        sendJson(200, d);
    }

    // ---- CORS ----
    void cors()
    {
        server_.sendHeader("Access-Control-Allow-Origin", cfg::CORS_ORIGIN);
        server_.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        server_.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server_.sendHeader("Access-Control-Allow-Private-Network", "true"); // Chrome: public page -> LAN device
        server_.sendHeader("Access-Control-Max-Age", "600");
    }

    void reply(int code, const char *type, const String &body)
    {
        cors(); // must come before send()
        server_.send(code, type, body);
    }

    void preflight()
    {
        cors();
        server_.send(204, "text/plain", "");
    }

    // ---- helpers ----
    bool readBody(JsonDocument &in)
    {
        if (server_.hasArg("plain") && deserializeJson(in, server_.arg("plain")))
        {
            fail("invalid json");
            return false;
        }
        return true;
    }

    // JSON body field first, then form/query arg.
    bool field(JsonDocument &in, const char *key, String &out)
    {
        JsonVariant v = in[key];
        if (!v.isNull())
        {
            out = v.as<String>();
            return true;
        }
        if (server_.hasArg(key))
        {
            out = server_.arg(key);
            return true;
        }
        return false;
    }

    static bool parseU32(String v, uint32_t &out)
    {
        v.trim();
        if (!v.length())
        {
            out = 0;
            return true;
        }
        if (v.length() > 10)
            return false;
        for (size_t i = 0; i < v.length(); i++)
            if (!isDigit(v[i]))
                return false;
        uint64_t n = strtoull(v.c_str(), nullptr, 10);
        if (n > 0xFFFFFFFFULL)
            return false;
        out = (uint32_t)n;
        return true;
    }

    void sendJson(int code, JsonDocument &d)
    {
        String out;
        serializeJson(d, out);
        reply(code, "application/json", out);
    }

    void fail(const char *msg)
    {
        JsonDocument d;
        d["error"] = msg;
        sendJson(400, d);
    }

    WebServer server_{80};
    PriceTag &tag_;
    Storage &store_;
    Clock &clock_;
    Identity &identity_;
    std::function<void()> onChange_;
};
