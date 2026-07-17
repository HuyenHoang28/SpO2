#ifndef MAX30102_H
#define MAX30102_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX30102_I2C_ADDRESS_7BIT   0x57U
#define MAX30102_I2C_ADDRESS_HAL    (MAX30102_I2C_ADDRESS_7BIT << 1U)
#define MAX30102_PART_ID_EXPECTED   0x15U
#define MAX30102_FIFO_DEPTH         32U

typedef enum {
    MAX30102_OK = 0,
    MAX30102_ERROR_I2C,
    MAX30102_ERROR_NOT_FOUND,
    MAX30102_ERROR_BAD_PART_ID,
    MAX30102_ERROR_TIMEOUT,
    MAX30102_ERROR_ARGUMENT,
    MAX30102_ERROR_BUS_EMPTY,
    MAX30102_ERROR_NO_57_ON_BUS,
    MAX30102_ERROR_SDA_STUCK_LOW,
    MAX30102_ERROR_SCL_STUCK_LOW,
    MAX30102_STATUS_CHECKING
} MAX30102_Status;

typedef struct {
    uint32_t red;
    uint32_t ir;
} MAX30102_Sample;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
    uint8_t part_id;
    bool initialized;
} MAX30102_Handle;

/**
 * Default profile adapted from the reference project:
 * - Red + IR SpO2 mode
 * - ADC sample rate 100 sps
 * - 4-sample averaging => 25 FIFO samples/s
 * - 411 us pulse width, 18-bit ADC
 * - approximately 12.6 mA LED current for Red and IR
 */
MAX30102_Status MAX30102_InitDefault(MAX30102_Handle *dev,
                                      I2C_HandleTypeDef *hi2c);
MAX30102_Status MAX30102_Reset(MAX30102_Handle *dev);
MAX30102_Status MAX30102_ReadPartId(MAX30102_Handle *dev,
                                    uint8_t *part_id);
MAX30102_Status MAX30102_DiagnoseBus(I2C_HandleTypeDef *hi2c);
MAX30102_Status MAX30102_ClearFIFO(MAX30102_Handle *dev);
MAX30102_Status MAX30102_SetLedCurrent(MAX30102_Handle *dev,
                                       uint8_t red_current,
                                       uint8_t ir_current);
MAX30102_Status MAX30102_ReadAvailable(MAX30102_Handle *dev,
                                       MAX30102_Sample *samples,
                                       uint8_t capacity,
                                       uint8_t *sample_count);
MAX30102_Status MAX30102_Shutdown(MAX30102_Handle *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif
