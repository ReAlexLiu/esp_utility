/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/08/09 20:59
 * @LastEditors: l2q
 * @LastEditTime: 2025/08/09 20:59
 *
 */

#ifndef ESP_MODULE_H
#define ESP_MODULE_H
#include "precompiled.h"

#include <ESP8266WebServer.h>
namespace esp_utility
{
class module
{
    SINGLE_TPL(module);

public:
    void begin(ESP8266WebServer& server);
    void update();
};
}
#endif  // ESP_MODULE_H
