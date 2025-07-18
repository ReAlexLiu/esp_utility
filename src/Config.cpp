/*
 * @Description:
 * @Author: l2q
 * @Date: 2025/07/18 15:30
 * @LastEditors: l2q
 * @LastEditTime: 2025/07/18 15:30
 *
 */
#include "config.h"

#include <c_types.h>
#include <spi_flash.h>

const uint16_t crcTalbe[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

extern uint32_t _EEPROM_start;  // See EEPROM.cpp
#define EEPROM_PHYS_ADDR ((uint32_t)(&_EEPROM_start) - 0x40200000)

void config::create()
{
}

void config::destory()
{
}

bool config::load()
{
    uint8_t          buf[6] = { 0 };
    SpiFlashOpResult ret    = spi_flash_read(EEPROM_PHYS_ADDR, (uint32*)buf, 6);
    if (SPI_FLASH_RESULT_OK != ret)
    {
        Print("Failed to read EEPROM, return value: %d", ret);
        return false;
    }

    bool   status = false;
    uint16 cfg    = (buf[0] << 8 | buf[1]);
    if (cfg != GLOBAL_CFG_VERSION)
    {
        reset();
        Print("Configuration file version mismatch");
        return false;
    }

    uint16 len = (buf[2] << 8 | buf[3]);
    _crc       = (buf[4] << 8 | buf[5]);

    if (len > ConfigMessage_size)
    {
        len = ConfigMessage_size;
    }

    uint8_t* data = (uint8_t*)malloc(len);
    ret           = spi_flash_read(EEPROM_PHYS_ADDR + 6, (uint32*)data, len);
    if (SPI_FLASH_RESULT_OK != ret)
    {
        free(data);
        Print("Failed to read EEPROM, return value: %d", ret);
        return false;
    }

    uint16_t crc = crc16(data, len);
    if (crc != _crc)
    {
        reset();
        free(data);
        Print("Data verification failed");
        return false;
    }

    memset(&_config, 0, sizeof(ConfigMessage));
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    status              = pb_decode(&stream, ConfigMessage_fields, &_config);
    if (!status)
    {
        reset();
        free(data);
        Print("Failed to serialize data");
        return false;
    }

    free(data);
    Print("read len: %d crc: %d", len, _crc);
    return true;
}

bool config::save()
{
    uint8_t      buffer[ConfigMessage_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool         status = pb_encode(&stream, ConfigMessage_fields, &_config);
    if (!status)
    {
        Print("save config failed");
        return false;
    }

    size_t   len = stream.bytes_written;
    uint16_t crc = crc16(buffer, len);
    if (crc == _crc)
    {
        Print("check config same data");
        return true;
    }

    _crc          = crc;

    // 读取原来数据
    uint8_t* data = (uint8_t*)malloc(SPI_FLASH_SEC_SIZE);
    int      ret  = spi_flash_read(EEPROM_PHYS_ADDR, (uint32*)data, SPI_FLASH_SEC_SIZE);
    if (SPI_FLASH_RESULT_OK != ret)
    {
        free(data);
        Print("Read EEPROM Data Error");
        return false;
    }

    // 拷贝数据
    data[0] = GLOBAL_CFG_VERSION >> 8;
    data[1] = GLOBAL_CFG_VERSION;

    data[2] = len >> 8;
    data[3] = len;

    data[4] = _crc >> 8;
    data[5] = _crc;

    memcpy(&data[6], buffer, len);

    // 擦写扇区
    ret = spi_flash_erase_sector(EEPROM_PHYS_ADDR / SPI_FLASH_SEC_SIZE);
    if (SPI_FLASH_RESULT_OK != ret)
    {
        free(data);
        Print("Erase Sector Error");
        return false;
    }

    // 写入数据
    ret = spi_flash_write(EEPROM_PHYS_ADDR, (uint32*)data, SPI_FLASH_SEC_SIZE);
    if (SPI_FLASH_RESULT_OK != ret)
    {
        free(data);
        Print("Write EEPROM Data Error");
        return false;
    }
    free(data);

    Print("save config len: %d crc: %d", len, _crc);
    return true;
}

bool config::reset()
{
    Print("reset config");
    memset(&_config, 0, sizeof(ConfigMessage));
    return save();
}

uint16_t config::crc16(uint8_t* ptr, uint16_t len)
{
    uint16_t crc = 0xffff;
    for (uint16_t i = 0; i < len; i++)
    {
        crc = crcTalbe[(ptr[i] ^ crc) & 15] ^ (crc >> 4);
        crc = crcTalbe[((ptr[i] >> 4) ^ crc) & 15] ^ (crc >> 4);
    }
    return crc;
}