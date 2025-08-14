#include "fun_control.h"

#ifdef ENABLE_FAN
#include "ds18B20.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define FUN_UPDATE_TIMER 2000

namespace esp_utility
{
void fun_control::create()
{
    _remote_control_enabled = false;
}

void fun_control::destory()
{
}

int fun_control::adjust_speed(float current_temp)
{
    int speed_percent = 0;

    // 30~45度范围，转速5%~30%，每2度调整一次
    if (current_temp < 30.0)
    {
        speed_percent = std::max(0, static_cast<int>(5 - (30 - current_temp)));
    }
    else if (current_temp >= 30.0 && current_temp < 46.0)
    {
        // 计算与基准温度(30度)的差值
        int diff      = current_temp - 30;
        // 计算调整步数（每2度一步）
        int steps     = diff / 2;
        // 总步数：(45-30)/2 = 7步（30到44度），45度单独处理
        int max_steps = (45 - 30) / 2;
        // 转速范围（30% - 5%）
        int range     = 30 - 5;

        // 计算转速百分比
        speed_percent = 5 + (steps * range) / max_steps;
    }
    else if (current_temp >= 46.0 && current_temp < 60.0)
    {
        // 计算与基准温度(46度)的差值
        int diff      = current_temp - 46;
        // 计算调整步数（每2度一步）
        int steps     = diff / 2;
        // 总步数：(60-46)/2 = 7步（46到58度），60度单独处理
        int max_steps = (60 - 46) / 2;
        // 转速范围（99% - 31%）
        int range     = 99 - 31;

        // 计算转速百分比
        speed_percent = 31 + (steps * range) / max_steps;
    }
    else
    {
        speed_percent = 100;
    }

    return speed_percent;
}

// 初始化函数
void fun_control::begin(ESP8266WebServer& server)
{
    // 初始化风扇控制引脚
    pinMode(PWM_MOTOR1, OUTPUT);
    pinMode(PWM_MOTOR2, OUTPUT);
    pinMode(PWM_MOTOR3, OUTPUT);
    pinMode(PWM_MOTOR4, OUTPUT);

    // analogWriteFreq(1000);
    analogWriteFreq(100);
    analogWriteRange(100);

    server.on("/fun-control-states", HTTP_GET, [&]() {
        // 先扫描传感器更新数据
        String json;
        json += "{\"controlEnabled\":" + String(_remote_control_enabled) + ",";
        json += "\"fan1\":\"" + String(_fan_speeds[0]) + "\",";
        json += "\"fan2\":" + String(_fan_speeds[1]) + ",";
        json += "\"fan3\":" + String(_fan_speeds[2]) + ",";
        json += "\"fan4\":" + String(_fan_speeds[3]);
        json += "}";

        server.send(200, "application/json", json);
    });

    server.on("/fun-control-state", HTTP_POST, [&]() {
        if (!server.hasArg("plain"))
        {
            server.send(400, "application/json", "{\"error\":\"无效请求\"}");
            return;
        }

        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));

        _remote_control_enabled = doc["enabled"];

        if (!_remote_control_enabled)
        {
            Println("remote contorl disabled");
        }
        else
        {
            Println("remote contorl enabled");
        }

        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/fun-control-all", HTTP_POST, [&]() {
        // 只有远程控制启用时才响应
        if (!_remote_control_enabled)
        {
            server.send(403, "application/json", "{\"error\":\"远程控制已禁用\"}");
            return;
        }

        if (!server.hasArg("plain"))
        {
            server.send(400, "application/json", "{\"error\":\"无效请求\"}");
            return;
        }

        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));

        // 解析并更新所有风扇速度
        for (int i = 0; i < 4; i++)
        {
            String fanKey = "fan" + String(i + 1);
            if (!doc.containsKey(fanKey))
            {
                continue;
            }
            int speed = doc[fanKey];
            speed     = constrain(speed, 0, 100);  // 限制在0-100%
            if (_fan_speeds[i] == speed)
            {
                continue;
            }
            _fan_speeds[i] = speed;
            analogWrite(_fan_pins[i], speed);
            Println("fun %d speed: %d%%", i + 1, speed);
        }


        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });  // 合并请求的路由

    _update_timer.start();
}

// 主循环
void fun_control::update()
{
    if (_remote_control_enabled)
    {
        delay(1);
        return;
    }

    if (!_update_timer.hasExpired(FUN_UPDATE_TIMER))
    {
        delay(1);
        return;
    }

    _update_timer.restart();
    for (int i = 0; i < 4; i++)
    {
        int speed = adjust_speed(ds18b20::getInstance()._sensor_config[i].temperature);

        if (_fan_speeds[i] == speed)
        {
            // Println("fun %d pin: %d,temp: %f, speed: %d%% same", i + 1,ds18b20::getInstance()._sensor_config[i].output_pin, ds18b20::getInstance()._sensor_config[i].temperature, speed);
            continue;
        }
        _fan_speeds[i] = speed;
        analogWrite(ds18b20::getInstance()._sensor_config[i].output_pin, speed);
        Println("fun %d pin: %d,temp: %f, speed: %d%% update", i + 1, ds18b20::getInstance()._sensor_config[i].output_pin, ds18b20::getInstance()._sensor_config[i].temperature, speed);
    }
}
}
#endif // ENABLE_FAN
