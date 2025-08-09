#ifndef ESP_UTILITY_ELAPSED_TIMER_H
#define ESP_UTILITY_ELAPSED_TIMER_H

#include "Arduino.h"

namespace esp_utility
{
class ElapsedTimer
{
private:
    unsigned long _startTime;        // 计时开始时间
    unsigned long _accumulatedTime;  // 累计时间（暂停时使用）
    bool          _isRunning;        // 计时器是否正在运行

public:
    // 构造函数
    ElapsedTimer();

    // 开始计时
    void          start();

    // 停止计时（保留累计时间）
    void          stop();

    // 重置计时器
    void          reset();

    // 重启计时器（重置并立即开始）
    void          restart();

    // 获取经过的毫秒数
    unsigned long elapsedMillis();

    // 检查计时器是否正在运行
    bool          isActive() const;

    // 判断是否已超过指定的超时时间（单位：毫秒）
    // 如果已超过或等于timeout，返回true；否则返回false
    bool          hasExpired(int timeout);
};
}
#endif  // ESP_UTILITY_ELAPSED_TIMER_H