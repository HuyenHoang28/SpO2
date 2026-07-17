#ifndef TINY_RTC_H
#define TINY_RTC_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define TINY_RTC_I2C_ADDR (0x68 << 1)

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_week;
    uint8_t date;
    uint8_t month;
    uint16_t year;
} TinyRTC_Time_t;

typedef enum {
    TINY_RTC_OK = 0,
    TINY_RTC_ERROR = 1
} TinyRTC_Status_t;

TinyRTC_Status_t TinyRTC_Init(I2C_HandleTypeDef *hi2c);
TinyRTC_Status_t TinyRTC_GetTime(I2C_HandleTypeDef *hi2c, TinyRTC_Time_t *time);
TinyRTC_Status_t TinyRTC_SetTime(I2C_HandleTypeDef *hi2c, const TinyRTC_Time_t *time);

#endif
