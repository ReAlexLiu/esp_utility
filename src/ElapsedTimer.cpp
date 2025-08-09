#include "ElapsedTimer.h"

namespace esp_utility
{
ElapsedTimer::ElapsedTimer()
    : _accumulatedTime(0)
    , _isRunning(false)
{
}

// 开始计时
void ElapsedTimer::start()
{
    if (!_isRunning)
    {
        _startTime = millis();
        _isRunning = true;
    }
}

// 停止计时（保留累计时间）
void ElapsedTimer::stop()
{
    if (_isRunning)
    {
        _accumulatedTime += millis() - _startTime;
        _isRunning = false;
    }
}

// 重置计时器
void ElapsedTimer::reset()
{
    _accumulatedTime = 0;
    _isRunning       = false;
}

// 重启计时器（重置并立即开始）
void ElapsedTimer::restart()
{
    _accumulatedTime = 0;
    _startTime       = millis();
    _isRunning       = true;
}

// 获取经过的毫秒数
unsigned long ElapsedTimer::elapsedMillis()
{
    if (_isRunning)
    {
        return _accumulatedTime + (millis() - _startTime);
    }
    else
    {
        return _accumulatedTime;
    }
}

// 检查计时器是否正在运行
bool ElapsedTimer::isActive() const
{
    return _isRunning;
}

// 判断是否已超过指定的超时时间（单位：毫秒）
// 如果已超过或等于timeout，返回true；否则返回false
bool ElapsedTimer::hasExpired(int timeout)
{
    // 确保计时器正在运行
    if (!_isRunning)
    {
        return false;  // 未运行时视为未超时
    }

    // 计算当前已过去的时间
    unsigned long currentElapsed = elapsedMillis();

    // 处理可能的溢出情况
    if (timeout > 0)
    {
        return currentElapsed >= (unsigned long)timeout;
    }

    return false;
}
};