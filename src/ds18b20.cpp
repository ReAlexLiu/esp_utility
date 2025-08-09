#include "ds18b20.h"
#include "config.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <OneWire.h>


#define DS18B20_SCAN_TIME 2000

namespace esp_utility
{
OneWire           oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void              ds18b20::create()
{
}

void ds18b20::destory()
{
}

void ds18b20::begin(ESP8266WebServer& server)
{
    // Start up the library
    sensors.begin();

    // report parasite power requirements
    if (sensors.isParasitePowerMode())
    {
        Println("Parasite power is: ON");
    }
    else
    {
        Println("Parasite power is: OFF");
    }

    for (int i = 0; i != 4; ++i)
    {
        if (esp_utility::config::getInstance()._config.has_apps_config)
        {
            memcpy(_sensor_config[i].address, esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].address.bytes, 8);
            // 转换地址为字符串
            for (int k = 0; k < 8; k++)
            {
                sprintf(_sensor_config[i].address_str + (k * 2), "%02X", esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].address.bytes[k]);
            }
            _sensor_config[i].output_pin = esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].output_pin;
            //Println("config %s", _sensor_config[i].address_str);
        }
        else
        {
            if (!sensors.getAddress(_sensor_config[i].address, i))
            {
                Println("Unable to find address for Device %d", i);
                continue;
            }
            esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].address.size = 8;
            memcpy(esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].address.bytes, _sensor_config[i].address, 8);
            // 转换地址为字符串
            for (int k = 0; k < 8; k++)
            {
                sprintf(_sensor_config[i].address_str + (k * 2), "%02X", _sensor_config[i].address[k]);
            }
            _sensor_config[i].output_pin = 0;
          //  Println("memory %s", _sensor_config[i].address_str);
        }
        sensors.setResolution(_sensor_config[i].address, TEMPERATURE_PRECISION);
    }

    // 注册HTTP处理函数
    server.on("/ds18b20-data", HTTP_GET, [&]() {
        // 先扫描传感器更新数据
        String json = "[";
        for (int i = 0; i != 4; ++i)
        {
            json += "{\"id\":" + String(i + 1) + ",";
            json += "\"address\":\"" + String(_sensor_config[i].address_str) + "\",";
            json += "\"temperature\":" + String(_sensor_config[i].temperature) + ",";
            json += "\"outputPin\":" + String(_sensor_config[i].output_pin) + ",";
            json += "\"connected\":" + String(true);
            json += "},";
        }
        json.remove(json.length() - 1);
        json += "]";

        server.send(200, "application/json", json);
    });

    server.on("/ds18b20-save-config", HTTP_POST, [&]() {
        if (server.method() != HTTP_POST)
        {
            server.send(405, "text/plain", "方法不允许");
            return;
        }

        // 解析JSON数据
        JsonDocument         doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error)
        {
            server.send(400, "text/plain", "解析JSON失败");
            return;
        }
        Serial.println(server.arg("plain"));
        // 更新配置
        for (int i = 0; i < 4; i++)
        {
            String outputKey = "output-" + String(i + 1);
            if (doc.containsKey(outputKey))
            {
                const char* outputPin                                                                   = doc[outputKey];
                // 验证引脚是否有效
                _sensor_config[i].output_pin                                                           = atoi(outputPin);
                esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].output_pin = _sensor_config[i].output_pin;
            }
            for (int k = 0; k < 8; k++)
            {
                sprintf(_sensor_config[i].address_str + (k * 2), "%02X", esp_utility::config::getInstance()._config.apps_config.temperature_sensor[i].address.bytes[k]);
            }
            Println("config %s - %d", _sensor_config[i].address_str, _sensor_config[i].output_pin);
        }

        esp_utility::config::getInstance()._config.apps_config.temperature_sensor_count = 4;
        esp_utility::config::getInstance()._config.has_apps_config                      = true;
        esp_utility::config::getInstance().save();
        Println("save config: %d", esp_utility::config::getInstance()._config.has_apps_config);

        server.send(200, "text/plain", "配置保存成功");

        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        delay(100);
        ESP.restart();
    });

    _scan_sensor_timer.start();
}

// 2秒扫描异一次数据
void ds18b20::update()
{
    if (_scan_sensor_timer.hasExpired(DS18B20_SCAN_TIME))
    {
        _scan_sensor_timer.restart();

        sensors.requestTemperatures();

        for (int i = 0; i != 4; ++i)
        {
            float tempC = sensors.getTempC(_sensor_config[i].address);
            if (tempC == DEVICE_DISCONNECTED_C)
            {
               // Println("Error: Could not read temperature data");
                continue;
            }
            _sensor_config[i].temperature = tempC;
        }
    }
}
}  // namespace esp_utility