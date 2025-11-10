#include <M5Stack.h>
#define RX_PIN      22
#define TX_PIN      21  


#define X_LOCAL 40
#define Y_LOCAL 40

#define X_OFF 160
#define Y_OFF 30

int i=0,s=0;
char prevChar=0;
bool sendReq=false;

void header(const char *string, uint16_t color){
    M5.Lcd.fillScreen(color);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
    M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(string, 160, 3, 4);
}

void setup() {
  M5.begin();
  M5.Power.begin();

  header("RS485 A", TFT_BLACK);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setCursor(X_LOCAL, Y_LOCAL,2);
  M5.Lcd.printf("S 1");
  
  //Serial.begin(38400);   
  Serial2.begin(38400, SERIAL_8N1, RX_PIN, TX_PIN); 

    M5.Lcd.printf("S 2");
}


void loop() {
  M5.update();
delay(10);
        Serial2.write(0x0b);
        Serial2.write(0x00);
        
  if(Serial2.available() && false){
    char c = Serial2.read();
    if(c ==0x00 && prevChar == 0x21){
      if(sendReq){
        Serial2.write(0x02);
        Serial2.write(0x03);
        Serial2.write(0x00);
        Serial2.write(0x00);
        Serial2.write(0x00);

        delayMicroseconds(10);
        
        Serial2.write(0x02);
        Serial2.write(0x00);
        sendReq = false;
      } else {
        Serial2.write(0x02);
        Serial2.write(0x00);
      }
      s++;
      Serial2.flush();
    }
    prevChar = c;
  }

  if (M5.BtnA.wasReleased() and sendReq == false) {
    sendReq=true;
  }
  M5.Lcd.setCursor(X_LOCAL, Y_LOCAL,2);
  M5.Lcd.printf("S :%d\n",s);
//  M5.Lcd.setCursor(X_LOCAL+X_OFF, Y_LOCAL,2);
//  M5.Lcd.printf("R :%d\n",i);
//  delay(10);
}
