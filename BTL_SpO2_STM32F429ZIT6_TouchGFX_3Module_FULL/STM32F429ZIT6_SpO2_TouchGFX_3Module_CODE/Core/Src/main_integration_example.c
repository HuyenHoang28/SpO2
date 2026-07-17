/*
 * main_integration_example.c
 * File này KHÔNG thay thế nguyên file main.c do CubeMX sinh ra.
 * Hãy copy các đoạn USER CODE vào main.c thật của project TouchGFX.
 */

#include "main.h"
#include "health_monitor.h"

extern I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN 2 */
HealthMonitor_Init(&hi2c1);
/* USER CODE END 2 */

/* USER CODE BEGIN WHILE */
while (1)
{
    /* Nếu project TouchGFX không dùng FreeRTOS */
    MX_TouchGFX_Process();

    static uint32_t lastTick = 0;
    if (HAL_GetTick() - lastTick >= 10) {
        lastTick = HAL_GetTick();
        HealthMonitor_Process10ms();
    }
}
/* USER CODE END WHILE */

/* Nếu project TouchGFX dùng FreeRTOS, đặt HealthMonitor_Process10ms()
 * trong task riêng hoặc trong Model::tick() với bộ chia thời gian.
 */
