#include "MotorWebServer.h"
#include <unit_rolleri2c.hpp>

// External globals from main.cpp
extern UnitRollerI2C* g_motorI2C;
extern bool* g_useI2CMode;

MotorWebServer::MotorWebServer(UnitRoller485Controller* motorController) 
    : _motorController(motorController), _server(80) {
}

void MotorWebServer::begin() {
    // Serve static web page
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    // API endpoints
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });

    _server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleConfig(request);
    });

    _server.on("/api/control/speed", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleMotorControl(request, data, len);
        });

    _server.on("/api/control/position", HTTP_POST,
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handlePositionControl(request, data, len);
        });

    _server.on("/api/control/current", HTTP_POST,
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleCurrentControl(request, data, len);
        });

    _server.on("/api/control/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleStop(request);
    });

    _server.on("/api/control/reset_position", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleResetPosition(request);
    });

    _server.on("/api/motor/enable", HTTP_POST,
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleEnableMotor(request, data, len);
        });

    // Enable CORS for all routes
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    _server.begin();
}

void MotorWebServer::handleRoot(AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5 PWR485 Motor Controller</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; }
        .control-group { background: #f8f9fa; padding: 15px; border-radius: 5px; border: 1px solid #dee2e6; }
        button { background: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; margin: 5px; }
        button:hover { background: #0056b3; }
        button.stop { background: #dc3545; }
        input[type="number"] { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ddd; border-radius: 3px; }
        .error { color: #dc3545; font-weight: bold; }
        .success { color: #28a745; font-weight: bold; }
        #message { margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>M5 PWR485 Motor Controller</h1>
        
        <div id="message"></div>
        
        <div class="status" id="status">
            <h3>Motor Status</h3>
            <div id="statusContent">Loading...</div>
        </div>
        
        <div class="controls">
            <div class="control-group">
                <h4>Speed Control</h4>
                <input type="number" id="speedValue" placeholder="Speed (RPM)" value="100">
                <button onclick="setSpeed()">Set Speed</button>
            </div>
            
            <div class="control-group">
                <h4>Position Control</h4>
                <input type="number" id="positionValue" placeholder="Position" value="1000">
                <input type="number" id="positionSpeed" placeholder="Speed (RPM)" value="500">
                <button onclick="setPosition()">Set Position</button>
            </div>
            
            <div class="control-group">
                <h4>Motor Control</h4>
                <button class="stop" onclick="stopMotor()">Stop Motor</button>
                <button onclick="resetPosition()">Reset Position</button>
            </div>
        </div>
    </div>

    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('statusContent').innerHTML =
                        '<strong>Running:</strong> ' + (data.isRunning ? 'Yes' : 'No') + '<br>' +
                        '<strong>Mode:</strong> ' + data.mode + '<br>' +
                        '<strong>Position:</strong> ' + data.position + '<br>' +
                        '<strong>Voltage:</strong> ' + data.voltage + ' V<br>' +
                        '<strong>Current:</strong> ' + data.current + ' A<br>' +
                        '<strong>Temperature:</strong> ' + data.temperature + ' °C<br>' +
                        (data.errorCode ? '<strong class="error">Error:</strong> ' + data.errorMessage + '<br>' : '');
                })
                .catch(error => {
                    document.getElementById('statusContent').innerHTML = '<span class="error">Error loading status</span>';
                });
        }

        function showMessage(message, isError = false) {
            const messageDiv = document.getElementById('message');
            messageDiv.innerHTML = '<div class="' + (isError ? 'error' : 'success') + '">' + message + '</div>';
            setTimeout(() => messageDiv.innerHTML = '', 3000);
        }

        function setSpeed() {
            const speed = document.getElementById('speedValue').value;
            fetch('/api/control/speed', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ speed: parseInt(speed) })
            })
            .then(response => response.json())
            .then(data => {
                showMessage(data.message, !data.success);
                updateStatus();
            })
            .catch(error => showMessage('Error setting speed', true));
        }

        function setPosition() {
            const position = document.getElementById('positionValue').value;
            const speed = document.getElementById('positionSpeed').value;
            fetch('/api/control/position', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ position: parseInt(position), speed: parseInt(speed) })
            })
            .then(response => response.json())
            .then(data => {
                showMessage(data.message, !data.success);
                updateStatus();
            })
            .catch(error => showMessage('Error setting position', true));
        }

        function stopMotor() {
            fetch('/api/control/stop', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    showMessage(data.message, !data.success);
                    updateStatus();
                })
                .catch(error => showMessage('Error stopping motor', true));
        }

        function resetPosition() {
            fetch('/api/control/reset_position', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    showMessage(data.message, !data.success);
                    updateStatus();
                })
                .catch(error => showMessage('Error resetting position', true));
        }

        // Update status every 2 seconds
        setInterval(updateStatus, 2000);
        
        // Initial status update
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    setCORSHeaders(response);
    request->send(response);
}

void MotorWebServer::handleStatus(AsyncWebServerRequest *request) {
    String json = motorStatusToJson();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    setCORSHeaders(response);
    request->send(response);
}

void MotorWebServer::handleConfig(AsyncWebServerRequest *request) {
    String json = motorConfigToJson();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    setCORSHeaders(response);
    request->send(response);
}

void MotorWebServer::handleMotorControl(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    Serial.println("handleMotorControl called");
    
    if (len == 0) {
        Serial.println("No body data received");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"No body data\"}");
        return;
    }
    
    String body = String((char*)data).substring(0, len);
    Serial.printf("Received body: %s\n", body.c_str());
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.printf("JSON parse error: %s\n", error.c_str());
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("speed")) {
        Serial.println("No speed key in JSON");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing speed parameter\"}");
        return;
    }
    
    int32_t speed = doc["speed"];
    bool success = false;
    
    Serial.printf("I2C Mode: %s\n", (*g_useI2CMode) ? "YES" : "NO");
    
    if (*g_useI2CMode && g_motorI2C) {
        // I2C Mode - M5Stack library handles scaling internally
        // Speed value should be in units (speed*100), so 100 RPM = 10000
        int32_t speedValue = speed * 100;
        
        // First set mode to speed mode and enable output
        g_motorI2C->setMode(ROLLER_MODE_SPEED);
        delay(10);
        g_motorI2C->setOutput(1);  // Enable motor output
        delay(10);
        g_motorI2C->setSpeed(speedValue);  // Library expects value*100
        
        Serial.printf("I2C: Setting speed to %d RPM (raw value: %d), mode: SPEED, output: ENABLED\n", 
                     speed, speedValue);
        success = true;
    } else {
        // RS485 Mode
        success = _motorController->setVelocityMode(speed);
        Serial.printf("RS485: Setting speed to %d RPM, result: %s\n", speed, success ? "SUCCESS" : "FAILED");
    }
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Speed set successfully" : "Failed to set speed") + "\"}";
    
    request->send(200, "application/json", responseJson);
}

void MotorWebServer::handlePositionControl(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    if (len == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"No body data\"}");
        return;
    }
    
    String body = String((char*)data).substring(0, len);
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("position")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing position parameter\"}");
        return;
    }
    
    int32_t position = doc["position"];
    int32_t speed = doc.containsKey("speed") ? doc["speed"] : 500;
    
    bool success = false;
    if (*g_useI2CMode && g_motorI2C) {
        // I2C Mode - M5Stack library handles scaling
        int32_t positionValue = position * 100;  // Library expects value*100
        int32_t maxCurrent = 100000;  // Default max current: 1A (100000 = 1.0A * 100 * 1000)
        
        // Set mode to position mode
        g_motorI2C->setMode(ROLLER_MODE_POSITION);
        delay(10);
        
        // Set the target position
        g_motorI2C->setPos(positionValue);
        delay(10);
        
        // Set maximum current for position control (limits speed/torque)
        g_motorI2C->setPosMaxCurrent(maxCurrent);
        delay(10);
        
        // Enable motor output
        g_motorI2C->setOutput(1);
        
        Serial.printf("I2C: Setting position to %d (raw: %d), max current: %d (1A)\n", 
                     position, positionValue, maxCurrent);
        success = true;
    } else {
        // RS485 Mode
        success = _motorController->setPositionMode(position, speed);
    }
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Position set successfully" : "Failed to set position") + "\"}";
    
    request->send(200, "application/json", responseJson);
}

void MotorWebServer::handleCurrentControl(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    if (len == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"No body data\"}");
        return;
    }
    
    String body = String((char*)data).substring(0, len);
    Serial.printf("Received current control body: %s\n", body.c_str());
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.printf("JSON parse error: %s\n", error.c_str());
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("current")) {
        Serial.println("No current key in JSON");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing current parameter\"}");
        return;
    }
    
    // Current in mA (e.g., 500 for 0.5A)
    int32_t current = doc["current"];
    
    bool success = false;
    if (*g_useI2CMode && g_motorI2C) {
        // I2C Mode - M5Stack library expects current in units (current_mA * 100)
        // User sends current in mA (e.g., 500 for 0.5A)
        // Library expects: 500mA * 100 = 50000
        int32_t currentValue = current * 100;
        
        // Set mode to current mode
        g_motorI2C->setMode(ROLLER_MODE_CURRENT);
        delay(10);
        
        // Set the target current
        g_motorI2C->setCurrent(currentValue);
        delay(10);
        
        // Enable motor output
        g_motorI2C->setOutput(1);
        
        Serial.printf("I2C: Setting current to %d mA (raw: %d), mode: CURRENT, output: ENABLED\n", 
                     current, currentValue);
        success = true;
    } else {
        // RS485 Mode
        success = _motorController->setCurrentMode(current / 1000.0);  // Convert mA to A
        Serial.printf("RS485: Setting current to %.3f A\n", current / 1000.0);
    }
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Current set successfully" : "Failed to set current") + "\"}";
    
    request->send(200, "application/json", responseJson);
}

void MotorWebServer::handleStop(AsyncWebServerRequest *request) {
    bool success = false;
    if (*g_useI2CMode && g_motorI2C) {
        // I2C Mode - disable output
        g_motorI2C->setOutput(0);  // Disable motor output
        Serial.println("I2C: Stopping motor (output disabled)");
        success = true;
    } else {
        // RS485 Mode
        success = _motorController->disableMotor();
    }
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Motor stopped successfully" : "Failed to stop motor") + "\"}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseJson);
    setCORSHeaders(response);
    request->send(response);
}

void MotorWebServer::handleResetPosition(AsyncWebServerRequest *request) {
    bool success = _motorController->resetPosition();
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Position reset successfully" : "Failed to reset position") + "\"}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseJson);
    setCORSHeaders(response);
    request->send(response);
}

void MotorWebServer::handleEnableMotor(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    if (len == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"No body data\"}");
        return;
    }
    
    String body = String((char*)data).substring(0, len);
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    bool enable = doc["enable"];
    bool success = enable ? _motorController->enableMotor() : _motorController->disableMotor();
    
    String responseJson = "{\"success\":" + String(success ? "true" : "false") + 
                        ",\"message\":\"" + (success ? "Motor state changed successfully" : "Failed to change motor state") + "\"}";
    
    request->send(200, "application/json", responseJson);
}

String MotorWebServer::motorStatusToJson() {
    StaticJsonDocument<512> doc;
    
    if (*g_useI2CMode && g_motorI2C) {
        // I2C Mode - read directly from I2C using M5Stack library
        // Library returns values scaled by 100
        int32_t rawVoltage = g_motorI2C->getVin();  // Returns value*100 (e.g., 1200 for 12.0V)
        int32_t rawCurrent = g_motorI2C->getCurrentReadback();  // Returns value*100 (e.g., 50 for 0.5A)
        int32_t rawSpeed = g_motorI2C->getSpeedReadback();  // Returns value*100 (e.g., 10000 for 100 RPM)
        int32_t rawPosition = g_motorI2C->getPosReadback();  // Returns value*100
        int32_t rawTemp = g_motorI2C->getTemp();  // Returns value*100 (e.g., 2500 for 25.0°C)
        uint8_t mode = g_motorI2C->getMotorMode();  // 1=Speed, 2=Position, 3=Current, 4=Encoder
        uint8_t output = g_motorI2C->getOutputStatus();  // 0=disabled, 1=enabled
        
        Serial.printf("[Status] Raw - V:%d, I:%d, Speed:%d, Pos:%d, Temp:%d, Mode:%d, Output:%d\n", 
                     rawVoltage, rawCurrent, rawSpeed, rawPosition, rawTemp, mode, output);
        
        doc["isRunning"] = (output == 1);
        
        String modeStr = "Unknown";
        switch(mode) {
            case 1: modeStr = "Speed"; break;
            case 2: modeStr = "Position"; break;
            case 3: modeStr = "Current"; break;
            case 4: modeStr = "Encoder"; break;
        }
        doc["mode"] = modeStr;
        
        doc["position"] = rawPosition / 100;  // Convert to actual position
        doc["velocity"] = rawSpeed / 100.0;  // Convert to RPM
        doc["voltage"] = rawVoltage / 100.0; // Convert to Volts
        doc["current"] = rawCurrent / 100.0; // Convert to Amps
        doc["temperature"] = rawTemp / 100.0; // Convert to °C
        doc["errorCode"] = g_motorI2C->getErrorCode();
        doc["errorMessage"] = "OK (I2C Mode)";
    } else {
        // RS485 Mode - use existing motor controller
        MotorStatus status = _motorController->getStatus();
        doc["isRunning"] = status.isRunning;
        
        String mode;
        switch (status.currentMode) {
            case MODE_VELOCITY: mode = "Velocity"; break;
            case MODE_POSITION: mode = "Position"; break;
            case MODE_CURRENT: mode = "Current"; break;
            case MODE_DISABLE: mode = "Disabled"; break;
            default: mode = "Unknown"; break;
        }
        doc["mode"] = mode;
        
        doc["position"] = status.position;
        doc["velocity"] = status.velocity;
        doc["voltage"] = status.voltage;
        doc["current"] = status.current;
        doc["temperature"] = status.temperature;
        doc["errorCode"] = status.errorCode;
        doc["errorMessage"] = status.errorCode != 0 ? "Motor Error" : "OK";
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String MotorWebServer::motorConfigToJson() {
    MotorConfig config = _motorController->getMotorConfig();
    
    StaticJsonDocument<512> doc;
    doc["motorId"] = config.motorId;
    doc["maxVelocity"] = config.maxVelocity;
    doc["acceleration"] = config.acceleration;
    doc["maxCurrent"] = config.maxCurrent;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void MotorWebServer::setCORSHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}