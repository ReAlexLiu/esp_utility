#include "wifi_config.h"

#ifdef ENABLE_WIFI
#include "config.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// ���ӳ�ʱ����
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_AP_PREFIX "AK_"
#define WIFI_AP_PASSWD "liuqingquan"
namespace esp_utility
{
void wifi_config::create()
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

        Println("wifi ssid: %s", reinterpret_cast<char *>(config::getInstance()._config.wifi_config.ssid.bytes));
        Println("wifi passphrase: %s", reinterpret_cast<char *>(config::getInstance()._config.wifi_config.passphrase.bytes));
        // ����WiFi
        WiFi.begin(reinterpret_cast<const char *>(config::getInstance()._config.wifi_config.ssid.bytes),
                   reinterpret_cast<const char *>(config::getInstance()._config.wifi_config.passphrase.bytes));

        // �ȴ����ӣ���ʱ�����APģʽ
        while (WiFi.status() != WL_CONNECTED && (!_esp.hasExpired(WIFI_CONNECT_TIMEOUT)))
        {
            delay(500);
            Print(".");
        }

        // �������״̬
        if (WiFi.status() == WL_CONNECTED)
        {
            Println("");
            Println("WiFi���ӳɹ�, IP��ַ: %s", WiFi.localIP().toString().c_str());
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

    // ����APģʽ
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), WIFI_AP_PASSWD);

    Println("AP name: %s", ssid.c_str());
    Println("AP IP address: %s", WiFi.softAPIP().toString().c_str());

    // ���� captive portal (ǿ���Ż�)
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

    // ����Web������������ģʽ��
    server.on("/wifi-scan", [&]() {
        // ����WiFiɨ������
        Serial.println("ɨ��WiFi����...");

        // ɨ������
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

                // ȷ����������
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

        // ���ɨ������Ϊ��һ��ɨ����׼��
        WiFi.scanDelete();
    });

    server.on("/wifi-connect", [&]() {
        // ��������WiFi����
        if (server.method() != HTTP_POST)
        {
            server.send(405, "text/plain", "Method Not Allowed");
            return;
        }

        // ����������
        String               requestBody = server.arg("plain");
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, requestBody);

        if (error)
        {
            server.send(400, "text/plain", "Bad Request");
            return;
        }

        // ��ȡSSID������
        String ssid     = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();

        if (ssid.isEmpty())
        {
            server.send(400, "text/plain", "SSID cannot be empty");
            return;
        }

        // ��������
        config::getInstance()._config.has_wifi_config       = true;
        config::getInstance()._config.wifi_config.ssid.size = ssid.length();
        memcpy(config::getInstance()._config.wifi_config.ssid.bytes, ssid.c_str(), config::getInstance()._config.wifi_config.ssid.size);
        config::getInstance()._config.wifi_config.passphrase.size = password.length();
        memcpy(config::getInstance()._config.wifi_config.passphrase.bytes, password.c_str(), config::getInstance()._config.wifi_config.passphrase.size);

        if (config::getInstance().save())
        {
            WiFi.begin(reinterpret_cast<const char *>(config::getInstance()._config.wifi_config.ssid.bytes),
           reinterpret_cast<const char *>(config::getInstance()._config.wifi_config.passphrase.bytes));
            server.send(200, "application/json", "{\"status\":\"connecting\"}");
        }
        else
        {
            server.send(500, "text/plain", "Failed to save configuration");
        }
    });

    server.on("/wifi-connection-status", [&]() {
        // ��������״̬����
        String json = "{\"connected\":";
        json += (WiFi.status() == WL_CONNECTED);
        json += ",\"status\":";
        json += WiFi.status();
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/wifi-device-info", [&]() {
        // �����豸��Ϣ����
        String json;
        json += "{\"name\":\"ESP-config\",\"mac\":\"";
        json += WiFi.macAddress();
        json += "\",\"version\":\"1.0.1\",\"connected\":";
        json += (WiFi.status() == WL_CONNECTED);
        json += ",\"mode\":\"";
        switch (WiFi.getMode())
        {
        case WIFI_OFF:
            json += "WIFI_OFF";
            break;

        case WIFI_STA:
            json += "WIFI_STA";
            break;

        case WIFI_AP:
            json += "WIFI_AP";
            break;

        case WIFI_AP_STA:
            json += "WIFI_AP_STA";
            break;
        }
        json += "\"}";

        server.send(200, "application/json", json);
    });

    server.on("/wifi-success", [&]() {
        // �������óɹ�ҳ��
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
        // ���ڴ���mDNS����
        MDNS.update();

        // �����APģʽ�³ɹ����ӵ�WiFi��������
        if (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
        {
            ESP.restart();
        }
    }
}
}
#endif  // ENABLE_WIFI
