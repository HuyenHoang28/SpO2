#ifndef FINGER_DETECTOR_H
#define FINGER_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FINGER_DETECTOR_WINDOW_SIZE 16U

typedef struct {
    uint32_t ir_window[FINGER_DETECTOR_WINDOW_SIZE];
    uint32_t red_window[FINGER_DETECTOR_WINDOW_SIZE];
    uint8_t write_index;
    uint8_t sample_count;
    uint8_t found_count;
    uint8_t lost_count;
    uint32_t baseline_ir;
    uint32_t baseline_red;
    uint32_t mean_ir;
    uint32_t mean_red;
    uint32_t span_ir;
    uint32_t span_red;
    uint32_t threshold_ir;
    bool baseline_valid;
    bool finger_present;
} FingerDetector;

void FingerDetector_Reset(FingerDetector *detector);
bool FingerDetector_Update(FingerDetector *detector,
                           uint32_t ir,
                           uint32_t red);
uint32_t FingerDetector_GetMeanIR(const FingerDetector *detector);
uint32_t FingerDetector_GetMeanRed(const FingerDetector *detector);
uint32_t FingerDetector_GetThresholdIR(const FingerDetector *detector);
uint32_t FingerDetector_GetSpanIR(const FingerDetector *detector);

#ifdef __cplusplus
}
#endif

#endif
