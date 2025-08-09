/*
 * Copyright (c) 2012 YuanHang Inc. All rights reserved.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND YuanHang HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @Description: Public Macro Definition
 * @Author: l2q
 * @Date: 2012/3/8 13:27
 * @LastEditors: lucky
 * @LastEditTime: 2023/4/7 8:15
 */

#ifndef ESP_UTILITY_PRECOMPILED_H
#define ESP_UTILITY_PRECOMPILED_H

#include <Arduino.h>

#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif

#ifndef SINGLE_TPL
#define SINGLE_TPL(className)                               \
private:                                                    \
    className() { create(); }                               \
    ~className() { destory(); }                             \
                                                            \
public:                                                     \
    className(const className&)                   = delete; \
    className&        operator=(const className&) = delete; \
    static className& getInstance()                         \
    {                                                       \
        static className _instance;                         \
        return _instance;                                   \
    }                                                       \
    void create();                                          \
    void destory();
#endif

#define Print(fmt, ...) Serial.printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define Println(fmt, ...) Serial.printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define GLOBAL_CFG_VERSION 1

#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

#endif  // ESP_UTILITY_PRECOMPILED_H
