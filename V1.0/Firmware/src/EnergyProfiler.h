#pragma once

#include <Arduino.h>
#include "Config.h"

#if ENERGY_PROFILER_ENABLED
void energyProfilerBegin(bool usbMode);
void energyProfilerLoopBegin();
void energyProfilerBeforeDelay(uint32_t requestedDelayMs);
void energyProfilerEncoderEventFromIsr();
void energyProfilerImuPoll();
void energyProfilerBleReport();
void energyProfilerUsbReport();
void energyProfilerUsbModeChanged(bool usbMode);
void energyProfilerUpdate();
#else
inline void energyProfilerBegin(bool) {}
inline void energyProfilerLoopBegin() {}
inline void energyProfilerBeforeDelay(uint32_t) {}
inline void energyProfilerEncoderEventFromIsr() {}
inline void energyProfilerImuPoll() {}
inline void energyProfilerBleReport() {}
inline void energyProfilerUsbReport() {}
inline void energyProfilerUsbModeChanged(bool) {}
inline void energyProfilerUpdate() {}
#endif
