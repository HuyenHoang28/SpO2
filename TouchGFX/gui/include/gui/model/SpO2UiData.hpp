#ifndef SPO2UIDATA_HPP
#define SPO2UIDATA_HPP

#include <stdint.h>

enum SpO2UiStatus
{
    SPO2_UI_SENSOR_ERROR = 0,
    SPO2_UI_PLACE_FINGER,
    SPO2_UI_MEASURING,
    SPO2_UI_INVALID_SIGNAL,
    SPO2_UI_NORMAL,
    SPO2_UI_LOW_SPO2,
    SPO2_UI_LOW_HEART_RATE,
    SPO2_UI_HIGH_HEART_RATE
};

struct SpO2UiData
{
    uint32_t sequence;
    uint32_t measurementSequence;
    SpO2UiStatus status;
    int16_t heartRateBpm;
    int16_t spo2Percent;
    uint8_t signalQuality;
    int16_t waveform;
    uint32_t rawIr;
    uint32_t rawRed;
    uint32_t fingerThreshold;
    uint32_t irSpan;
    uint16_t bufferedSamples;
    uint8_t ledCurrent;
    bool sensorOk;
    uint8_t sensorError;
    bool fingerPresent;
    bool heartRateValid;
    bool spo2Valid;
    bool measurementValid;
    bool rtcOk;
    bool rtcTimeValid;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

#endif // SPO2UIDATA_HPP
