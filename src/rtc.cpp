#include "rtc.h"

// #ifdef ENABLE_RTC

#include "Config.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <PCF8563.h>
#include <time.h>
#include <WiFiUdp.h>


namespace esp_utility
{
// NTP客户端设置
WiFiUDP       ntpUDP;
NTPClient     timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000 * 60 * 8);  // 默认UTC+8，10分钟同步一次

// PCF8563 RTC模块实例
PCF8563_Class pcf8563;

// 时间相关变量
String        ntpServer      = "pool.ntp.org";
float         timezoneOffset = 8.0;

String        lastSyncTime   = "从未";
String        timeSource     = "未同步";
String        timeStatus     = "未同步";

// 转换星期数字为中文
String        rtc::getWeekday(int weekday)
{
    // 注意：PCF8563中星期一是1，星期日是0；而tm_wday中星期日是0，星期六是6
    // 这里统一转换为星期日为0的格式处理
    if (weekday == 7)
    {
        weekday = 0;  // 处理可能的7值（有些库用7表示星期日）
    }

    switch (weekday)
    {
    case 0:
        return "星期日";
    case 1:
        return "星期一";
    case 2:
        return "星期二";
    case 3:
        return "星期三";
    case 4:
        return "星期四";
    case 5:
        return "星期五";
    case 6:
        return "星期六";
    default:
        return "未知";
    }
}

void rtc::create()
{
}

void rtc::destory()
{
}

void rtc::begin(ESP8266WebServer& server)
{  // 初始化NTP客户端
    timeClient.setPoolServerName(ntpServer.c_str());
    timeClient.begin();

    // 初始化PCF8563 - 使用Wire库，默认地址0x51
    _pcf_detected = (pcf8563.begin() == 0);
    if (!pcfDetected)
    {
        Serial.println("未检测到PCF8563模块!");
    }
    else
    {
        Serial.println("PCF8563模块初始化成功");
        // 检查电池电压低标志
        if (pcf8563.status2() & PCF8563_VOL_LOW_MASK)
        {
            Serial.println("警告: PCF8563电池电压低，可能无法保持时间");
        }
    }

    server.on("/time-info", HTTP_GET, [&]() {
        // 创建JSON响应
        JsonDocument doc;

        // 获取当前时间
        if (pcfDetected && pcf8563.isVaild())
        {
            RTC_Date now = pcf8563.getDateTime();
            doc["time"]  = String(now.hour) + ":" +
                          (now.minute < 10 ? "0" : "") + String(now.minute) + ":" +
                          (now.second < 10 ? "0" : "") + String(now.second);

            doc["date"] = String(now.year) + "年" +
                          String(now.month) + "月" +
                          String(now.day) + "日 " +
                          getWeekday(pcf8563.getDayOfWeek(now.day, now.month, now.year));
        }
        else
        {
            time_t     epochTime = timeClient.getEpochTime() + timezoneOffset * 3600;
            struct tm* timeinfo;
            timeinfo    = localtime(&epochTime);

            doc["time"] = String(timeinfo->tm_hour) + ":" +
                          (timeinfo->tm_min < 10 ? "0" : "") + String(timeinfo->tm_min) + ":" +
                          (timeinfo->tm_sec < 10 ? "0" : "") + String(timeinfo->tm_sec);

            doc["date"] = String(timeinfo->tm_year + 1900) + "年" +
                          String(timeinfo->tm_mon + 1) + "月" +
                          String(timeinfo->tm_mday) + "日 " +
                          getWeekday(timeinfo->tm_wday);
        }

        doc["source"]      = timeSource;
        doc["status"]      = timeStatus;
        doc["pcfDetected"] = pcfDetected;
        doc["lastSync"]    = lastSyncTime;
        doc["ntpServer"]   = ntpServer;
        doc["timezone"]    = timezoneOffset;

        // 发送JSON响应
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        server.send(200, "application/json", jsonResponse);
    });
    server.on("/sync-ntp", HTTP_POST, [&]() {
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error)
        {
            server.send(400, "application/json", "{\"success\": false, \"message\": \"无效的请求数据\"}");
            return;
        }

        // 更新NTP服务器和时区
        if (doc.containsKey("ntpServer"))
        {
            ntpServer = doc["ntpServer"].as<String>();
            timeClient.setPoolServerName(ntpServer.c_str());
        }

        if (doc.containsKey("timezone"))
        {
            timezoneOffset = doc["timezone"].as<float>();
        }

        // 同步NTP时间
        timeStatus     = "syncing";
        timeSource     = "同步中...";

        bool   success = false;
        String message = "NTP同步失败";

        // 强制更新NTP时间
        timeClient.forceUpdate();

        if (timeClient.getEpochTime() > 1600000000)
        {  // 检查是否获取到有效时间
            time_t     epochTime = timeClient.getEpochTime() + timezoneOffset * 3600;
            struct tm* timeinfo  = localtime(&epochTime);

            // 更新PCF8563
            if (pcfDetected)
            {
                // 使用库提供的setDateTime方法设置时间
                pcf8563.setDateTime(
                    timeinfo->tm_year + 1900,  // 年
                    timeinfo->tm_mon + 1,      // 月
                    timeinfo->tm_mday,         // 日
                    timeinfo->tm_hour,         // 时
                    timeinfo->tm_min,          // 分
                    timeinfo->tm_sec           // 秒
                );

                // 同步系统时间到RTC
                pcf8563.syncToRtc();
            }

            // 更新状态信息
            lastSyncTime = String(timeinfo->tm_year + 1900) + "-" +
                           (timeinfo->tm_mon + 1 < 10 ? "0" : "") + String(timeinfo->tm_mon + 1) + "-" +
                           (timeinfo->tm_mday < 10 ? "0" : "") + String(timeinfo->tm_mday) + " " +
                           (timeinfo->tm_hour < 10 ? "0" : "") + String(timeinfo->tm_hour) + ":" +
                           (timeinfo->tm_min < 10 ? "0" : "") + String(timeinfo->tm_min);

            timeSource = "NTP服务器";
            timeStatus = "synced";
            success    = true;
            message    = "NTP同步成功";
        }

        server.send(200, "application/json", "{\"success\": " + String(success ? "true" : "false") + ", \"message\": \"" + message + "\"}");
    });
    server.on("/sync-browser", HTTP_POST, [&]() {
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error)
        {
            server.send(400, "application/json", "{\"success\": false, \"message\": \"无效的请求数据\"}");
            return;
        }

        // 提取时间数据
        int year       = doc["year"];
        int month      = doc["month"];
        int day        = doc["day"];
        int hour       = doc["hour"];
        int minute     = doc["minute"];
        int second     = doc["second"];
        timezoneOffset = doc["timezone"];

        // 更新PCF8563
        bool   success = false;
        String message = "时间同步失败";

        if (pcfDetected)
        {
            // 使用库的RTC_Date结构设置时间
            RTC_Date newDate(year, month, day, hour, minute, second);
            pcf8563.setDateTime(newDate);
            pcf8563.syncToRtc();

            // 更新状态信息
            lastSyncTime = String(year) + "-" +
                           (month < 10 ? "0" : "") + String(month) + "-" +
                           (day < 10 ? "0" : "") + String(day) + " " +
                           (hour < 10 ? "0" : "") + String(hour) + ":" +
                           (minute < 10 ? "0" : "") + String(minute);

            timeSource = "浏览器时间";
            timeStatus = "synced";
            success    = true;
            message    = "时间同步成功";
        }

        server.send(200, "application/json", "{\"success\": " + String(success ? "true" : "false") + ", \"message\": \"" + message + "\"}");
    });

    // 启动服务器
    server.begin();
    Serial.println("服务器启动成功");
}

void rtc::update()
{
    timeClient.update();
}
}  // namespace esp_utility
// #endif // ENABLE_OTA
