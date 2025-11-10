#include <M5Stack.h>
#include "UI.h"

namespace UI {

void begin() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
}

void showStartup() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Crestron Master");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Initializing...");
}

void updateMainScreen(bool pingEnabled, uint32_t heap, uint32_t tx, uint32_t rx, uint32_t err, uint32_t successPings, uint32_t totalPings, const uint8_t* slaveAddresses, const char** slaveNames, int slaveCount, bool dim1, bool dim2) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Crestron Master");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0, 30);
    M5.Lcd.printf("Ping: %s  Heap: %d\n", pingEnabled ? "ON" : "OFF", heap);
    M5.Lcd.printf("TX:%5d RX:%5d ERR:%4d\n", tx, rx, err);

    uint32_t pingRate = totalPings > 0 ? (successPings * 100 / totalPings) : 0;
    M5.Lcd.printf("Pings:%d/%d (%d%%)\n", successPings, totalPings, pingRate);

    M5.Lcd.println();
    M5.Lcd.println("Slaves:");
    for (int i = 0; i < slaveCount; ++i) {
        const uint8_t addr = slaveAddresses[i];
        M5.Lcd.printf("0x%02X %s\n", addr, slaveNames[i]);
    }

    M5.Lcd.setCursor(0, 220);
    M5.Lcd.printf("A:Ping  B:Dim1%s  C:Dim2%s", dim1 ? "*" : " ", dim2 ? "*" : " ");
}

} // namespace UI
