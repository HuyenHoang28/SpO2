#include "spo2_algorithm.h"
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE        100u      /* 1 second at 100Hz */
#define FINGER_IR_MIN      50000u
#define MIN_BPM            40
#define MAX_BPM            180

static uint32_t red_buf[BUFFER_SIZE];
static uint32_t ir_buf[BUFFER_SIZE];
static uint32_t idx = 0;
static uint8_t filled = 0;
static SpO2Result_t last_result;

static uint32_t mean_u32(const uint32_t *x, uint32_t n)
{
    uint64_t sum = 0;
    for (uint32_t i = 0; i < n; i++) sum += x[i];
    return (uint32_t)(sum / n);
}

static uint32_t abs_diff_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static uint32_t ac_estimate(const uint32_t *x, uint32_t n, uint32_t dc)
{
    uint64_t acc = 0;
    for (uint32_t i = 0; i < n; i++) acc += abs_diff_u32(x[i], dc);
    return (uint32_t)(acc / n);
}

static int count_peaks_ir(uint32_t dc)
{
    int peaks = 0;
    uint32_t threshold = dc + (dc / 80u); /* adaptive threshold ~1.25% above DC */
    uint8_t above = 0;

    for (uint32_t i = 1; i + 1 < BUFFER_SIZE; i++) {
        uint32_t prev = ir_buf[i - 1];
        uint32_t cur  = ir_buf[i];
        uint32_t next = ir_buf[i + 1];
        if (!above && cur > threshold && cur > prev && cur >= next) {
            peaks++;
            above = 1;
        }
        if (cur < dc) above = 0;
    }
    return peaks;
}

void SpO2Algorithm_Init(void)
{
    memset(red_buf, 0, sizeof(red_buf));
    memset(ir_buf, 0, sizeof(ir_buf));
    idx = 0;
    filled = 0;
    memset(&last_result, 0, sizeof(last_result));
}

void SpO2Algorithm_AddSample(uint32_t red, uint32_t ir)
{
    red_buf[idx] = red;
    ir_buf[idx] = ir;
    idx = (idx + 1) % BUFFER_SIZE;
    if (idx == 0) filled = 1;

    if (!filled) return;

    uint32_t red_dc = mean_u32(red_buf, BUFFER_SIZE);
    uint32_t ir_dc  = mean_u32(ir_buf, BUFFER_SIZE);
    uint32_t red_ac = ac_estimate(red_buf, BUFFER_SIZE, red_dc);
    uint32_t ir_ac  = ac_estimate(ir_buf, BUFFER_SIZE, ir_dc);

    last_result.ir_dc = ir_dc;
    last_result.red_dc = red_dc;
    last_result.finger_detected = (ir_dc > FINGER_IR_MIN) ? 1 : 0;

    if (!last_result.finger_detected || red_dc == 0 || ir_dc == 0 || red_ac == 0 || ir_ac == 0) {
        last_result.valid = 0;
        last_result.bpm = 0;
        last_result.spo2 = 0;
        return;
    }

    int peaks = count_peaks_ir(ir_dc);
    int bpm = peaks * 60; /* because window is approximately 1 second */

    /* More stable: clamp unrealistic one-second peak count */
    if (bpm < MIN_BPM || bpm > MAX_BPM) {
        /* keep previous BPM when invalid */
        bpm = (last_result.bpm >= MIN_BPM && last_result.bpm <= MAX_BPM) ? last_result.bpm : 0;
    }

    /* Ratio of ratios: R = (ACred/DCred) / (ACir/DCir) */
    float r = ((float)red_ac / (float)red_dc) / ((float)ir_ac / (float)ir_dc);
    int spo2 = (int)(110.0f - 25.0f * r);
    if (spo2 > 100) spo2 = 100;
    if (spo2 < 70) spo2 = 70;

    last_result.bpm = bpm;
    last_result.spo2 = spo2;
    last_result.valid = (bpm > 0) ? 1 : 0;
}

SpO2Result_t SpO2Algorithm_GetResult(void)
{
    return last_result;
}
