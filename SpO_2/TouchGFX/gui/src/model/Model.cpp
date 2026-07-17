#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifdef SIMULATOR
#include <math.h>
#else
extern "C"
{
#include "spo2_app.h"
}
#endif

Model::Model()
    : modelListener(0),
      lastSequence(0U)
#ifdef SIMULATOR
      , simulatorTick(0U)
#endif
{
}

#ifndef SIMULATOR
static SpO2UiStatus mapStatus(SpO2AppStatus status)
{
    switch (status)
    {
    case SPO2_APP_SENSOR_ERROR:    return SPO2_UI_SENSOR_ERROR;
    case SPO2_APP_PLACE_FINGER:    return SPO2_UI_PLACE_FINGER;
    case SPO2_APP_MEASURING:       return SPO2_UI_MEASURING;
    case SPO2_APP_INVALID_SIGNAL:  return SPO2_UI_INVALID_SIGNAL;
    case SPO2_APP_NORMAL:          return SPO2_UI_NORMAL;
    case SPO2_APP_LOW_SPO2:        return SPO2_UI_LOW_SPO2;
    case SPO2_APP_LOW_HEART_RATE:  return SPO2_UI_LOW_HEART_RATE;
    case SPO2_APP_HIGH_HEART_RATE: return SPO2_UI_HIGH_HEART_RATE;
    default:                       return SPO2_UI_SENSOR_ERROR;
    }
}
#endif

void Model::tick()
{
    if (modelListener == 0)
    {
        return;
    }

#ifdef SIMULATOR
    ++simulatorTick;
    if ((simulatorTick % 6U) != 0U)
    {
        return;
    }

    SpO2UiData data = {};
    const float phase = static_cast<float>(simulatorTick) * 0.08f;
    data.sequence = simulatorTick;
    data.measurementSequence = simulatorTick / 60U;
    data.status = SPO2_UI_NORMAL;
    data.heartRateBpm = static_cast<int16_t>(74.0f + 4.0f * sinf(phase * 0.25f));
    data.spo2Percent = static_cast<int16_t>(97.0f + sinf(phase * 0.12f));
    data.signalQuality = 90U;
    data.waveform = static_cast<int16_t>(50.0f + 35.0f * sinf(phase));
    data.sensorOk = true;
    data.sensorError = 0U;
    data.fingerPresent = true;
    data.measurementValid = true;
    data.rtcOk = true;
    data.rtcTimeValid = true;
    data.year = 2026U;
    data.month = 7U;
    data.day = 17U;
    data.hour = static_cast<uint8_t>(18U + ((simulatorTick / 216000U) % 6U));
    data.minute = static_cast<uint8_t>((simulatorTick / 3600U) % 60U);
    data.second = static_cast<uint8_t>((simulatorTick / 60U) % 60U);
    modelListener->onSpO2DataChanged(data);
#else
    SpO2AppSnapshot snapshot;
    SpO2App_GetSnapshot(&snapshot);

    if (snapshot.sequence == lastSequence)
    {
        return;
    }
    lastSequence = snapshot.sequence;

    SpO2UiData data = {};
    data.sequence = snapshot.sequence;
    data.measurementSequence = snapshot.measurement_sequence;
    data.status = mapStatus(snapshot.status);
    data.heartRateBpm = snapshot.heart_rate_bpm;
    data.spo2Percent = snapshot.spo2_percent;
    data.signalQuality = snapshot.signal_quality;
    data.waveform = snapshot.waveform;
    data.sensorOk = snapshot.sensor_ok;
    data.sensorError = static_cast<uint8_t>(snapshot.sensor_error);
    data.fingerPresent = snapshot.finger_present;
    data.measurementValid = snapshot.measurement_valid;
    data.rtcOk = snapshot.rtc_ok;
    data.rtcTimeValid = snapshot.rtc_time_valid;
    data.year = snapshot.date_time.year;
    data.month = snapshot.date_time.month;
    data.day = snapshot.date_time.day;
    data.hour = snapshot.date_time.hour;
    data.minute = snapshot.date_time.minute;
    data.second = snapshot.date_time.second;
    modelListener->onSpO2DataChanged(data);
#endif
}
