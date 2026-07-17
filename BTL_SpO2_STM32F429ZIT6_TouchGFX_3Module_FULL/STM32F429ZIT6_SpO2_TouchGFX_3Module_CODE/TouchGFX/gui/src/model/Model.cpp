#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
extern "C" {
#include "health_monitor.h"
}

Model::Model() : modelListener(0)
{
}

void Model::tick()
{
    static int tickDiv = 0;
    tickDiv++;
    /* TouchGFX tick thường là 60Hz; cập nhật GUI khoảng 5 lần/giây */
    if (tickDiv >= 12) {
        tickDiv = 0;
        HealthData_t data = HealthMonitor_GetData();
        if (modelListener) {
            modelListener->healthDataChanged(data);
        }
    }
}
