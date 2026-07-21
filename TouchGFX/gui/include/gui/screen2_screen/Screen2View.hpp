#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/model/SpO2UiData.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    void preloadGraphHistory(const float* bpmValues,
                             const float* spo2Values,
                             uint16_t count,
                             uint32_t latestSequence);
    void updateData(const SpO2UiData& data);

    void appendGraphSample(float bpmValue, float spo2Value, bool interpolateFromPrevious);

protected:
    bool buttonWasPressed;
    uint32_t lastMeasurementSequence;
    bool hasPreviousGraphSample;
    bool interpolationPhase;
    float previousBpmSample;
    float previousSpo2Sample;
};

#endif // SCREEN2VIEW_HPP
