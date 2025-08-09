#include "ota.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

namespace esp_utility
{
void ota::create()
{
}

void ota::destory()
{
}

void ota::begin(ESP8266WebServer& server)
{
    // 设置OTA更新
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else
        {
            // U_SPIFFS
            type = "filesystem";
        }
        Println("Start updating %s", type.c_str());
    });

    ArduinoOTA.onEnd([]() {
        Println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Print("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Print("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            Println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Println("End Failed");
        }
    });

    ArduinoOTA.begin();

    // 存储信息接口
    server.on("/ota-storage-info", HTTP_GET, [&]() {
        String json = "{";
        json += "\"total\":" + String(ESP.getFlashChipRealSize()) + ",";
        json += "\"used\":" + String(ESP.getFlashChipSize()) + ",";
        json += "\"free\":" + String(ESP.getFreeSketchSpace());
        json += "}";

        server.send(200, "application/json", json);
    });

    server.on("/ota-update", HTTP_POST, [&]() {
        server.sendHeader("Connection", "close");
        // 区分成功/失败的HTTP状态码，便于前端判断
        if (Update.hasError()) {
            server.send(500, "text/plain", "FAIL");
        } else {
            server.send(200, "text/plain", "OK");
        }
        // 延迟重启，确保响应完全发送
        delay(100);
        ESP.restart();
    }, [&]() {
                  HTTPUpload& upload = server.upload();
                  if (upload.status == UPLOAD_FILE_START) {
                      Serial.setDebugOutput(true);
                      WiFiUDP::stopAll();
                      Println("Update: %s", upload.filename.c_str());  // 保留自定义日志函数
                      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                      if (!Update.begin(maxSketchSpace)) {
                          Update.printError(Serial);
                          // 新增：上传开始失败时向前端反馈
                          server.send(500, "text/plain", "Update start failed");
                      }
                  } else if (upload.status == UPLOAD_FILE_WRITE) {
                      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                          Update.printError(Serial);
                          // ESP8266中使用Update.end(false)终止更新
                          Update.end(false); // 替代Update.abort()
                      }
                  } else if (upload.status == UPLOAD_FILE_END) {
                      if (Update.end(true)) {
                          Println("Update Success: %u\nRebooting...", upload.totalSize);  // 保留自定义日志函数
                      } else {
                          Update.printError(Serial);
                      }
                      Serial.setDebugOutput(false);
                  }
                  yield();
              });
}

void ota::update()
{
    ArduinoOTA.handle();
}
}  // namespace esp_utility