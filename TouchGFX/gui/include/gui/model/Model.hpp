#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>
#include <gui/model/SpO2UiData.hpp>

class ModelListener;

class Model
{
public:
    static const std::uint16_t GRAPH_HISTORY_SIZE = 160U;

    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void copyGraphHistory(float* bpmValues,
                          float* spo2Values,
                          std::uint16_t& count,
                          std::uint32_t& latestMeasurementSequence) const;

protected:
    void appendGraphSample(const SpO2UiData& data);

    ModelListener* modelListener;
    std::uint32_t lastSequence;
    std::uint32_t lastGraphSequence;
    float bpmHistory[GRAPH_HISTORY_SIZE];
    float spo2History[GRAPH_HISTORY_SIZE];
    std::uint16_t historyCount;
    std::uint16_t historyHead;
#ifdef SIMULATOR
    std::uint32_t simulatorTick;
#endif
};

#endif // MODEL_HPP
