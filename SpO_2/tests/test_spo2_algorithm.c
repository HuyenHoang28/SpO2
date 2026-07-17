#include "spo2_algorithm.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void)
{
    enum { N = 100 };
    const float fs = 25.0f;
    const float bpm = 75.0f;
    uint32_t ir[N];
    uint32_t red[N];
    SpO2AlgorithmResult result;
    int i;

    for (i = 0; i < N; ++i) {
        float phase = 2.0f * (float)M_PI * (bpm / 60.0f) *
                      ((float)i / fs);
        float harmonic = 0.20f * sinf(2.0f * phase + 0.3f);
        ir[i] = (uint32_t)(100000.0f + 5000.0f *
                           (sinf(phase) + harmonic));
        red[i] = (uint32_t)(90000.0f + 2668.0f *
                            (sinf(phase) + harmonic));
    }

    SpO2Algorithm_Compute(ir, red, N, fs, 50000U, &result);
    printf("HR=%d valid=%d, SpO2=%d valid=%d, quality=%u\n",
           result.heart_rate_bpm,
           result.heart_rate_valid,
           result.spo2_percent,
           result.spo2_valid,
           result.signal_quality);

    if (!result.heart_rate_valid || !result.spo2_valid) {
        return 1;
    }
    if ((result.heart_rate_bpm < 72) || (result.heart_rate_bpm > 78)) {
        return 2;
    }
    if ((result.spo2_percent < 95) || (result.spo2_percent > 99)) {
        return 3;
    }
    return 0;
}
