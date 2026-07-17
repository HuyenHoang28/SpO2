#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>
#include <gui/model/SpO2UiData.hpp>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

protected:
    ModelListener* modelListener;
    uint32_t lastSequence;
#ifdef SIMULATOR
    uint32_t simulatorTick;
#endif
};

#endif // MODEL_HPP
