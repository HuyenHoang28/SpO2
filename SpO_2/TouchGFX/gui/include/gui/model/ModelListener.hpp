#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/model/SpO2UiData.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void onSpO2DataChanged(const SpO2UiData& data)
    {
        (void)data;
    }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
