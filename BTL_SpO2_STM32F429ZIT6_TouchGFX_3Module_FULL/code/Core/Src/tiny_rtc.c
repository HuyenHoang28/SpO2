#include "tiny_rtc.h"

static uint8_t bcd_to_dec(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0F); }
static uint8_t dec_to_bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }

TinyRTC_Status_t TinyRTC_Init(I2C_HandleTypeDef *hi2c)
{
    if (HAL_I2C_IsDeviceReady(hi2c, TINY_RTC_I2C_ADDR, 3, 100) != HAL_OK)
        return TINY_RTC_ERROR;
    return TINY_RTC_OK;
}

TinyRTC_Status_t TinyRTC_GetTime(I2C_HandleTypeDef *hi2c, TinyRTC_Time_t *time)
{
    uint8_t raw[7];
    if (HAL_I2C_Mem_Read(hi2c, TINY_RTC_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, raw, 7, 100) != HAL_OK)
        return TINY_RTC_ERROR;

    time->second = bcd_to_dec(raw[0] & 0x7F);
    time->minute = bcd_to_dec(raw[1] & 0x7F);
    time->hour   = bcd_to_dec(raw[2] & 0x3F); /* 24-hour mode */
    time->day_of_week = bcd_to_dec(raw[3] & 0x07);
    time->date   = bcd_to_dec(raw[4] & 0x3F);
    time->month  = bcd_to_dec(raw[5] & 0x1F);
    time->year   = 2000 + bcd_to_dec(raw[6]);
    return TINY_RTC_OK;
}

TinyRTC_Status_t TinyRTC_SetTime(I2C_HandleTypeDef *hi2c, const TinyRTC_Time_t *time)
{
    uint8_t raw[7];
    raw[0] = dec_to_bcd(time->second);
    raw[1] = dec_to_bcd(time->minute);
    raw[2] = dec_to_bcd(time->hour); /* 24-hour mode */
    raw[3] = dec_to_bcd(time->day_of_week);
    raw[4] = dec_to_bcd(time->date);
    raw[5] = dec_to_bcd(time->month);
    raw[6] = dec_to_bcd((uint8_t)(time->year % 100));

    if (HAL_I2C_Mem_Write(hi2c, TINY_RTC_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, raw, 7, 100) != HAL_OK)
        return TINY_RTC_ERROR;
    return TINY_RTC_OK;
}
