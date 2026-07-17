#include "spo2_algorithm.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static float clampf(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static void detrend_and_smooth(const uint32_t *input,
                               uint16_t length,
                               float *output,
                               float *dc_mean,
                               float *rms)
{
    float sum = 0.0f;
    float line_start;
    float line_slope;
    float centered_mean = 0.0f;
    float energy = 0.0f;
    float temp[SPO2_ALGORITHM_MAX_SAMPLES];
    uint16_t i;

    for (i = 0U; i < length; ++i) {
        sum += (float)input[i];
    }
    *dc_mean = sum / (float)length;

    line_start = (float)input[0];
    line_slope = ((float)input[length - 1U] - line_start) /
                 (float)(length - 1U);

    for (i = 0U; i < length; ++i) {
        temp[i] = (float)input[i] - (line_start + line_slope * (float)i);
        centered_mean += temp[i];
    }
    centered_mean /= (float)length;

    for (i = 0U; i < length; ++i) {
        temp[i] -= centered_mean;
    }

    /* Three-point moving average reduces high-frequency noise. */
    output[0] = temp[0];
    for (i = 1U; i < (uint16_t)(length - 1U); ++i) {
        output[i] = (temp[i - 1U] + temp[i] + temp[i + 1U]) / 3.0f;
    }
    output[length - 1U] = temp[length - 1U];

    for (i = 0U; i < length; ++i) {
        energy += output[i] * output[i];
    }
    *rms = sqrtf(energy / (float)length);
}

static float normalized_autocorrelation(const float *x,
                                        uint16_t length,
                                        uint16_t lag)
{
    float cross = 0.0f;
    float energy_a = 0.0f;
    float energy_b = 0.0f;
    uint16_t i;

    for (i = 0U; i < (uint16_t)(length - lag); ++i) {
        float a = x[i];
        float b = x[i + lag];
        cross += a * b;
        energy_a += a * a;
        energy_b += b * b;
    }

    if ((energy_a <= 1.0e-6f) || (energy_b <= 1.0e-6f)) {
        return 0.0f;
    }
    return cross / sqrtf(energy_a * energy_b);
}

static bool estimate_heart_rate(const float *ir_ac,
                                uint16_t length,
                                float sample_rate_hz,
                                float *heart_rate,
                                float *correlation)
{
    uint16_t min_lag = (uint16_t)(sample_rate_hz * 60.0f / 220.0f);
    uint16_t max_lag = (uint16_t)(sample_rate_hz * 60.0f / 35.0f);
    float corr[SPO2_ALGORITHM_MAX_SAMPLES];
    float best_corr = -1.0f;
    uint16_t best_lag = 0U;
    uint16_t selected_lag = 0U;
    uint16_t lag;

    if (min_lag < 2U) {
        min_lag = 2U;
    }
    if (max_lag >= (uint16_t)(length / 2U)) {
        max_lag = (uint16_t)(length / 2U);
    }
    if (min_lag >= max_lag) {
        return false;
    }

    memset(corr, 0, sizeof(corr));
    for (lag = min_lag; lag <= max_lag; ++lag) {
        corr[lag] = normalized_autocorrelation(ir_ac, length, lag);
        if (corr[lag] > best_corr) {
            best_corr = corr[lag];
            best_lag = lag;
        }
    }

    /* Prefer the first strong local peak so a harmonic is not selected. */
    for (lag = (uint16_t)(min_lag + 1U); lag < max_lag; ++lag) {
        if ((corr[lag] >= corr[lag - 1U]) &&
            (corr[lag] >= corr[lag + 1U]) &&
            (corr[lag] >= best_corr * 0.85f) &&
            (corr[lag] > 0.20f)) {
            selected_lag = lag;
            break;
        }
    }
    if (selected_lag == 0U) {
        selected_lag = best_lag;
    }

    if ((selected_lag == 0U) || (best_corr < 0.20f)) {
        return false;
    }

    /* Parabolic interpolation around the correlation peak. */
    {
        float refined_lag = (float)selected_lag;
        if ((selected_lag > min_lag) && (selected_lag < max_lag)) {
            float y1 = corr[selected_lag - 1U];
            float y2 = corr[selected_lag];
            float y3 = corr[selected_lag + 1U];
            float denominator = y1 - (2.0f * y2) + y3;
            if (fabsf(denominator) > 1.0e-6f) {
                refined_lag += 0.5f * (y1 - y3) / denominator;
            }
        }
        *heart_rate = 60.0f * sample_rate_hz / refined_lag;
    }

    *correlation = corr[selected_lag];
    return (*heart_rate >= 35.0f) && (*heart_rate <= 220.0f);
}

void SpO2Algorithm_Compute(const uint32_t *ir,
                           const uint32_t *red,
                           uint16_t length,
                           float sample_rate_hz,
                           uint32_t finger_threshold,
                           SpO2AlgorithmResult *result)
{
    float ir_ac[SPO2_ALGORITHM_MAX_SAMPLES];
    float red_ac[SPO2_ALGORITHM_MAX_SAMPLES];
    float ir_dc = 0.0f;
    float red_dc = 0.0f;
    float ir_rms = 0.0f;
    float red_rms = 0.0f;
    float ratio;
    float spo2;
    float heart_rate = 0.0f;
    float correlation = 0.0f;
    bool hr_ok;

    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));

    if ((ir == NULL) || (red == NULL) ||
        (length < 32U) || (length > SPO2_ALGORITHM_MAX_SAMPLES) ||
        (sample_rate_hz <= 0.0f)) {
        return;
    }

    detrend_and_smooth(ir, length, ir_ac, &ir_dc, &ir_rms);
    detrend_and_smooth(red, length, red_ac, &red_dc, &red_rms);

    result->finger_present = (ir_dc >= (float)finger_threshold);
    if (!result->finger_present || (ir_dc <= 1.0f) || (red_dc <= 1.0f)) {
        return;
    }

    hr_ok = estimate_heart_rate(ir_ac, length, sample_rate_hz,
                                &heart_rate, &correlation);
    result->signal_quality = (uint8_t)clampf(correlation * 100.0f,
                                             0.0f, 100.0f);

    if (hr_ok && (ir_rms / ir_dc > 0.0005f)) {
        result->heart_rate_bpm = (int16_t)lroundf(heart_rate);
        result->heart_rate_valid = true;
    }

    if ((ir_rms > 1.0f) && (red_rms > 1.0f)) {
        ratio = (red_rms / red_dc) / (ir_rms / ir_dc);

        /* Common empirical ratio-of-ratios calibration curve. */
        spo2 = (-45.060f * ratio * ratio) +
               (30.354f * ratio) + 94.845f;

        if ((ratio >= 0.20f) && (ratio <= 1.80f) &&
            (spo2 >= 70.0f) && (spo2 <= 100.5f) &&
            (ir_rms / ir_dc > 0.0005f)) {
            result->spo2_percent = (int16_t)lroundf(clampf(spo2, 70.0f, 100.0f));
            result->spo2_valid = true;
        }
    }
}
