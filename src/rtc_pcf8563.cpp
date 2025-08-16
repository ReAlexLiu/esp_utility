#include "rtc_pcf8563.h"

// #ifdef ENABLE_RTC_PCF8563

#include "Config.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <pcf8563.h>
#include <time.h>


namespace esp_utility
{
WiFiUDP             ntpUDP;
NTPClient           timeClient(ntpUDP);
// ��ʱͬ����� (24Сʱ����λ������)
const unsigned long SYNC_INTERVAL = 24 * 60 * 60 * 1000;

void                rtc_pcf8563::create()
{
    _time_synced = false;
}

void rtc_pcf8563::destory()
{
}

// ����ʱ���ַ��������õ�PCF8563
bool rtc_pcf8563::parse_and_set_time(String& time_buf)
{
    // ʱ���ʽ: "YYYY-MM-DD HH:MM:SS"
    int  year, month, day, hour, minute, second;
    char separator1, separator2, separator3, separator4;

    int  parsed = sscanf(time_buf.c_str(), "%d%c%d%c%d %d%c%d%c%d",
                         &year, &separator1,
                         &month, &separator2,
                         &day,
                         &hour, &separator3,
                         &minute, &separator4,
                         &second);

    if (parsed != 10)
    {
        return false;
    }

    // ���ָ���
    if (separator1 != '-' || separator2 != '-' ||
        separator3 != ':' || separator4 != ':')
    {
        return false;
    }

    // ����ʱ�䵽PCF8563
    if (_pcf_detected)
    {
        Println("����ʱ�䵽PCF8563: %d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);
        _rtc.setDateTime(year, month, day, hour, minute, second);
        _time_synced = true;
        return true;
    }

    return false;
}

void rtc_pcf8563::check_time_sync()
{
    if (millis() - _last_sync_millis >= SYNC_INTERVAL)
    {
        sync_time_from_ntp();
    }
}

bool rtc_pcf8563::sync_time_from_ntp()
{
    if (!WiFi.isConnected())
    {
        return false;
    }

    timeClient.forceUpdate();



    if (timeClient.getEpochTime() > 1600000000)
    {
        // ����Ƿ��ȡ����Чʱ��
        time_t     epochTime = timeClient.getEpochTime();
        struct tm* timeinfo  = localtime(&epochTime);

        // ����PCF8563
        if (_pcf_detected)
        {
            int year, month, day, hour, minute, second;
            year   = timeinfo->tm_year + 1900;  // ��
            month  = timeinfo->tm_mon + 1;      // ��
            day    = timeinfo->tm_mday;         // ��
            hour   = timeinfo->tm_hour;         // ʱ
            minute = timeinfo->tm_min;          // ��
            second = timeinfo->tm_sec;          // ��
            Println("����ʱ�䵽PCF8563: %d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);
            _rtc.setDateTime(year, month, day, hour, minute, second);

            _time_synced = true;

            // ͬ��ϵͳʱ�䵽RTC
            //_rtc.syncToRtc();
        }

        // ����״̬��Ϣ
        _last_sync_time = String(timeinfo->tm_year + 1900) + "-" +
                          (timeinfo->tm_mon + 1 < 10 ? "0" : "") + String(timeinfo->tm_mon + 1) + "-" +
                          (timeinfo->tm_mday < 10 ? "0" : "") + String(timeinfo->tm_mday) + " " +
                          (timeinfo->tm_hour < 10 ? "0" : "") + String(timeinfo->tm_hour) + ":" +
                          (timeinfo->tm_min < 10 ? "0" : "") + String(timeinfo->tm_min);

        return true;
    }
    return false;
}

String rtc_pcf8563::format_number(int number, int width)
{
    String result    = "";
    int    numDigits = 0;

    // �������ֵ�λ��
    int    temp      = number;
    if (temp == 0)
    {
        numDigits = 1;
    }
    else
    {
        while (temp != 0)
        {
            temp /= 10;
            numDigits++;
        }
    }

    // ����ǰ����
    for (int i = numDigits; i < width; i++)
    {
        result += "0";
    }

    // ������ֱ���
    result += String(number);

    return result;
}

String rtc_pcf8563::get_datetime()
{
    RTC_Date time = _rtc.getDateTime();
    return format_number(time.year, 4) + "-" +
           format_number(time.month) + "-" +
           format_number(time.day) + " " +
           format_number(time.hour) + ":" +
           format_number(time.minute) + ":" +
           format_number(time.second);
}

void rtc_pcf8563::begin(ESP8266WebServer& server)
{
    // ��ʼ��NTP�ͻ���
    if (!config::getInstance()._config.has_rtc_config)
    {
        config::getInstance()._config.has_rtc_config             = true;
        char ntp_server[]                                        = "ntp.aliyun.com";
        config::getInstance()._config.rtc_config.ntp_server.size = sizeof(ntp_server);
        memcpy(config::getInstance()._config.rtc_config.ntp_server.bytes, ntp_server, config::getInstance()._config.rtc_config.ntp_server.size);
        config::getInstance()._config.rtc_config.timezone = 8;
        config::getInstance().save();
    }
    timeClient.setTimeOffset(config::getInstance()._config.rtc_config.timezone * 3600);
    timeClient.setPoolServerName(reinterpret_cast<const char*>(config::getInstance()._config.rtc_config.ntp_server.bytes));
    timeClient.begin();

    // ��ʼ��PCF8563 - ʹ��Wire�⣬Ĭ�ϵ�ַ0x51
    _pcf_detected = (_rtc.begin() == 0);
    if (!_pcf_detected)
    {
        Serial.println("δ��⵽PCF8563ģ��!");
    }
    else
    {
        Serial.println("PCF8563ģ���ʼ���ɹ�");
        // ����ص�ѹ�ͱ�־
        if (_rtc.status2() & PCF8563_VOL_LOW_MASK)
        {
            Serial.println("����: PCF8563��ص�ѹ�ͣ������޷�����ʱ��");
            sync_time_from_ntp();
        }
    }

    server.on("/pcf8563-time-info", HTTP_GET, [&]() {
        String json = "{\"time\":\"";

        // ����PCF8563
        if (_pcf_detected)
        {
            json += get_datetime();
        }
        else
        {
            json += "1900-00-00 00:00:00";
        }

        json += "\",\"source\":";
        json += _time_synced ? "true" : "false";
        json += ",\"status\":true,\"pcfDetected\":";
        json += _pcf_detected;
        json += ",\"lastSync\":\"";
        json += _last_sync_time;
        json += "\",\"ntpServer\":\"";
        json += reinterpret_cast<const char*>(config::getInstance()._config.rtc_config.ntp_server.bytes);
        json += "\",\"timezone\":";
        json += config::getInstance()._config.rtc_config.timezone;
        json += "}";

        server.send(200, "application/json", json);
    });

    server.on("/pcf8563-sync-ntp", HTTP_POST, [&]() {
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
            String ntp_server                                        = doc["ntpServer"].as<String>();
            config::getInstance()._config.rtc_config.ntp_server.size = ntp_server.length();
            memcpy(config::getInstance()._config.rtc_config.ntp_server.bytes, ntp_server.c_str(), config::getInstance()._config.rtc_config.ntp_server.size);
        }

        if (doc.containsKey("timezone"))
        {
            config::getInstance()._config.rtc_config.timezone = doc["timezone"].as<float>();
            timeClient.setTimeOffset(config::getInstance()._config.rtc_config.timezone * 3600);
        }

        // ͬ��NTPʱ��
        bool   success = sync_time_from_ntp();
        String json    = "{\"status\": ";
        json += success ? "true" : "false";
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/pcf8563-sync-browser", HTTP_POST, [&]() {
        JsonDocument         doc;

        // ����������
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error)
        {
            server.send(400, "application/json", "{\"success\": false, \"message\": \"��Ч����������\"}");
            return;
        }

        String timeStr                                    = doc["time"];
        config::getInstance()._config.rtc_config.timezone = doc["timezone"];

        timeClient.setTimeOffset(config::getInstance()._config.rtc_config.timezone * 3600);

        bool success = false;

        if (_pcf_detected && parse_and_set_time(timeStr))
        {
            success           = true;
            _time_synced      = true;
            _last_sync_time   = timeStr;
            _last_sync_millis = millis();
        }

        String json = "{\"status\": ";
        json += success ? "true" : "false";
        json += "}";

        server.send(200, "application/json", json);
    });
}

void rtc_pcf8563::update()
{
    check_time_sync();
}
}  // namespace esp_utility
// #endif // ENABLE_RTC_PCF8563
