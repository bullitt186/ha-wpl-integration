#include <Arduino.h>
#include <mcp_can.h>
#include <SPI.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
const int spiCSPin = 17;
byte data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

MCP_CAN CAN0(spiCSPin);     // Set CS to pin GPIO17


void setup() {
  Serial.begin(115200);

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_20KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  Serial.println("MCP2515 Library Receive Example...");

}


void sendCanMsg() {
  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
}

void readCanMsg() {

    if(CAN_MSGAVAIL == CAN0.checkReceive())
    {
        //CAN0.readMsgBuf(&len, buf);
        CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

        Serial.println("-----------------------------");
        Serial.print("Data from ID: 0x");
        Serial.println(rxId, HEX);

        for(int i = 0; i<len; i++)
        {
            Serial.print(rxBuf[i]);
            Serial.print("\t");
        }
        Serial.println();
    }
}

void loop() {
  sendCanMsg();
  readCanMsg();
}