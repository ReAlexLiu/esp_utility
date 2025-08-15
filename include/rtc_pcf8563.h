/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/08/14 14:13
 * @LastEditors: l2q
 * @LastEditTime: 2025/08/14 14:13
 *
 */

#ifndef ESP_UTILITY_RTC_PCF8563_H
#define ESP_UTILITY_RTC_PCF8563_H
//#ifdef ENABLE_RTC_PCF8563
#include "precompiled.h"

#include <ESP8266WebServer.h>
namespace esp_utility
{
class rtc_pcf8563
{
    SINGLE_TPL(rtc_pcf8563);

public:
    void begin(ESP8266WebServer& server);
    void update();
};
}
//#endif // ESP_UTILITY_RTC_PCF8563_H
#endif  // ESP_UTILITY_RTC_H