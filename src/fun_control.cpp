#include "fun_control.h"

#include "ds18B20.h"

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define FUN_UPDATE_TIMER 2000

namespace esp_utility
{
void       fun_control::create()
{
    _remote_control_enabled = false;
}

void fun_control::destory()
{
}

/* PWM码表
    30	0%
    35	20%
    40	26%
    45	32%
    50	40%
    55	60%
    60	80%
    65	100%
*/
int get_pwm_value(float temperature)
{
    if (temperature <= 30.0)
    {
        return 0;
    }
    else if (temperature <= 35.0)
    {
        return 20;
    }
    else if (temperature <= 40.0)
    {
        return 26;
    }
    else if (temperature <= 45.0)
    {
        return 32;
    }
    else if (temperature <= 50.0)
    {
        return 40;
    }
    else if (temperature <= 55.0)
    {
        return 55;
    }
    else if (temperature <= 58.0)
    {
        return 70;
    }
    else if (temperature <= 60.0)
    {
        return 85;
    }

    return 100;
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
        int speed = get_pwm_value(ds18b20::getInstance()._sensor_config[i].temperature);

        if (_fan_speeds[i] == speed)
        {
            //Println("fun %d pin: %d,temp: %f, speed: %d%% same", i + 1,ds18b20::getInstance()._sensor_config[i].output_pin, ds18b20::getInstance()._sensor_config[i].temperature, speed);
            continue;
        }
        _fan_speeds[i] = speed;
        analogWrite(ds18b20::getInstance()._sensor_config[i].output_pin, speed);
        Println("fun %d pin: %d,temp: %f, speed: %d%% update", i + 1,ds18b20::getInstance()._sensor_config[i].output_pin, ds18b20::getInstance()._sensor_config[i].temperature, speed);

    }
}
}
