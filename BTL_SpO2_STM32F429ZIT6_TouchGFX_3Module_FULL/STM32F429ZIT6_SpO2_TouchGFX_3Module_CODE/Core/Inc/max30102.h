#ifndef MAX30102_H
#define MAX30102_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    MAX30102_OK = 0,
    MAX30102_ERROR = 1,
    MAX30102_NOT_FOUND = 2
} MAX30102_Status_t;

typedef struct {
    uint32_t red;
    uint32_t ir;
} MAX30102_Sample_t;

#define MAX30102_I2C_ADDR        (0x57 << 1)
#define MAX30102_PART_ID_VALUE   0x15

MAX30102_Status_t MAX30102_Init(I2C_HandleTypeDef *hi2c);
MAX30102_Status_t MAX30102_Reset(I2C_HandleTypeDef *hi2c);
MAX30102_Status_t MAX30102_ReadPartID(I2C_HandleTypeDef *hi2c, uint8_t *part_id);
MAX30102_Status_t MAX30102_ReadFIFO(I2C_HandleTypeDef *hi2c, MAX30102_Sample_t *sample);
MAX30102_Status_t MAX30102_SetLedAmplitude(I2C_HandleTypeDef *hi2c, uint8_t red_pa, uint8_t ir_pa);
MAX30102_Status_t MAX30102_Shutdown(I2C_HandleTypeDef *hi2c, uint8_t enable);

#endif
