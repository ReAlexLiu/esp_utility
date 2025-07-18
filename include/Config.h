/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/07/18 15:30
 * @LastEditors: l2q
 * @LastEditTime: 2025/07/18 15:30
 *
 */

#ifndef ESP_UTILITY_CONFIG_H
#define ESP_UTILITY_CONFIG_H

#include "../../../include/config.pb.h"
#include "precompiled.h"

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

class config
{
    SINGLE_TPL(config)

public:
    bool load();
    bool save();
    bool reset();

private:
    uint16_t crc16(uint8_t* ptr, uint16_t len);

public:
    ConfigMessage _config;

private:
    uint16_t _crc;
};

#endif  // ESP_UTILITY_CONFIG_H
