#include "wifi_config.h"

#include "config.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// 连接超时设置
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_AP_PREFIX "AK_"
#define WIFI_AP_PASSWD "liuqingquan"
namespace esp_utility
{
void       wifi_config::create()
{
}

void wifi_config::destory()
{
}

bool wifi_config::try_connect()
{
    if (config::getInstance()._config.has_wifi_config)
    {
        Println("try to connect wifi...");

        ElapsedTimer _esp;
        _esp.start();

        Println("wifi ssid: %s", config::getInstance()._config.wifi_config.ssid);
        Println("wifi passphrase: %s", config::getInstance()._config.wifi_config.passphrase);
        // 连接WiFi
        WiFi.begin(config::getInstance()._config.wifi_config.ssid,
                   config::getInstance()._config.wifi_config.passphrase);

        // 等待连接，超时则进入AP模式
        while (WiFi.status() != WL_CONNECTED && (!_esp.hasExpired(WIFI_CONNECT_TIMEOUT)))
        {
            delay(500);
            Print(".");
        }

        // 检查连接状态
        if (WiFi.status() == WL_CONNECTED)
        {
            Println("");
            Println("WiFi连接成功, IP地址: %s", WiFi.localIP().toString().c_str());
        }
    }
    return WiFi.status() == WL_CONNECTED;
}

void wifi_config::try_config()
{
    Println("try to start ap...");

    String address = WiFi.softAPmacAddress();
    address.replace(":", "");
    String ssid = WIFI_AP_PREFIX + address.substring(6);

    // 配置AP模式
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), WIFI_AP_PASSWD);

    Println("AP name: %s", ssid.c_str());
    Println("AP IP address: %s", WiFi.softAPIP().toString().c_str());

    // 启动 captive portal (强制门户)
    if (!MDNS.begin("Arkss"))
    {
        Println("MDNS start failed");
    }
    else
    {
        Println("MDNS start success");
    }
}

void wifi_config::begin(ESP8266WebServer& server)
{
    if (!try_connect())
    {
        try_config();
    }

    // 启动Web服务器（配网模式）
    server.on("/wifi-scan", [&]() {
        // 处理WiFi扫描请求
        Serial.println("扫描WiFi网络...");

        // 扫描网络
        int n = WiFi.scanNetworks(false, true);
        _scan_results.clear();

        if (n == 0)
        {
            server.send(200, "application/json", "[]");
        }
        else
        {
            String json = "[";
            for (int i = 0; i < n; ++i)
            {
                json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + ",\"encryption\":\"";

                // 确定加密类型
                switch (WiFi.encryptionType(i))
                {
                case ENC_TYPE_NONE:
                    json += "";
                    break;
                case ENC_TYPE_WEP:
                    json += "WEP";
                    break;
                case ENC_TYPE_TKIP:
                    json += "WPA";
                    break;
                case ENC_TYPE_CCMP:
                    json += "WPA2";
                    break;
                case ENC_TYPE_AUTO:
                    json += "WPA/WPA2";
                    break;
                default:
                    json += "unknown";
                    break;
                }
                json += "\"},";

                _scan_results.push_back(wifi_scan_result(WiFi.SSID(i), WiFi.RSSI(i), WiFi.encryptionType(i)));
            }
            json.remove(json.length() - 1);
            json += "]";

            server.send(200, "application/json", json);
        }

        // 清除扫描结果，为下一次扫描做准备
        WiFi.scanDelete();
    });

    server.on("/wifi-connect", [&]() {
        // 处理连接WiFi请求
        if (server.method() != HTTP_POST)
        {
            server.send(405, "text/plain", "Method Not Allowed");
            return;
        }

        // 解析请求体
        String               requestBody = server.arg("plain");
        JsonDocument  doc;
        DeserializationError error = deserializeJson(doc, requestBody);

        if (error)
        {
            server.send(400, "text/plain", "Bad Request");
            return;
        }

        // 提取SSID和密码
        String ssid     = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();

        if (ssid.isEmpty())
        {
            server.send(400, "text/plain", "SSID cannot be empty");
            return;
        }

        // 保存配置
        config::getInstance()._config.has_wifi_config = true;
        memcpy(config::getInstance()._config.wifi_config.ssid,ssid.c_str(), ssid.length());
        memcpy(config::getInstance()._config.wifi_config.passphrase, password.c_str(), password.length());

        if (config::getInstance().save())
        {
            WiFi.begin(config::getInstance()._config.wifi_config.ssid, config::getInstance()._config.wifi_config.passphrase);
            server.send(200, "application/json", "{\"status\":\"connecting\"}");
        }
        else
        {
            server.send(500, "text/plain", "Failed to save configuration");
        }
    });

    server.on("/wifi-connection-status", [&]() {
        // 处理连接状态请求
        String json = "{\"connected\":";
        json += (WiFi.status() == WL_CONNECTED);
        json += ",\"status\":";
        json += WiFi.status();
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/wifi-device-info", [&]() {
        // 处理设备信息请求
        String json;
        json+="{\"name\":\"ESP-config\",\"mac\":\"";
        json+=WiFi.macAddress();
        json+="\",\"version\":\"1.0.1\",\"connected\":";
        json+=(WiFi.status() == WL_CONNECTED);
        json+=",\"mode\":\"";
        switch (WiFi.getMode())
        {
        case WIFI_OFF:
            json+="WIFI_OFF";
            break;

        case WIFI_STA:
            json+="WIFI_STA";
            break;

        case WIFI_AP :
            json+="WIFI_AP";
            break;

        case WIFI_AP_STA:
            json+="WIFI_AP_STA";
            break;

        }
        json+="\"}";

        server.send(200, "application/json", json);
    });

    server.on("/wifi-success", [&]() {
        // 处理配置成功页面
        String json = "{\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        server.send(200, "application/json", json);
    });

    if (WiFi.getMode() == WIFI_AP_STA)
    {
        MDNS.addService("http", "tcp", 80);
    }
}

void wifi_config::update()
{
    if (WiFi.getMode() == WIFI_AP_STA)
    {
        // 定期处理mDNS请求
        MDNS.update();

        // 如果在AP模式下成功连接到WiFi，则重启
        if (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
        {
            ESP.restart();
        }
    }
}
}
