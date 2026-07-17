#ifndef TINY_RTC_H
#define TINY_RTC_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TINY_RTC_I2C_ADDRESS_7BIT  0x68U
#define TINY_RTC_I2C_ADDRESS_HAL   (TINY_RTC_I2C_ADDRESS_7BIT << 1U)

typedef enum {
    TINY_RTC_DS3231 = 0,
    TINY_RTC_DS1307
} TinyRTC_Kind;

typedef enum {
    TINY_RTC_OK = 0,
    TINY_RTC_ERROR_I2C,
    TINY_RTC_ERROR_NOT_FOUND,
    TINY_RTC_ERROR_INVALID_TIME,
    TINY_RTC_ERROR_ARGUMENT
} TinyRTC_Status;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} TinyRTC_DateTime;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
    TinyRTC_Kind kind;
    bool initialized;
} TinyRTC_Handle;

TinyRTC_Status TinyRTC_Init(TinyRTC_Handle *rtc,
                            I2C_HandleTypeDef *hi2c,
                            TinyRTC_Kind kind);
TinyRTC_Status TinyRTC_GetDateTime(TinyRTC_Handle *rtc,
                                   TinyRTC_DateTime *date_time);
TinyRTC_Status TinyRTC_SetDateTime(TinyRTC_Handle *rtc,
                                   const TinyRTC_DateTime *date_time);
TinyRTC_Status TinyRTC_IsTimeValid(TinyRTC_Handle *rtc,
                                   bool *is_valid);

#ifdef __cplusplus
}
#endif

#endif
