#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Unicode.hpp>
#include <string.h>

Screen1View::Screen1View()
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::updateHealthData(const HealthData_t& data)
{
    if (data.valid) {
        Unicode::snprintf(textSpo2Buffer, TEXTSPO2_SIZE, "%ld %%", (long)data.spo2);
        Unicode::snprintf(textBpmBuffer, TEXTBPM_SIZE, "%ld BPM", (long)data.bpm);
    } else {
        Unicode::snprintf(textSpo2Buffer, TEXTSPO2_SIZE, "-- %%");
        Unicode::snprintf(textBpmBuffer, TEXTBPM_SIZE, "-- BPM");
    }

    Unicode::snprintf(textTimeBuffer, TEXTTIME_SIZE, "%02d:%02d:%02d", data.hour, data.minute, data.second);
    Unicode::snprintf(textDateBuffer, TEXTDATE_SIZE, "%02d/%02d/%04d", data.date, data.month, data.year);

    const char* status = HealthMonitor_StatusText(data.status);
    Unicode::snprintf(textStatusBuffer, TEXTSTATUS_SIZE, "%s", status);

    textSpo2.invalidate();
    textBpm.invalidate();
    textTime.invalidate();
    textDate.invalidate();
    textStatus.invalidate();
}
