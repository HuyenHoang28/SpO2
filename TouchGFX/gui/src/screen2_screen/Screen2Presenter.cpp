#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{
    if (model != 0)
    {
        float bpmHistory[Model::GRAPH_HISTORY_SIZE];
        float spo2History[Model::GRAPH_HISTORY_SIZE];
        uint16_t historyCount = 0U;
        uint32_t latestSequence = 0U;

        model->copyGraphHistory(bpmHistory,
                                spo2History,
                                historyCount,
                    latestSequence);
        view.preloadGraphHistory(bpmHistory,
                                 spo2History,
                                 historyCount,
                     latestSequence);
    }

}

void Screen2Presenter::deactivate()
{

}

void Screen2Presenter::onSpO2DataChanged(const SpO2UiData& data)
{
    view.updateData(data);
}
