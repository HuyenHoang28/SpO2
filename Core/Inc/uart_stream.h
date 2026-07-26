#ifndef UART_STREAM_H
#define UART_STREAM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void UartStream_Init(void);

bool UartStream_SendSpO2(int16_t hr_bpm,
                         int16_t spo2_percent,
                         bool    low_spo2,
                         bool    abnormal_hr,
                         uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif /* UART_STREAM_H */
