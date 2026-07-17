#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HM_STATUS_INIT = 0,
    HM_STATUS_PLACE_FINGER,
    HM_STATUS_MEASURING,
    HM_STATUS_VALID,
    HM_STATUS_LOW_SPO2,
    HM_STATUS_SENSOR_ERROR,
    HM_STATUS_RTC_ERROR
} HealthStatus_t;

typedef struct {
    int32_t bpm;
    int32_t spo2;
    uint8_t valid;
    HealthStatus_t status;
    uint8_t hour, minute, second;
    uint8_t date, month;
    uint16_t year;
    uint32_t red_raw;
    uint32_t ir_raw;
} HealthData_t;

void HealthMonitor_Init(I2C_HandleTypeDef *i2c_sensor_rtc);
void HealthMonitor_Process10ms(void);
HealthData_t HealthMonitor_GetData(void);
const char* HealthMonitor_StatusText(HealthStatus_t status);

#ifdef __cplusplus
}
#endif

#endif
