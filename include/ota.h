/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/07/18 15:30
 * @LastEditors: l2q
 * @LastEditTime: 2025/07/18 15:30
 *
 */

#ifndef ESP_UTILITY_OTA_H
#define ESP_UTILITY_OTA_H
#ifdef ENABLE_OTA
#include "precompiled.h"

#include <ESP8266WebServer.h>
namespace esp_utility
{
class ota
{
    SINGLE_TPL(ota);

public:
    void begin(ESP8266WebServer& server);
    void update();
};
}
#endif // ENABLE_OTA
#endif  // ESP_UTILITY_OTA_H