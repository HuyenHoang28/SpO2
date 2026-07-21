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
    uint32_t ir[N], red[N];
    SpO2AlgorithmResult result;
    int i;
    for (i = 0; i < N; ++i) {
        float phase = 2.0f * (float)M_PI * 1.2f * ((float)i / 25.0f);
        float shape = sinf(phase) + 0.18f * sinf(2.0f * phase + 0.2f);
        ir[i] = (uint32_t)(900.0f + 45.0f * shape);
        red[i] = (uint32_t)(700.0f + 25.0f * shape);
    }
    SpO2Algorithm_Compute(ir, red, N, 25.0f, 0U, &result);
    printf("low signal: HR=%d/%d SpO2=%d/%d Q=%u\n",
           result.heart_rate_bpm, result.heart_rate_valid,
           result.spo2_percent, result.spo2_valid,
           result.signal_quality);
    return (!result.heart_rate_valid || !result.spo2_valid) ? 1 : 0;
}
