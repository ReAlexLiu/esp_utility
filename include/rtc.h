/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/08/14 14:13
 * @LastEditors: l2q
 * @LastEditTime: 2025/08/14 14:13
 *
 */

#ifndef ESP_UTILITY_RTC_H
#define ESP_UTILITY_RTC_H
//#ifdef ENABLE_RTC
#include "precompiled.h"

#include <ESP8266WebServer.h>
namespace esp_utility
{
class rtc
{
    SINGLE_TPL(rtc);

public:
    void begin(ESP8266WebServer& server);
    void update();
private:
    String        getWeekday(int weekday);

private:
    bool          _pcf_detected;
};
}
//#endif // ENABLE_RTC
#endif  // ESP_UTILITY_RTC_H