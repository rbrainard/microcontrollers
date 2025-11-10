/*-----( Import needed libraries )-----*/
#include <M5Stack.h>
#include <SoftwareSerial.h>
#include <Ticker.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        16  //Serial Receive pin
#define SSerialTX        17  //Serial Transmit pin

#define RS485Transmit    HIGH
#define RS485Receive     LOW

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX
Ticker pingTimer;
Ticker replyTimeout;
Ticker repingTimer;

/*-----( Declare Variables )-----*/
int byteReceived;
int byteSend;
char prevChar = 0;
bool sendOn = false;
bool sendOff = false;
int sendingTo = 0;
int state = 0;
long lastByteTime;
int msgTotalLen = 0;
int msgCurrLen = 0;
int configReq = 0;
bool pingEnabled = false;
bool reqToPing = false;
byte slaves[3] = {0x11, 0x0b, 0x0c};
byte slaveStatus[3] = {0x00, 0x00, 0x00};
byte slaveConfigStep[3] = {0x00, 0x00, 0x00};
byte slaveConfigRequest[3] = {0x00, 0x00, 0x00};
bool dimReq1 = false;
bool dimReq2 = false;
bool dimUReq1 = false;
bool dimUReq2 = false;

int buttonState = 0; 

int currentSlaveIndex = 0;
int numberOfSlaves = 3;
long currPingTime = 0;

//DIM8 setup
byte dimSetup1[4] = { 0x00, 0x02, 0x03, 0x00};
byte dimSetup2[10] = { 0x00, 0x08, 0x08, 0x0E, 0x15, 0x31, 0x01, 0x05, 0x18, 0x20}; //placehold. we are really sending a current timestamp
byte dimSetup3[20] = { 0x00, 0x12, 0x1C, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x07, 0x01};
byte dimSetup4[7] = { 0x00, 0x05, 0x14, 0x00, 0x00, 0xFF, 0xFF};
byte dimSetup5[7] = { 0x00, 0x05, 0x14, 0x00, 0x01, 0xFF, 0xFF};
byte dimSetup6[7] = { 0x00, 0x05, 0x14, 0x00, 0x02, 0xFF, 0xFF};
byte dimSetup7[7] = { 0x00, 0x05, 0x14, 0x00, 0x03, 0xFF, 0xFF};
byte dimSetup8[7] = { 0x00, 0x05, 0x14, 0x00, 0x04, 0xFF, 0xFF};
byte dimSetup9[7] = { 0x00, 0x05, 0x14, 0x00, 0x05, 0xFF, 0xFF};
byte dimSetup10[7] = { 0x00, 0x05, 0x14, 0x00, 0x06, 0xFF, 0xFF};
byte dimSetup11[7] = { 0x00, 0x05, 0x14, 0x00, 0x07, 0xFF, 0xFF};
byte dimSetup12[4] = { 0x00, 0x02, 0x03, 0x16};

//DIMU8 setup
byte dimUSetup1[4] = { 0x00, 0x02, 0x03, 0x00};
byte dimUSetup2[10] = { 0x00, 0x08, 0x08, 0x0E, 0x15, 0x31, 0x01, 0x05, 0x18, 0x20}; //placehold. we are really sending a current timestamp
byte dimUSetup3[55] = { 0x00, 0x35, 0x020, 0x03, 0x32, 0x1C, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x05, 0x00, 0x00, 0xFF, 0x0F, 0x00, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x07, 0x00, 0x00, 0xFF, 0xFF};
byte dimUSetup4[55] = { 0x00, 0x35, 0x020, 0x04, 0x32, 0x1C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x01};
byte dimUSetup5[67] = { 0x00, 0x41, 0x020, 0x05, 0x3E, 0x1C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00};
byte dimUSetup6[43] = { 0x00, 0x29, 0x020, 0x05, 0x26, 0x1C, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00};
byte dimUSetup7[4] = { 0x00, 0x02, 0x03, 0x16};

void pingTout() 
{
    reqToPing = true;
}

void repingTout() 
{
    slaveStatus[0] = 0;
    slaveStatus[1] = 0;
    slaveStatus[2] = 0;
}

void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(115200);
  // Start the built-in serial port, probably to Serial Monitor
  M5.begin(true, false, true);

  M5.Power.begin();

  pinMode(2, INPUT);

  // Start the software serial port, to another device
  RS485Serial.begin(38400, SWSERIAL_8N2);   // set the data rate 

  Serial.println("START");
  M5.Lcd.println("START");

}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
    M5.update();

    buttonState = digitalRead(2);
    if(buttonState) M5.Lcd.println("-");
   if (M5.BtnA.wasReleased() and pingEnabled == true) { 
    clearTimers(); 
    pingEnabled = false; 
    clearScreen();
    M5.Lcd.println("PING OFF");
    reqToPing = false;
   } else if (M5.BtnA.wasReleased() and pingEnabled == false) { 
      pingEnabled = true;
      clearScreen();
      M5.Lcd.println("PING ON");
      reqToPing = true;
   }
   


   if (M5.BtnB.wasReleased()) { 
    dimReq1 = true;
    dimUReq1 = true;
   }

   if (M5.BtnC.wasReleased()) { 
    dimReq2 = true;
    dimUReq2 = true;
   }
   
   if(reqToPing){
    startPingCycle();
   }
   
   if(currPingTime > 0 and millis() - currPingTime > 4){
       sendZero();
       //Serial.print("timeout ");
       //Serial.println(sendingTo, HEX);
       slaveStatus[currentSlaveIndex] = 0x00;
       currPingTime = 0;
       turnOnRepingTimer();
       if(pingEnabled) reqToPing = true;
   }
   
  checkResp();
}

void startPingCycle(){
  delay(2);
  if(currentSlaveIndex > numberOfSlaves - 1 and slaveConfigRequest[currentSlaveIndex] == 0) {
      delay(20);
      currentSlaveIndex = 0; 
  }

  if(slaveStatus[currentSlaveIndex] == 0){ //slave is available
    if(slaveConfigRequest[currentSlaveIndex] == 0){ //slave has not requested config
      if(slaves[currentSlaveIndex] == 0x0b and (dimReq1 or dimReq2)){
        if(dimReq1) sendDimCmdRamp1(slaves[currentSlaveIndex]);
        if(dimReq2) sendDimCmdRamp2(slaves[currentSlaveIndex]);
        dimReq1 = false;
        dimReq2 = false;
      } else if(slaves[currentSlaveIndex] == 0x0c and (dimUReq1 or dimUReq2)){
        if(dimUReq1) sendDimUCmdRamp1(slaves[currentSlaveIndex]);
        if(dimUReq2) sendDimUCmdRamp2(slaves[currentSlaveIndex]);
        dimUReq1 = false;
        dimUReq2 = false;        
      }else{
        sendPingRequest(slaves[currentSlaveIndex]);  
        currPingTime = millis();    
        reqToPing = false;
      }
      
    } else { //slave has requests config
      sendConfigRequest(currentSlaveIndex);
      currPingTime = 0;
      reqToPing = true; //leave request true since we dont expect a ping response to the config
    }
  }
  currentSlaveIndex++;
}

void clearTimers(){
  turnOffPinger();
  turnOffRepingTimer();
}

void turnOnPinger(){
  clearTimers();
  if(pingEnabled) pingTimer.once_ms(23, pingTout);//23
}

void turnOffPinger(){
  pingTimer.detach();
}

void turnOnRepingTimer(){
  repingTimer.once_ms(320, repingTout);//320
}

void turnOffRepingTimer(){
  repingTimer.detach();
}

void checkResp(){
  if (RS485Serial.available())  //Look for data from other Arduino
   {     
    lastByteTime = millis();
    byteReceived = RS485Serial.read();    // Read received byte

    if(state == 0)
    { 
      if(byteReceived == 0x02){
        state = 1; //to master
      }
      if(byteReceived != 0x02 && byteReceived != 0xFF && byteReceived != 0x00){
        sendingTo = byteReceived;
        state = 11; //from master
      }
//To Master
    } else if (state == 1){
       if(byteReceived == 0x00) { //ping
        //Serial.println('.');
        currPingTime = 0;
        if(pingEnabled) reqToPing = true;
        state = 0;
      } else { //get length
        Serial.print("s>m ");
        msgTotalLen = byteReceived;
        msgCurrLen = 0;
        Serial.print(sendingTo, HEX);
        Serial.print('.');
        Serial.print(msgTotalLen, HEX);
        Serial.print('.');
         
        state = 2;
      }
    } else if (state == 2){ //get content
      Serial.print(byteReceived, HEX);
      if(msgTotalLen == 0x02 && byteReceived == 3) {
        for(int i = 0; i < numberOfSlaves;i++){
          if(slaves[i] == sendingTo){
            slaveConfigRequest[i] = 1;
            slaveConfigStep[i] = 0;
          }
        }
      }
      msgCurrLen++;
      if(msgCurrLen == msgTotalLen) {
        if(configReq > 0){
          Serial.print("config req");
        }
        state = 0;
        Serial.println('!');
      } else {
        Serial.print('.');
      }
//From Master
    } else if (state == 11){ //ping
       if(byteReceived == 0x00) {
        //Serial.print('.');
        state = 0;
      } else { //get length
        Serial.print("m>s ");
        msgTotalLen = byteReceived;
        msgCurrLen = 0;
        Serial.print(sendingTo, HEX);
        Serial.print('.');
        Serial.print(msgTotalLen, HEX);  
        Serial.print('.');      
        state = 12;
      }
    } else if (state == 12){
      Serial.print(byteReceived, HEX);
      Serial.print('.');
      msgCurrLen++;
      if(msgCurrLen == msgTotalLen) {
        state = 0;
        Serial.println('!');
      }
    }

    prevChar = byteReceived;
   }
}

void sendPingRequest(int slaveID){
  sendingTo = slaveID;
  if(configReq == slaveID){
    sendTimeSync(slaveID);
    configReq = 0;
    delay(2);
  }
  byte bytes_to_send[2] =  { slaveID, 0x00};
  RS485Serial.write(bytes_to_send,2);
}

void sendPingResponse(){
  byte bytes_to_send[3] =  { 0x02, 0x00};
  RS485Serial.write(bytes_to_send,2);
}

void clearScreen(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
}

void sendOnCmd(){
  byte bytes_to_send[5] =  { 0x02, 0x03, 0x00, 0x00, 0x00};
  RS485Serial.write(bytes_to_send,5);
}

void sendOffCmd(){
  byte bytes_to_send[5] =  { 0x02, 0x03, 0x00, 0x00, 0x80};
  RS485Serial.write(bytes_to_send,5);
}

void sendDimCmd1(byte slaveID){
  //SlaveID, Len, ?,?,?,", Channel, Level
  byte bytes_to_send[10] =  { 0x0b, 0x08, 0x1D, 0x00, 0x00, 0x64, 0x00, 0xFF, 0x01, 0xFF};
  RS485Serial.write(bytes_to_send,10);
  Serial.println("Dim On Req");
}

void sendDimCmd2(byte slaveID){
  byte bytes_to_send[10] =  { 0x0b, 0x08, 0x1D, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01, 0x00};
  RS485Serial.write(bytes_to_send,10);
  Serial.println("Dim Off Req");
}

void sendDimCmdRamp1(byte slaveID){
  //SlaveID, Len, ?, Ramp Hi, Ramp Lo, Channel, Level
  byte bytes_to_send[10] =  { 0x0b, 0x08, 0x1D, 0x00, 0x01, 0xF4, 0x00, 0xFF, 0x01, 0xFF};
  RS485Serial.write(bytes_to_send,10);
  Serial.println("Dim On Req");
}

void sendDimCmdRamp2(byte slaveID){
  byte bytes_to_send[10] =  { 0x0b, 0x08, 0x1D, 0x00, 0x01, 0xF4, 0x00, 0x00, 0x01, 0x00};
  RS485Serial.write(bytes_to_send,10);
  Serial.println("Dim Off Req");
}

void sendDimUCmd1(byte slaveID){
    //SlaveID, Len, ?,?,?,?, Ramp Hi, Ramp Lo, Channel, Level
  byte bytes_to_send[13] =  { 0x0c, 0x0b, 0x20, 0x01, 0x06, 0x1D, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0xFF};
  RS485Serial.write(bytes_to_send,13);
  Serial.println("DimU On Req");
}

void sendDimUCmd2(byte slaveID){
  byte bytes_to_send[13] =  { 0x0c, 0x0b, 0x20, 0x01, 0x08, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
  RS485Serial.write(bytes_to_send,13);
  Serial.println("DimU Off Req");
}

void sendDimUCmdRamp1(byte slaveID){
  byte bytes_to_send[13] =  { 0x0c, 0x0b, 0x20, 0x01, 0x08, 0x1D, 0x00, 0x01, 0xF4, 0x00, 0xFF, 0x01, 0xFF};
  RS485Serial.write(bytes_to_send,13);
  Serial.println("Dim On Req");
}

void sendDimUCmdRamp2(byte slaveID){
  byte bytes_to_send[13] =  { 0x0c, 0x0b, 0x20, 0x01, 0x08, 0x1D, 0x00, 0x01, 0xF4, 0x00, 0x00, 0x01, 0x00};
  RS485Serial.write(bytes_to_send,13);
  Serial.println("Dim Off Req");
}

void sendZero(){
  digitalWrite(SSerialTX, false);  // Init Transceiver  
  delayMicroseconds(260);
  digitalWrite(SSerialTX, true);  // Init Transceiver  
}

void sendConfigRequest(byte currentSlaveIndex){
  if(currentSlaveIndex == 0){ //48i at address 0x11
    sendTimeSync(slaves[currentSlaveIndex]);
    slaveConfigRequest[currentSlaveIndex] = 0;
  } else if (currentSlaveIndex == 1 ){ //Dimemrs at address 0x0b
            //Send dimmer config
          
            //Skip
            //slaveConfigStep[currentSlaveIndex] = 12;
            delay(2);
            
            if(slaveConfigStep[currentSlaveIndex] == 0){
                  dimSetup1[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup1,sizeof(dimSetup1));
                  slaveConfigStep[currentSlaveIndex]++;
                  
                  delay(6); 
                  sendPingRequest(0x0b); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(21);
                  
                  //} else if(slaveConfigStep[currentSlaveIndex] == 1){ 
                  sendTimeSync(slaves[currentSlaveIndex]);
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                    
                  delay(1);//} else if(slaveConfigStep[currentSlaveIndex] == 2){
                  dimSetup3[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup3,sizeof(dimSetup3));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 3){
                  dimSetup4[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup4,sizeof(dimSetup4));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  delay(4); 
                  sendPingRequest(0x0b); 
                  delay(4); 
                      
                  //} else if(slaveConfigStep[currentSlaveIndex] == 4){
                  dimSetup5[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup5,sizeof(dimSetup5));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 5){
                  dimSetup6[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup6,sizeof(dimSetup6));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                 
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 6){
                  dimSetup7[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup7,sizeof(dimSetup7));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 7){
                  dimSetup8[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup8,sizeof(dimSetup8));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 8){
                  dimSetup9[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup9,sizeof(dimSetup9));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 9){
                  dimSetup10[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup10,sizeof(dimSetup10));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++; 
                                    
                  delay(4); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 10){
                  dimSetup11[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup11,sizeof(dimSetup11));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++; 
                  
                  delay(2);//} else if(slaveConfigStep[currentSlaveIndex] == 11){
                  dimSetup12[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimSetup12,sizeof(dimSetup12));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  //} else if(slaveConfigStep[currentSlaveIndex] == 12){
                  slaveConfigRequest[currentSlaveIndex] = 0;
              }
            } else if ( currentSlaveIndex == 2){ //Dimemrs at address ox0c
                //Send dimmer config
                delay(2);
                if(slaveConfigStep[currentSlaveIndex] == 0){
                  dimUSetup1[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup1,sizeof(dimUSetup1));
                  slaveConfigStep[currentSlaveIndex]++;
                   
                  delay(15); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  delay(21); 
                  sendPingRequest(0x0b); 
                  delay(4);
                  sendPingRequest(0x11); 
                  delay(21); 
                  
                  //} else if(slaveConfigStep[currentSlaveIndex] == 1){ 
                  sendTimeSync(slaves[currentSlaveIndex]);
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;    
                  
                  delay(4); 
                  sendPingRequest(0x11); 
                  
                  delay(4);//} else if(slaveConfigStep[currentSlaveIndex] == 2){
                  dimUSetup3[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup3,sizeof(dimUSetup3));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;
                  
                  delay(4); 
                  sendPingRequest(0x0c); 
  
                  delay(4);//} else if(slaveConfigStep[currentSlaveIndex] == 3){
                  dimUSetup4[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup4,sizeof(dimUSetup4));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  
                  
                  delay(4); 
                  sendPingRequest(0x0b); 
                  
                  delay(4);//} else if(slaveConfigStep[currentSlaveIndex] == 4){
                  dimUSetup5[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup5,sizeof(dimUSetup5));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  

                  delay(4); 
                  sendPingRequest(0x0b); 
                  
                  delay(4);//} else if(slaveConfigStep[currentSlaveIndex] == 5){
                  dimUSetup6[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup6,sizeof(dimUSetup6));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++;  

                  delay(4); 
                  sendPingRequest(0x11); 
                  
                  delay(4);//} else if(slaveConfigStep[currentSlaveIndex] == 11){
                  dimUSetup7[0] = slaves[currentSlaveIndex];
                  RS485Serial.write(dimUSetup7,sizeof(dimUSetup7));
                  Serial.println(slaveConfigStep[currentSlaveIndex]);
                  slaveConfigStep[currentSlaveIndex]++; 
                   
                  //} else if(slaveConfigStep[currentSlaveIndex] == 12){
                  slaveConfigRequest[currentSlaveIndex] = 0;
                  
                  delay(4); 
                  sendPingRequest(0x11); 
                  delay(21); 
                  //Serial.print("config index");
                  // Serial.println(slaveConfigStep[currentSlaveIndex]);
           
                }
            }
}

void sendTimeSync(byte slaveID){
  byte bytes_to_send[10] =  { slaveID, 0x08, 0x08, 0x0e, 0x15, 0x45, 0x29, 0x05, 0x20, 0x20};
  RS485Serial.write(bytes_to_send,10);
  Serial.print('*');
}

void sendConfig2(byte slaveID){
  byte bytes_to_send[3] =  { slaveID, 0x01, 0x00};
  RS485Serial.write(bytes_to_send,3);
}
