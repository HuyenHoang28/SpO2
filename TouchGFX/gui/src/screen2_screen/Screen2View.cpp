#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/events/ClickEvent.hpp>

#ifndef SIMULATOR
extern "C"
{
#include "main.h"
}
#endif

Screen2View::Screen2View()
    : buttonWasPressed(false),
      lastMeasurementSequence(0U)
{
}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
#ifndef SIMULATOR
    buttonWasPressed = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
#endif
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::handleTickEvent()
{
#ifndef SIMULATOR
    const bool pressed = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    if (pressed && !buttonWasPressed)
    {
        application().gotoScreen1ScreenNoTransition();
    }
    buttonWasPressed = pressed;
#endif
}

void Screen2View::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        application().gotoScreen1ScreenNoTransition();
    }
}

void Screen2View::updateData(const SpO2UiData& data)
{
    if (!data.measurementValid ||
        (data.measurementSequence == 0U) ||
        (data.measurementSequence == lastMeasurementSequence))
    {
        return;
    }

    lastMeasurementSequence = data.measurementSequence;
    BPM_Graph.addDataPoint(static_cast<float>(data.heartRateBpm));
    SpO2_Graph.addDataPoint(static_cast<float>(data.spo2Percent));
}
