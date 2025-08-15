#include "rtc_pcf8563.h"

//#ifdef ENABLE_RTC_PCF8563

#include "Config.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <pcf8563.h>
#include <time.h>


namespace esp_utility
{

void rtc_pcf8563::create()
{
}

void rtc_pcf8563::destory()
{
}

void rtc_pcf8563::begin(ESP8266WebServer& server)
{
    // ��ʼ��NTP�ͻ���
    timeClient.setPoolServerName(ntpServer.c_str());
    timeClient.begin();

    // ��ʼ��PCF8563 - ʹ��Wire�⣬Ĭ�ϵ�ַ0x51
    _pcf_detected = (rtc_pcf8563.begin() == 0);
    if (!_pcf_detected)
    {
        Serial.println("δ��⵽PCF8563ģ��!");
    }
    else
    {
        Serial.println("PCF8563ģ���ʼ���ɹ�");
        // ����ص�ѹ�ͱ�־
        if (rtc_pcf8563.status2() & PCF8563_VOL_LOW_MASK)
        {
            Serial.println("����: PCF8563��ص�ѹ�ͣ������޷�����ʱ��");
        }
    }

    server.on("/time-info", HTTP_GET, [&]() {
        // ����JSON��Ӧ
        JsonDocument doc;

        // ��ȡ��ǰʱ��
        if (_pcf_detected && rtc_pcf8563.isVaild())
        {
            RTC_Date now = rtc_pcf8563.getDateTime();
            doc["time"]  = String(now.hour) + ":" +
                          (now.minute < 10 ? "0" : "") + String(now.minute) + ":" +
                          (now.second < 10 ? "0" : "") + String(now.second);

            doc["date"] = String(now.year) + "��" +
                          String(now.month) + "��" +
                          String(now.day) + "�� " +
                          getWeekday(rtc_pcf8563.getDayOfWeek(now.day, now.month, now.year));
        }
        else
        {
            time_t     epochTime = timeClient.getEpochTime() + timezoneOffset * 3600;
            struct tm* timeinfo;
            timeinfo    = localtime(&epochTime);

            doc["time"] = String(timeinfo->tm_hour) + ":" +
                          (timeinfo->tm_min < 10 ? "0" : "") + String(timeinfo->tm_min) + ":" +
                          (timeinfo->tm_sec < 10 ? "0" : "") + String(timeinfo->tm_sec);

            doc["date"] = String(timeinfo->tm_year + 1900) + "��" +
                          String(timeinfo->tm_mon + 1) + "��" +
                          String(timeinfo->tm_mday) + "�� " +
                          getWeekday(timeinfo->tm_wday);
        }

        doc["source"]      = timeSource;
        doc["status"]      = timeStatus;
        doc["pcfDetected"] = _pcf_detected;
        doc["lastSync"]    = lastSyncTime;
        doc["ntpServer"]   = ntpServer;
        doc["timezone"]    = timezoneOffset;

        // ����JSON��Ӧ
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        server.send(200, "application/json", jsonResponse);
    });
    server.on("/sync-ntp", HTTP_POST, [&]() {
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error)
        {
            server.send(400, "application/json", "{\"success\": false, \"message\": \"��Ч����������\"}");
            return;
        }

        // ����NTP��������ʱ��
        if (doc.containsKey("ntpServer"))
        {
            ntpServer = doc["ntpServer"].as<String>();
            timeClient.setPoolServerName(ntpServer.c_str());
        }

        if (doc.containsKey("timezone"))
        {
            timezoneOffset = doc["timezone"].as<float>();
        }

        // ͬ��NTPʱ��
        timeStatus     = "syncing";
        timeSource     = "ͬ����...";

        bool   success = false;
        String message = "NTPͬ��ʧ��";

        // ǿ�Ƹ���NTPʱ��
        timeClient.forceUpdate();

        if (timeClient.getEpochTime() > 1600000000)
        {  // ����Ƿ��ȡ����Чʱ��
            time_t     epochTime = timeClient.getEpochTime() + timezoneOffset * 3600;
            struct tm* timeinfo  = localtime(&epochTime);

            // ����PCF8563
            if (_pcf_detected)
            {
                // ʹ�ÿ��ṩ��setDateTime��������ʱ��
                rtc_pcf8563.setDateTime(
                    timeinfo->tm_year + 1900,  // ��
                    timeinfo->tm_mon + 1,      // ��
                    timeinfo->tm_mday,         // ��
                    timeinfo->tm_hour,         // ʱ
                    timeinfo->tm_min,          // ��
                    timeinfo->tm_sec           // ��
                );

                // ͬ��ϵͳʱ�䵽RTC
                rtc_pcf8563.syncToRtc();
            }

            // ����״̬��Ϣ
            lastSyncTime = String(timeinfo->tm_year + 1900) + "-" +
                           (timeinfo->tm_mon + 1 < 10 ? "0" : "") + String(timeinfo->tm_mon + 1) + "-" +
                           (timeinfo->tm_mday < 10 ? "0" : "") + String(timeinfo->tm_mday) + " " +
                           (timeinfo->tm_hour < 10 ? "0" : "") + String(timeinfo->tm_hour) + ":" +
                           (timeinfo->tm_min < 10 ? "0" : "") + String(timeinfo->tm_min);

            timeSource = "NTP������";
            timeStatus = "synced";
            success    = true;
            message    = "NTPͬ���ɹ�";
        }

        server.send(200, "application/json", "{\"success\": " + String(success ? "true" : "false") + ", \"message\": \"" + message + "\"}");
    });
    server.on("/sync-browser", HTTP_POST, [&]() {
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error)
        {
            server.send(400, "application/json", "{\"success\": false, \"message\": \"��Ч����������\"}");
            return;
        }

        // ��ȡʱ������
        int year       = doc["year"];
        int month      = doc["month"];
        int day        = doc["day"];
        int hour       = doc["hour"];
        int minute     = doc["minute"];
        int second     = doc["second"];
        timezoneOffset = doc["timezone"];

        // ����PCF8563
        bool   success = false;
        String message = "ʱ��ͬ��ʧ��";

        if (_pcf_detected)
        {
            // ʹ�ÿ��RTC_Date�ṹ����ʱ��
            RTC_Date newDate(year, month, day, hour, minute, second);
            rtc_pcf8563.setDateTime(newDate);
            rtc_pcf8563.syncToRtc();

            // ����״̬��Ϣ
            lastSyncTime = String(year) + "-" +
                           (month < 10 ? "0" : "") + String(month) + "-" +
                           (day < 10 ? "0" : "") + String(day) + " " +
                           (hour < 10 ? "0" : "") + String(hour) + ":" +
                           (minute < 10 ? "0" : "") + String(minute);

            timeSource = "�����ʱ��";
            timeStatus = "synced";
            success    = true;
            message    = "ʱ��ͬ���ɹ�";
        }

        server.send(200, "application/json", "{\"success\": " + String(success ? "true" : "false") + ", \"message\": \"" + message + "\"}");
    });

    // ����������
    server.begin();
    Serial.println("�����������ɹ�");
}

void rtc_pcf8563::update()
{
    //timeClient.update();
}
}  // namespace esp_utility
//#endif // ENABLE_RTC_PCF8563
