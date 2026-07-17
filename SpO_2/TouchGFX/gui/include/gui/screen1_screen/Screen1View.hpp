#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/model/SpO2UiData.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    void updateData(const SpO2UiData& data);

protected:
    touchgfx::Box topAccent;
    touchgfx::Box spo2Panel;
    touchgfx::Box bpmPanel;
    touchgfx::Box timePanel;
    touchgfx::Box datePanel;
    touchgfx::Box statusPanel;
    bool buttonWasPressed;
};

#endif // SCREEN1VIEW_HPP
