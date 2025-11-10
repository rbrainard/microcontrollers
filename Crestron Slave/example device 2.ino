

/*-----( Import needed libraries )-----*/
#include <M5Stack.h>
#include <SoftwareSerial.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        5  //Serial Receive pin
#define SSerialTX        2  //Serial Transmit pin

#define SSerialTxControl 3   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

/*-----( Declare Variables )-----*/
int byteReceived;
int byteSend;
char prevChar = 0;
bool sendOn = false;
bool sendOff = false;

void setup()   /****** SETUP: RUNS ONCE ******/
{
  // Start the built-in serial port, probably to Serial Monitor
  M5.begin(true, false, true);
  //Serial.println("YourDuino.com SoftwareSerial remote loop example");
  //Serial.println("Use Serial Monitor, type in upper window, ENTER");
  M5.Power.begin();
  
  pinMode(SSerialTxControl, OUTPUT);

  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver   

  // Start the software serial port, to another device
  RS485Serial.begin(38400, SWSERIAL_8M2);   // set the data rate 

}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  M5.update();

   if (M5.BtnA.wasPressed()) {
    sendOn = true;
    M5.Lcd.print('-');
    
   }
   if (M5.BtnA.wasReleased()) {
    sendOff = true;
   }

  if (RS485Serial.available())  //Look for data from other Arduino
   { 
    byteReceived = RS485Serial.read();    // Read received byte
    //Serial.write(byteReceived);        // Show on Serial Monitor
    
    if(byteReceived == 0x00 && prevChar == 0x22){
      M5.Lcd.print('.');
            if(sendOn){
              sendOnCmd();
            } else if(sendOff){ 
              sendOffCmd();
            }
            
      sendPing();

      sendOn = false;
      sendOff = false;
    }
    prevChar = byteReceived;
   }

}//--(end main loop )---

void sendPing(){
      delayMicroseconds(100);
      digitalWrite(SSerialTxControl, RS485Transmit); 
      delayMicroseconds(600);
        byte bytes_to_send[3] =  { 0x02, 0x00};
        //softSerial1.listen();
        RS485Serial.write(bytes_to_send,2);
      delayMicroseconds(350);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit  
}

void sendOnCmd(){
      delayMicroseconds(100);
      digitalWrite(SSerialTxControl, RS485Transmit); 
      delayMicroseconds(600);
        byte bytes_to_send[5] =  { 0x02, 0x03, 0x00, 0x00, 0x00};
        //softSerial1.listen();
        RS485Serial.write(bytes_to_send,5);
      delayMicroseconds(350);
}

void sendOffCmd(){
      delayMicroseconds(100);
      digitalWrite(SSerialTxControl, RS485Transmit); 
      delayMicroseconds(600);
        byte bytes_to_send[5] =  { 0x02, 0x03, 0x00, 0x00, 0x80};
        //softSerial1.listen();
        RS485Serial.write(bytes_to_send,5);
      delayMicroseconds(350);
}
