// MCP2515 - ESP32
//    SCK  - IO18
//    SI   - IO23
//    SO   - IO19
//    CS   - IO5
//    Vcc  - 5V ! Der ESP32 hat 5V-feste Eingänge
//    GND  - GND
//
// http://henrysbench.capnfatz.com/henrys-bench/arduino-projects-tips-and-more/arduino-can-bus-module-1st-network-tutorial/
// CAN Receive Example der Bibliothek https://github.com/coryjfowler/MCP_CAN_lib
#include <mcp_can.h>
#include <SPI.h>

MCP_CAN CAN0(5); // Set CS pin

char msgString[128]; // Serial Output String Buffer
const int LEDpin = 13;

void setup()
{
  Serial.begin(115200);
  delay(3000);
  pinMode(LEDpin, OUTPUT);

  while (CAN_OK != CAN0.begin(MCP_ANY, CAN_20KBPS, MCP_8MHZ)) // init can bus : masks and filters disabled, baudrate, Quarz vom MCP2551
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");
  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
}

void loop()
{
  unsigned long rxId = 0;
  byte len = 0;
  byte rxBuf[8];

  if (CAN_MSGAVAIL == CAN0.checkReceive()) // check if data coming
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
        if (i == 2)
        {
          digitalWrite(LEDpin, rxBuf[i] % 2);
        }
      }
    }
    Serial.println();
  }
}