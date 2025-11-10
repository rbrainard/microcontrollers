#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "M5PWR485MotorController.h"

class MotorWebServer {
private:
    AsyncWebServer _server;
    UnitRoller485Controller* _motorController;
    
    // Web server handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleConfig(AsyncWebServerRequest *request);
    void handleMotorControl(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handlePositionControl(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleCurrentControl(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleStop(AsyncWebServerRequest *request);
    void handleResetPosition(AsyncWebServerRequest *request);
    void handleEnableMotor(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    
    // Utility methods
    String motorStatusToJson();
    String motorConfigToJson();
    void setCORSHeaders(AsyncWebServerResponse *response);
    
public:
    MotorWebServer(UnitRoller485Controller* motorController);
    
    void begin();
};