#include "alarm_led.h"
#include "stm32f4xx_hal.h"

/* STM32F429I-DISCO user LEDs: LD3=PG13 (green), LD4=PG14 (red).
 * GPIO already initialized in MX_GPIO_Init(). */

#define ALARM_TOGGLE_PERIOD_MS   250U    /* 2 Hz blink */

static uint32_t s_last_toggle_ms = 0U;
static bool     s_led_on         = false;

void AlarmLed_Init(void)
{
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14, GPIO_PIN_RESET);
    s_led_on = false;
    s_last_toggle_ms = HAL_GetTick();
}

void AlarmLed_Update(bool alarm_active)
{
    if (!alarm_active)
    {
        if (s_led_on)
        {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14, GPIO_PIN_RESET);
            s_led_on = false;
        }
        return;
    }

    const uint32_t now = HAL_GetTick();
    if ((now - s_last_toggle_ms) >= ALARM_TOGGLE_PERIOD_MS)
    {
        s_last_toggle_ms = now;
        s_led_on = !s_led_on;
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14,
                          s_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
