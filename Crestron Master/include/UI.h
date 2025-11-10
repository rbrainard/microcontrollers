// UI.h - M5Station UI helpers
#pragma once

#include <stdint.h>

namespace UI {
    void begin();
    void showStartup();
    void updateMainScreen(bool pingEnabled, uint32_t heap, uint32_t tx, uint32_t rx, uint32_t err, uint32_t successPings, uint32_t totalPings, const uint8_t* slaveAddresses, const char** slaveNames, int slaveCount, bool dim1, bool dim2);
}
