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
// #ifdef ENABLE_RTC_PCF8563
#include "precompiled.h"

#include <ESP8266WebServer.h>
#include <pcf8563.h>
namespace esp_utility
{
class rtc_pcf8563
{
    SINGLE_TPL(rtc_pcf8563);

public:
    void begin(ESP8266WebServer& server);
    void update();

private:
    bool parse_and_set_time(String& time_buf);
    bool sync_time_from_ntp();
    void check_time_sync();

private:
    bool          _pcf_detected;
    PCF8563_Class _rtc;
    bool          _time_synced;
    String        _last_sync_time;
    unsigned long _last_sync_millis;
};
}
// #endif // ESP_UTILITY_RTC_PCF8563_H
#endif  // ESP_UTILITY_RTC_H