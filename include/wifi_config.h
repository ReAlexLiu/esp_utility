#ifndef ESP_UTILITY_WIFI_CONFIG_H
#define ESP_UTILITY_WIFI_CONFIG_H
#ifdef ENABLE_WIFI
#include "ElapsedTimer.h"
#include "precompiled.h"

#include <ESP8266WebServer.h>
#include <vector>

namespace esp_utility
{
class wifi_scan_result
{
public:
    String       ssid;
    int          rssi;
    unsigned int encryptionType;

    wifi_scan_result(String s, int r, unsigned int e)
        : ssid(s)
        , rssi(r)
        , encryptionType(e)
    {
    }
};

class wifi_config
{
    SINGLE_TPL(wifi_config);

public:
    void begin(ESP8266WebServer& server);
    void update();

private:
    bool try_connect();
    void try_config();

private:
    std::vector<wifi_scan_result> _scan_results;
};
}
#endif // ENABLE_WIFI
#endif  // ESP_UTILITY_WIFI_CONFIG_H
