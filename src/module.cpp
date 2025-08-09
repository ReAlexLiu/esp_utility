/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/08/09 20:58
 * @LastEditors: l2q
 * @LastEditTime: 2025/08/09 20:58
 *
 */
#include "module.h"
#include "config.h"

#define RESET_PIN 4 // 定义IO4引脚

namespace esp_utility
{
void module::create()
{
}

void module::destory()
{
}

void clear_config() {
    Println("Button on IO4 pressed, erasing config...");

    esp_utility::config::getInstance().reset();

    delay(1000); // 等待配置清除完成
    ESP.reset(); // 重启ESP8266
}

void module::begin(ESP8266WebServer& server)
{
    pinMode(RESET_PIN, INPUT_PULLUP); // 将IO4设置为上拉输入模式
    attachInterrupt(digitalPinToInterrupt(RESET_PIN), clear_config, FALLING); // 设置下降沿触发中断
}

void module::update()
{
}
}  // namespace esp_utility