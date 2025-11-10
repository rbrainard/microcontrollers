/*
 * Interactive Servo Test - Channel 8
 * M5Stack + Servo2 Module
 */

#include <M5Stack.h>
#include <Wire.h>
#include "Adafruit_PWMServoDriver.h"

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define SERVO_CHANNEL 8
#define SERVOMIN 102
#define SERVOMAX 512
#define SERVO_FREQ 50

enum ControlMode {
  MANUAL,
  SWEEP,
  RANDOM,
  WAVE,
  DIAGNOSTIC
};

ControlMode currentMode = MANUAL;
int servoAngle = 90;
unsigned long lastUpdate = 0;
bool sweepDirection = true;

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(SERVO_CHANNEL, 0, pulse);
  servoAngle = angle;
}

void updateDisplay() {
  M5.Lcd.fillScreen(BLACK);
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(70, 10);
  M5.Lcd.println("SERVO CH 8");
  
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(20, 50);
  M5.Lcd.print("Mode: ");
  switch(currentMode) {
    case MANUAL: M5.Lcd.print("MANUAL     "); break;
    case SWEEP:  M5.Lcd.print("SWEEP      "); break;
    case RANDOM: M5.Lcd.print("RANDOM     "); break;
    case WAVE:   M5.Lcd.print("WAVE       "); break;
    case DIAGNOSTIC: M5.Lcd.print("DIAGNOSTIC "); break;
  }
  
  M5.Lcd.setTextSize(6);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(80, 100);
  if (servoAngle < 100) M5.Lcd.print(" ");
  if (servoAngle < 10) M5.Lcd.print(" ");
  M5.Lcd.print(servoAngle);
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("o");
  
  int arcX = 160;
  int arcY = 180;
  int arcR = 50;
  
  for (int a = 0; a <= 180; a += 10) {
    float rad = (a - 90) * PI / 180.0;
    int x = arcX + arcR * cos(rad);
    int y = arcY + arcR * sin(rad);
    M5.Lcd.drawPixel(x, y, DARKGREY);
  }
  
  float angleRad = (servoAngle - 90) * PI / 180.0;
  int lineX = arcX + (arcR - 5) * cos(angleRad);
  int lineY = arcY + (arcR - 5) * sin(angleRad);
  M5.Lcd.drawLine(arcX, arcY, lineX, lineY, RED);
  M5.Lcd.fillCircle(arcX, arcY, 3, WHITE);
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLUE);
  M5.Lcd.fillRect(10, 220, 90, 20, BLUE);
  M5.Lcd.setCursor(25, 222);
  M5.Lcd.print("MODE");
  
  M5.Lcd.setTextColor(WHITE, GREEN);
  M5.Lcd.fillRect(115, 220, 90, 20, GREEN);
  M5.Lcd.setCursor(125, 222);
  if (currentMode == MANUAL) {
    M5.Lcd.print("< -15");
  } else {
    M5.Lcd.print("  -- ");
  }
  
  M5.Lcd.setTextColor(WHITE, RED);
  M5.Lcd.fillRect(220, 220, 90, 20, RED);
  M5.Lcd.setCursor(230, 222);
  if (currentMode == MANUAL) {
    M5.Lcd.print("+15 >");
  } else {
    M5.Lcd.print("  -- ");
  }
}

void setup() {
  M5.begin();
  M5.Power.begin();
  
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== Servo2 Interactive Test ===");
  Serial.println("Channel 8");
  
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() != 0) {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(40, 100);
    M5.Lcd.println("Servo2 NOT FOUND!");
    while(1) delay(1000);
  }
  
  Serial.println("Servo2 connected!");
  
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  
  setServoAngle(90);
  updateDisplay();
  
  Serial.println("Ready!");
  Serial.println("Button A: Change mode");
  Serial.println("Button B: Decrease angle (manual)");
  Serial.println("Button C: Increase angle (manual)");
}

void loop() {
  M5.update();
  
  if (M5.BtnA.wasPressed()) {
    currentMode = (ControlMode)((currentMode + 1) % 5);
    Serial.print("Mode: ");
    Serial.println(currentMode);
    updateDisplay();
  }
  
  if (M5.BtnB.wasPressed()) {
    if (currentMode == MANUAL) {
      servoAngle -= 15;
      if (servoAngle < 0) servoAngle = 0;
      setServoAngle(servoAngle);
      Serial.println("Angle: " + String(servoAngle));
      updateDisplay();
    }
  }
  
  if (M5.BtnC.wasPressed()) {
    if (currentMode == MANUAL) {
      servoAngle += 15;
      if (servoAngle > 180) servoAngle = 180;
      setServoAngle(servoAngle);
      Serial.println("Angle: " + String(servoAngle));
      updateDisplay();
    }
  }
  
  unsigned long now = millis();
  
  if (currentMode == SWEEP && now - lastUpdate > 50) {
    if (sweepDirection) {
      servoAngle += 2;
      if (servoAngle >= 180) {
        servoAngle = 180;
        sweepDirection = false;
      }
    } else {
      servoAngle -= 2;
      if (servoAngle <= 0) {
        servoAngle = 0;
        sweepDirection = true;
      }
    }
    setServoAngle(servoAngle);
    updateDisplay();
    lastUpdate = now;
  }
  
  if (currentMode == RANDOM && now - lastUpdate > 1000) {
    servoAngle = random(0, 181);
    setServoAngle(servoAngle);
    Serial.println("Random: " + String(servoAngle));
    updateDisplay();
    lastUpdate = now;
  }
  
  if (currentMode == WAVE && now - lastUpdate > 30) {
    servoAngle = 90 + 60 * sin(now / 500.0);
    setServoAngle(servoAngle);
    updateDisplay();
    lastUpdate = now;
  }
  
  // DIAGNOSTIC mode - tests all channels with extreme positions
  if (currentMode == DIAGNOSTIC && now - lastUpdate > 2000) {
    static int diagStep = 0;
    
    if (diagStep == 0) {
      Serial.println("DIAGNOSTIC: Setting all channels to MIN");
      for (int i = 0; i < 16; i++) {
        pwm.setPWM(i, 0, SERVOMIN);
      }
      diagStep = 1;
    } else {
      Serial.println("DIAGNOSTIC: Setting all channels to MAX");
      for (int i = 0; i < 16; i++) {
        pwm.setPWM(i, 0, SERVOMAX);
      }
      diagStep = 0;
    }
    lastUpdate = now;
  }
  
  delay(10);
}
