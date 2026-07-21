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
    lastMeasurementSequence(0U),
    hasPreviousGraphSample(false),
    interpolationPhase(false),
    previousBpmSample(0.0f),
    previousSpo2Sample(0.0f)
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

void Screen2View::preloadGraphHistory(const float* bpmValues,
                                      const float* spo2Values,
                                      uint16_t count,
                                      uint32_t latestSequence)
{
    BPM_Graph.clear();
    SpO2_Graph.clear();

    if ((bpmValues == 0) || (spo2Values == 0) || (count == 0U))
    {
        lastMeasurementSequence = latestSequence;
        hasPreviousGraphSample = false;
        interpolationPhase = false;
        return;
    }

    for (uint16_t i = 0U; i < count; ++i)
    {
        BPM_Graph.addDataPoint(bpmValues[i]);
        SpO2_Graph.addDataPoint(spo2Values[i]);
    }

    lastMeasurementSequence = latestSequence;
    hasPreviousGraphSample = true;
    interpolationPhase = false;
    previousBpmSample = bpmValues[count - 1U];
    previousSpo2Sample = spo2Values[count - 1U];
    BPM_Graph.invalidate();
    SpO2_Graph.invalidate();
}

void Screen2View::appendGraphSample(float bpmValue,
                                    float spo2Value,
                                    bool interpolateFromPrevious)
{
    if (interpolateFromPrevious && hasPreviousGraphSample && interpolationPhase)
    {
        BPM_Graph.addDataPoint((previousBpmSample + bpmValue) * 0.5f);
        SpO2_Graph.addDataPoint((previousSpo2Sample + spo2Value) * 0.5f);
    }

    BPM_Graph.addDataPoint(bpmValue);
    SpO2_Graph.addDataPoint(spo2Value);
    previousBpmSample = bpmValue;
    previousSpo2Sample = spo2Value;
    hasPreviousGraphSample = true;
    interpolationPhase = !interpolationPhase;
}

void Screen2View::updateData(const SpO2UiData& data)
{
    if (!data.measurementValid ||
        (data.sequence == 0U) ||
        (data.sequence == lastMeasurementSequence))
    {
        return;
    }

    lastMeasurementSequence = data.sequence;
    appendGraphSample(static_cast<float>(data.heartRateBpm),
                      static_cast<float>(data.spo2Percent),
                      true);
}
