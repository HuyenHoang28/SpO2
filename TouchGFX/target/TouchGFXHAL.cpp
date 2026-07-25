/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  * Description        : STM32F429I-DISCO TouchGFX HAL integration.
  ******************************************************************************
  */

#include <TouchGFXHAL.hpp>
#include <stdint.h>
#include "stm32f4xx_hal.h"

using namespace touchgfx;

extern "C" uint16_t* TouchGFX_GetFrameBuffer(void);
extern "C" volatile uint32_t g_touchgfxFrameCounter;

volatile uint32_t g_touchgfxFrameCounter = 0U;

void TouchGFXHAL::initialize()
{
    /* Use the generated TouchGFX 4.26 initialization without replacing its
       synchronization state machine. */
    TouchGFXGeneratedHAL::initialize();

    /* The application uses one RGB565 framebuffer in external SDRAM. */
    (void)setFrameRefreshStrategy(REFRESH_STRATEGY_DEFAULT);

    uint16_t* const frameBuffer = TouchGFX_GetFrameBuffer();
    LTDC_Layer1->CFBAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(frameBuffer));
    LTDC->SRCR = LTDC_SRCR_IMR;
}

void TouchGFXHAL::taskEntry()
{
    /* Run the official TouchGFX event loop. It configures/enables LTDC IRQ,
       waits for VSYNC through OSWrappers and invokes HAL::tick(). */
    TouchGFXGeneratedHAL::taskEntry();
}

uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return TouchGFX_GetFrameBuffer();
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    (void)address;

    /* Single-buffer configuration: LTDC must always scan the framebuffer
       placed by the linker at 0xD0000000. */
    uint16_t* const frameBuffer = TouchGFX_GetFrameBuffer();
    LTDC_Layer1->CFBAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(frameBuffer));
    LTDC->SRCR = LTDC_SRCR_IMR;
}

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
    ++g_touchgfxFrameCounter;

    /* PG13/PG14 are owned by the SpO2 alarm module (AlarmLed_*) and must
       not be driven from the GUI flush path. */
}

bool TouchGFXHAL::sampleKey(uint8_t& key)
{
    static GPIO_PinState previousState = GPIO_PIN_RESET;
    const GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    const bool pressed = (currentState == GPIO_PIN_SET) &&
                         (previousState == GPIO_PIN_RESET);
    previousState = currentState;

    if (pressed)
    {
        key = 0U;
        return true;
    }
    return false;
}

void TouchGFXHAL::configureInterrupts()
{
    TouchGFXGeneratedHAL::configureInterrupts();
}

void TouchGFXHAL::enableInterrupts()
{
    TouchGFXGeneratedHAL::enableInterrupts();
}

void TouchGFXHAL::disableInterrupts()
{
    TouchGFXGeneratedHAL::disableInterrupts();
}

void TouchGFXHAL::enableLCDControllerInterrupt()
{
    TouchGFXGeneratedHAL::enableLCDControllerInterrupt();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
