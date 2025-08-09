/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/07/18 15:30
 * @LastEditors: l2q
 * @LastEditTime: 2025/07/18 15:30
 *
 */

#ifndef ESP_UTILITY_FUN_CONTROL_H
#define ESP_UTILITY_FUN_CONTROL_H
#include "ElapsedTimer.h"
#include "precompiled.h"

#include <ESP8266WebServer.h>

#define PWM_MOTOR1 5
#define PWM_MOTOR2 13
#define PWM_MOTOR3 12
#define PWM_MOTOR4 14

namespace esp_utility
{

class fun_control
{
    SINGLE_TPL(fun_control);

public:
    void begin(ESP8266WebServer& server);
    void update();

private:
    int          _fan_pins[4]            = { PWM_MOTOR1, PWM_MOTOR2, PWM_MOTOR3, PWM_MOTOR4 };
    int          _fan_speeds[4]          = { 0, 0, 0, 0 };
    bool         _remote_control_enabled = false;
    ElapsedTimer _update_timer;
};
}
#endif  // ESP_UTILITY_FUN_CONTROL_H
