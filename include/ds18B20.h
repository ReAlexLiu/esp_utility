/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/07/18 15:30
 * @LastEditors: l2q
 * @LastEditTime: 2025/07/18 15:30
 *
 */

#ifndef ESP_UTILITY_DS18B20_H
#define ESP_UTILITY_DS18B20_H
#include "precompiled.h"
#include "ElapsedTimer.h"

#include <DallasTemperature.h>

#include <ESP8266WebServer.h>
namespace esp_utility
{
struct SensorConfig
{
    DeviceAddress address;  // 存储DS18B20的64位地址，格式为16进制字符串
    char          address_str[17];
    uint8_t       output_pin;   // 存储输出引脚，如"GPIO2"
    float         temperature;  // 存储最新温度
};

class ds18b20
{
    SINGLE_TPL(ds18b20);

public:
    void begin(ESP8266WebServer& server);
    void update();

public:
    SensorConfig _sensor_config[4];

private:
    ElapsedTimer _scan_sensor_timer;
};
}
#endif  // ESP_UTILITY_DS18B20_H
