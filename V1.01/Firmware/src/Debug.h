#pragma once
#include "Config.h"

inline bool debugOn(bool category) {
    return DEBUG_ENABLED && category;
}
