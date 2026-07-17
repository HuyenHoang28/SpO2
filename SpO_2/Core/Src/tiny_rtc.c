#include "tiny_rtc.h"

#include <stddef.h>

#define RTC_REG_TIME_START      0x00U
#define DS3231_REG_STATUS       0x0FU
#define DS3231_STATUS_OSF       0x80U
#define DS1307_SECONDS_CH       0x80U
#define RTC_I2C_TIMEOUT_MS      100U

static uint8_t bcd_to_bin(uint8_t value)
{
    return (uint8_t)(((value >> 4U) * 10U) + (value & 0x0FU));
}

static uint8_t bin_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static bool valid_date_time(const TinyRTC_DateTime *dt)
{
    return (dt != NULL) &&
           (dt->year >= 2000U) && (dt->year <= 2099U) &&
           (dt->month >= 1U) && (dt->month <= 12U) &&
           (dt->day >= 1U) && (dt->day <= 31U) &&
           (dt->weekday >= 1U) && (dt->weekday <= 7U) &&
           (dt->hour <= 23U) && (dt->minute <= 59U) &&
           (dt->second <= 59U);
}

TinyRTC_Status TinyRTC_Init(TinyRTC_Handle *rtc,
                            I2C_HandleTypeDef *hi2c,
                            TinyRTC_Kind kind)
{
    if ((rtc == NULL) || (hi2c == NULL)) {
        return TINY_RTC_ERROR_ARGUMENT;
    }

    rtc->hi2c = hi2c;
    rtc->address = TINY_RTC_I2C_ADDRESS_HAL;
    rtc->kind = kind;
    rtc->initialized = false;

    if (HAL_I2C_IsDeviceReady(hi2c, rtc->address, 3U, 100U) != HAL_OK) {
        return TINY_RTC_ERROR_NOT_FOUND;
    }

    rtc->initialized = true;
    return TINY_RTC_OK;
}

TinyRTC_Status TinyRTC_GetDateTime(TinyRTC_Handle *rtc,
                                   TinyRTC_DateTime *date_time)
{
    uint8_t data[7];
    uint8_t hour_reg;

    if ((rtc == NULL) || (date_time == NULL) ||
        (rtc->hi2c == NULL)) {
        return TINY_RTC_ERROR_ARGUMENT;
    }

    if (HAL_I2C_Mem_Read(rtc->hi2c,
                         rtc->address,
                         RTC_REG_TIME_START,
                         I2C_MEMADD_SIZE_8BIT,
                         data,
                         sizeof(data),
                         RTC_I2C_TIMEOUT_MS) != HAL_OK) {
        return TINY_RTC_ERROR_I2C;
    }

    date_time->second = bcd_to_bin(data[0] & 0x7FU);
    date_time->minute = bcd_to_bin(data[1] & 0x7FU);

    hour_reg = data[2];
    if ((hour_reg & 0x40U) != 0U) {
        uint8_t hour12 = bcd_to_bin(hour_reg & 0x1FU);
        bool pm = (hour_reg & 0x20U) != 0U;
        if (hour12 == 12U) {
            hour12 = 0U;
        }
        date_time->hour = (uint8_t)(hour12 + (pm ? 12U : 0U));
    } else {
        date_time->hour = bcd_to_bin(hour_reg & 0x3FU);
    }

    date_time->weekday = bcd_to_bin(data[3] & 0x07U);
    date_time->day = bcd_to_bin(data[4] & 0x3FU);
    date_time->month = bcd_to_bin(data[5] & 0x1FU);
    date_time->year = (uint16_t)(2000U + bcd_to_bin(data[6]));

    return valid_date_time(date_time)
               ? TINY_RTC_OK
               : TINY_RTC_ERROR_INVALID_TIME;
}

TinyRTC_Status TinyRTC_SetDateTime(TinyRTC_Handle *rtc,
                                   const TinyRTC_DateTime *date_time)
{
    uint8_t data[7];

    if ((rtc == NULL) || (rtc->hi2c == NULL) ||
        !valid_date_time(date_time)) {
        return TINY_RTC_ERROR_ARGUMENT;
    }

    data[0] = bin_to_bcd(date_time->second); /* CH = 0 for DS1307. */
    data[1] = bin_to_bcd(date_time->minute);
    data[2] = bin_to_bcd(date_time->hour);   /* Force 24-hour mode. */
    data[3] = bin_to_bcd(date_time->weekday);
    data[4] = bin_to_bcd(date_time->day);
    data[5] = bin_to_bcd(date_time->month);
    data[6] = bin_to_bcd((uint8_t)(date_time->year - 2000U));

    if (HAL_I2C_Mem_Write(rtc->hi2c,
                          rtc->address,
                          RTC_REG_TIME_START,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          sizeof(data),
                          RTC_I2C_TIMEOUT_MS) != HAL_OK) {
        return TINY_RTC_ERROR_I2C;
    }

    if (rtc->kind == TINY_RTC_DS3231) {
        uint8_t status;
        if (HAL_I2C_Mem_Read(rtc->hi2c,
                             rtc->address,
                             DS3231_REG_STATUS,
                             I2C_MEMADD_SIZE_8BIT,
                             &status,
                             1U,
                             RTC_I2C_TIMEOUT_MS) == HAL_OK) {
            status &= (uint8_t)~DS3231_STATUS_OSF;
            (void)HAL_I2C_Mem_Write(rtc->hi2c,
                                    rtc->address,
                                    DS3231_REG_STATUS,
                                    I2C_MEMADD_SIZE_8BIT,
                                    &status,
                                    1U,
                                    RTC_I2C_TIMEOUT_MS);
        }
    }

    return TINY_RTC_OK;
}

TinyRTC_Status TinyRTC_IsTimeValid(TinyRTC_Handle *rtc,
                                   bool *is_valid)
{
    uint8_t value;
    uint8_t reg;

    if ((rtc == NULL) || (rtc->hi2c == NULL) || (is_valid == NULL)) {
        return TINY_RTC_ERROR_ARGUMENT;
    }

    reg = (rtc->kind == TINY_RTC_DS3231)
              ? DS3231_REG_STATUS
              : RTC_REG_TIME_START;

    if (HAL_I2C_Mem_Read(rtc->hi2c,
                         rtc->address,
                         reg,
                         I2C_MEMADD_SIZE_8BIT,
                         &value,
                         1U,
                         RTC_I2C_TIMEOUT_MS) != HAL_OK) {
        return TINY_RTC_ERROR_I2C;
    }

    *is_valid = (rtc->kind == TINY_RTC_DS3231)
                    ? ((value & DS3231_STATUS_OSF) == 0U)
                    : ((value & DS1307_SECONDS_CH) == 0U);
    return TINY_RTC_OK;
}
