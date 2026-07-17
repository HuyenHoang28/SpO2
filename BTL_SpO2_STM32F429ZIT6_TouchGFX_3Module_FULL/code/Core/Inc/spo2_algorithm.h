#ifndef SPO2_ALGORITHM_H
#define SPO2_ALGORITHM_H

#include <stdint.h>

typedef struct {
    int32_t bpm;
    int32_t spo2;
    uint8_t valid;
    uint8_t finger_detected;
    uint32_t ir_dc;
    uint32_t red_dc;
} SpO2Result_t;

void SpO2Algorithm_Init(void);
void SpO2Algorithm_AddSample(uint32_t red, uint32_t ir);
SpO2Result_t SpO2Algorithm_GetResult(void);

#endif
