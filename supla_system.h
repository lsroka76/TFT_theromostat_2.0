#ifndef SUPLA_SYSTEM_H
#define SUPLA_SYSTEM_H

#include "callbacks.h"

void supla_system_init();

void supla_system_loop();

short supla_hvac_getPrimaryTemp();
short supla_hvac_getTemperatureRoomMin();
short supla_hvac_getTemperatureRoomMax();
short supla_hvac_getTemperatureHisteresis();
short supla_hvac_getTemperatureSetpointHeat();
short supla_hvac_getTemperatureHisteresisMin();
short supla_hvac_getTemperatureHisteresisMax();
short supla_hvac_getTemperatureFreezeProtection();
short supla_hvac_getTemperatureHeatProtection();
short supla_hvac_getMinOnTimeS();
short supla_hvac_getMinOffTimeS();

bool  supla_hvac_isWeeklyScheduleEnabled();
bool  supla_hvac_isManualModeEnabled();
bool  supla_hvac_isCountdownEnabled();
bool  supla_hvac_isThermostatDisabled();
bool  supla_hvac_isTemperatureSetpointChangeSwitchesToManualMode();
bool  supla_hvac_isAntiFreezeAndHeatProtectionEnabled();

void  supla_hvac_IncreaseHeatingTemperature();
void  supla_hvac_DecreaseHeatingTemperature();
void  supla_hvac_ToggleOffManualWeeklyScheduleModes();

void  supla_hvac_setTemperatureSetpointHeat(short temperature);
void  supla_hvac_setTemperatureHisteresis(short temperature);
void  supla_hvac_setTemperatureRoomMin(short temperature);
void  supla_hvac_setTemperatureRoomMax(short temperature);
void  supla_hvac_setTemperatureHisteresisMin(short temperature);
void  supla_hvac_setTemperatureHisteresisMax(short temperature);
void  supla_hvac_setTemperatureSetpointChangeSwitchesToManualMode(bool enabled);
void  supla_hvac_setAntiFreezeAndHeatProtectionEnabled(bool enabled);
void  supla_hvac_setTemperatureFreezeProtection(short temperature);
void  supla_hvac_setTemperatureHeatProtection(short temperature);
void  supla_hvac_setMinOnTimeS(short seconds);
void  supla_hvac_setMinOffTimeS(short seconds);
//getCountDownTimerEnds()

void  supla_hvac_setActionHandlerCallback(_actionhandler_callback actionhandler_callback);

#endif