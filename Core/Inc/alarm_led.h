#ifndef ALARM_LED_H
#define ALARM_LED_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void AlarmLed_Init(void);
void AlarmLed_Update(bool alarm_active);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_LED_H */
