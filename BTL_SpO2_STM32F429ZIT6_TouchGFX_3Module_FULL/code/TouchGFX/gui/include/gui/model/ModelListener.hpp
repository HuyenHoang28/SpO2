#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
extern "C" {
#include "health_monitor.h"
}

class ModelListener
{
public:
    ModelListener() : model(0) {}
    virtual ~ModelListener() {}
    void bind(Model* m) { model = m; }
    virtual void healthDataChanged(const HealthData_t& data) {}
protected:
    Model* model;
};

#endif
