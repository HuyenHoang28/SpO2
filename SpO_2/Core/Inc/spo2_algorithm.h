#ifndef SPO2_ALGORITHM_H
#define SPO2_ALGORITHM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPO2_ALGORITHM_MAX_SAMPLES  128U

typedef struct {
    int16_t spo2_percent;
    int16_t heart_rate_bpm;
    uint8_t signal_quality;
    bool finger_present;
    bool spo2_valid;
    bool heart_rate_valid;
} SpO2AlgorithmResult;

/**
 * Course-project estimation from raw Red/IR PPG samples.
 * It is not a medical-device algorithm and must not be used for diagnosis.
 */
void SpO2Algorithm_Compute(const uint32_t *ir,
                           const uint32_t *red,
                           uint16_t length,
                           float sample_rate_hz,
                           uint32_t finger_threshold,
                           SpO2AlgorithmResult *result);

#ifdef __cplusplus
}
#endif

#endif
