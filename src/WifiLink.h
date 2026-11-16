#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Config.h"
#include "Storage.h"
#include "Identity.h"

// Connects with the credentials stored in NVS (no provisioning).
// The hostname (DHCP + mDNS) is derived from the tag id, e.g. "pt-48213907".
class WifiLink
{
public:
    void begin(Storage &store, const Identity &identity)
    {
        hostname_ = identity.hostname();
        id_ = identity.id();

        String ssid, pass;
        hasCreds_ = store.loadWifi(ssid, pass);

        WiFi.persistent(false);              // don't let the WiFi lib keep its own copy in flash
        WiFi.setHostname(hostname_.c_str()); // must be before WiFi.mode()
        WiFi.mode(WIFI_STA);                 // always init the stack so WebServer is safe to start
        WiFi.setAutoReconnect(true);
        if (hasCreds_)
            WiFi.begin(ssid.c_str(), pass.c_str());
    }

    bool connected() const { return WiFi.status() == WL_CONNECTED; }

    // Short text for the screen while nothing is configured.
    String status() const
    {
        if (connected())
            return WiFi.localIP().toString();
        return hasCreds_ ? "Connecting..." : "No WiFi config";
    }

    // Call about once a second.
    void service()
    {
        if (!mdnsUp_ && connected())
        {
            mdnsUp_ = MDNS.begin(hostname_.c_str());
            if (mdnsUp_)
            {
                MDNS.setInstanceName(id_);
                MDNS.addService("http", "tcp", 80);
                MDNS.addService("pricetag", "tcp", 80); // _pricetag._tcp, for discovery
                MDNS.addServiceTxt("pricetag", "tcp", "id", id_.c_str());
            }
        }
    }

private:
    String hostname_, id_;
    bool hasCreds_ = false;
    bool mdnsUp_ = false;
};
