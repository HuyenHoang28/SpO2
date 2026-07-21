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

    void updateData(const SpO2UiData& data);

protected:
    bool buttonWasPressed;
    uint32_t lastMeasurementSequence;
};

#endif // SCREEN2VIEW_HPP
