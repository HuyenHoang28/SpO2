#ifndef SPO2_APP_H
#define SPO2_APP_H

#include "max30102.h"
#include "tiny_rtc.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPO2_APP_BUFFER_LENGTH             100U
#define SPO2_APP_RECALCULATE_SAMPLES       25U
#define SPO2_APP_FIFO_SAMPLE_RATE_HZ       25.0f
#define SPO2_APP_FINGER_IR_THRESHOLD       5000U
#define SPO2_APP_LOW_SPO2_THRESHOLD        94

typedef enum {
    SPO2_APP_SENSOR_ERROR = 0,
    SPO2_APP_PLACE_FINGER,
    SPO2_APP_MEASURING,
    SPO2_APP_INVALID_SIGNAL,
    SPO2_APP_NORMAL,
    SPO2_APP_LOW_SPO2,
    SPO2_APP_LOW_HEART_RATE,
    SPO2_APP_HIGH_HEART_RATE
} SpO2AppStatus;

typedef struct {
    uint32_t sequence;
    uint32_t measurement_sequence;
    SpO2AppStatus status;
    int16_t heart_rate_bpm;
    int16_t spo2_percent;
    uint8_t signal_quality;
    int16_t waveform; /* 0..100 for a TouchGFX Dynamic Graph. */
    uint32_t raw_ir;
    uint32_t raw_red;
    bool sensor_ok;
    MAX30102_Status sensor_error;
    bool finger_present;
    bool measurement_valid;
    bool low_spo2;
    bool abnormal_heart_rate;
    bool rtc_ok;
    bool rtc_time_valid;
    TinyRTC_DateTime date_time;
} SpO2AppSnapshot;

void SpO2App_Init(I2C_HandleTypeDef *hi2c, TinyRTC_Kind rtc_kind);
void SpO2App_Process(void);
void SpO2App_GetSnapshot(SpO2AppSnapshot *snapshot);
TinyRTC_Status SpO2App_SetDateTime(const TinyRTC_DateTime *date_time);

#ifdef __cplusplus
}
#endif

#endif
