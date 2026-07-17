#include "health_monitor.h"
#include "max30102.h"
#include "tiny_rtc.h"
#include "spo2_algorithm.h"
#include <string.h>

static I2C_HandleTypeDef *hm_i2c = 0;
static volatile HealthData_t hm_data;
static uint32_t rtc_divider = 0;

void HealthMonitor_Init(I2C_HandleTypeDef *i2c_sensor_rtc)
{
    hm_i2c = i2c_sensor_rtc;
    memset((void*)&hm_data, 0, sizeof(hm_data));
    hm_data.status = HM_STATUS_INIT;
    SpO2Algorithm_Init();

    if (MAX30102_Init(hm_i2c) != MAX30102_OK) {
        hm_data.status = HM_STATUS_SENSOR_ERROR;
        return;
    }
    if (TinyRTC_Init(hm_i2c) != TINY_RTC_OK) {
        hm_data.status = HM_STATUS_RTC_ERROR;
        /* Continue measuring even if RTC is missing */
    } else {
        hm_data.status = HM_STATUS_PLACE_FINGER;
    }
}

void HealthMonitor_Process10ms(void)
{
    if (hm_i2c == 0) return;
    if (hm_data.status == HM_STATUS_SENSOR_ERROR) return;

    MAX30102_Sample_t s;
    if (MAX30102_ReadFIFO(hm_i2c, &s) != MAX30102_OK) {
        hm_data.status = HM_STATUS_SENSOR_ERROR;
        return;
    }

    hm_data.red_raw = s.red;
    hm_data.ir_raw = s.ir;
    SpO2Algorithm_AddSample(s.red, s.ir);
    SpO2Result_t r = SpO2Algorithm_GetResult();

    hm_data.bpm = r.bpm;
    hm_data.spo2 = r.spo2;
    hm_data.valid = r.valid;

    if (!r.finger_detected) {
        hm_data.status = HM_STATUS_PLACE_FINGER;
    } else if (r.valid && r.spo2 < 94) {
        hm_data.status = HM_STATUS_LOW_SPO2;
    } else if (r.valid) {
        hm_data.status = HM_STATUS_VALID;
    } else {
        hm_data.status = HM_STATUS_MEASURING;
    }

    /* Read RTC once per second: 100 * 10ms = 1s */
    rtc_divider++;
    if (rtc_divider >= 100) {
        rtc_divider = 0;
        TinyRTC_Time_t t;
        if (TinyRTC_GetTime(hm_i2c, &t) == TINY_RTC_OK) {
            hm_data.hour = t.hour;
            hm_data.minute = t.minute;
            hm_data.second = t.second;
            hm_data.date = t.date;
            hm_data.month = t.month;
            hm_data.year = t.year;
        }
    }
}

HealthData_t HealthMonitor_GetData(void)
{
    return hm_data;
}

const char* HealthMonitor_StatusText(HealthStatus_t status)
{
    switch (status) {
    case HM_STATUS_INIT: return "Initializing";
    case HM_STATUS_PLACE_FINGER: return "Place finger";
    case HM_STATUS_MEASURING: return "Measuring";
    case HM_STATUS_VALID: return "Normal";
    case HM_STATUS_LOW_SPO2: return "Low SpO2 Warning";
    case HM_STATUS_SENSOR_ERROR: return "MAX30102 Error";
    case HM_STATUS_RTC_ERROR: return "RTC Error";
    default: return "Unknown";
    }
}
