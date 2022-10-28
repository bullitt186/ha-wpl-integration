#include <mcp_can.h>
#include <SPI.h>
#include "inc/KElsterTable.h"
#include "inc/ElsterTable.inc"

#define SENDER_ID 700
#define SEND_FREQ 2000 // The wait time between sending two messages

MCP_CAN CAN0(5); // Set CS pin


char msgString[128]; // Serial Output String Buffer

byte sendData[7] = {0x31, 0x00, 0xfa, 0x00, 0x0c, 0x00, 0x00};

unsigned long last_send = 0;

void setup()
{
  Serial.begin(115200);
  delay(3000);

  while (CAN_OK != CAN0.begin(MCP_ANY, CAN_20KBPS, MCP_8MHZ)) // init can bus : masks and filters disabled, baudrate, Quarz vom MCP2551
  {
    Serial.println("CAN initialization failed!");
    delay(100);
  }
  Serial.println("CAN initialization ok!");

  // Change to normal mode to allow messages to be transmitted
  
  if(MCP2515_OK == CAN0.setMode(MCP_NORMAL)){
    Serial.println("CAN Mode set to Normal");
  } else {
    Serial.println("Setting CAN mode returned MCP2515_FAIL");
  }


}

void receiveCanMsg()
{
  unsigned long rxId = 0;
  byte len = 0;
  byte rxBuf[8];

  while (CAN_MSGAVAIL == CAN0.checkReceive()) // check if data coming
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
    Serial.println("-----------------------------");
    if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000)
    { // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    }
    else
    {
      for (byte i = 0; i < len; i++)
      {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }
    Serial.println();
  }
  return;
}

void sendCanMsg()
{
  // send data:  ID = SENDER_ID, Standard CAN Frame, Data length = 7 bytes, 'sendData' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(SENDER_ID, 0, 7, sendData);
  if (sndStat == CAN_OK)
  {
    Serial.println("Message Sent Successfully!");
  }
  else
  {
    switch ((int)sndStat)
    {
    case  CAN_GETTXBFTIMEOUT:
      Serial.println("Error Sending Message: CAN_GETTXBFTIMEOUT");
      break;
    case CAN_SENDMSGTIMEOUT:
      Serial.println("Error Sending Message: CAN_SENDMSGTIMEOUT");
      break;
    default:
      Serial.println("Error Sending Message: Unknown Error");
      break;
    }
  }
  return;
}


void loop()
{
  receiveCanMsg();
  if (millis() > last_send + SEND_FREQ)
  {
    sendCanMsg();
    last_send = millis();
  }
}