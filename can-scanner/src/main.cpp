#include <mcp_can.h>
#include <SPI.h>
#include <Arduino.h>
#include <stdio.h>
#include "inc/KElsterTable.h"
#include "inc/ElsterTable.inc"
#include "esp_log.h"
#include "CanMembers.h"

#define SENDER_ID 0x700
#define SEND_FREQ 2000 // The wait time between sending two messages

MCP_CAN CAN0(5); // Set CS pin

bool canActive = true; // Indicates whether CAN has been set up correctly
bool mcp2515Connected = true; // If set to false, CAN setup and communications is omitted. (Useful for local development & testing)

byte sendData[] = {0xC1, 0x01, 0x69, 0x00, 0x00, 0x00, 0x00};

const int canMembersToScan[] = {0x180, 0x500, 0x480};
const int canMembersToScanCount = 3;

const int elsterTableSize = 3610;
//const int elsterTableSize = 4;

unsigned long last_send = 0;

void setup()
{

  //strcpy(canMembers[0].name, "Kessel");
  //canMembers[0].id = 0x180;

  //strcpy(canMembers[1].name, "Heizmodul");
  //canMembers[1].id = 0x500;

  //strcpy(canMembers[2].name, "Manager");
  //canMembers[2].id = 0x480;


  Serial.begin(115200);
  
  delay(3000);

  if(mcp2515Connected)
  {
    ESP_LOGI("CAN", "Begin CAN initialization");
    if (CAN_OK == CAN0.begin(MCP_ANY, CAN_20KBPS, MCP_8MHZ)) // init can bus : masks and filters disabled, baudrate, Quarz vom MCP2551
    {
      ESP_LOGI("CAN", "CAN initialization ok");
      canActive = true;

      // Change to normal mode to allow messages to be transmitted
      if(MCP2515_OK == CAN0.setMode(MCP_NORMAL)){
        ESP_LOGI("CAN", "CAN Mode set to Normal");
      } else {
        ESP_LOGW("CAN", "Setting CAN mode returned MCP2515_FAIL");
      }
    }
    else {
        ESP_LOGW("CAN", "CAN initialization failed");
        canActive = false;
    }
  }
  else {
    ESP_LOGI("CAN", "Skipping CAN setup as no MCP2515 is connected");
    canActive = false;
  }


}

void canIdToBytes(unsigned short id, msgType type, byte *byte0, byte *byte1)
{ 
  // nulle die letzten 4 bit der id
  // Teile die resultierende Zahl durch 8
  // Byte 0: die resultierende Zahl als Hex ist das Byte 1 (noch ohne send/receive)
  *byte0 = (id & 0xff00) / 8;

  // Setze typ in den letzten 4 Bit von Byte 1
  *byte0 = *byte0 | type;

  // Byte 1: Nimm von der ursprünglichen Dezimalzahl nur die letzten 4 bit
  // Byte 1: schreibe diese letzten 4 Bit in das Byte 1
  *byte1 = id & 0x000f;
  
  //ESP_LOGD("canIdToBytes", "Byte0 = %02x, Byte1 = %02x", *byte0, *byte1);

  return;
}

void indexToBytes(unsigned short index, byte *byte0, byte *byte1, byte *byte2)
{
  //An der 3. Stelle steht nicht notwendigerweise "fa" ("ERWEITERUNGSTELEGRAMM" siehe Elster-Tabelle). Wenn ein Elster-Index 2-stellig ist, also kleiner oder gleich ff ist, dann darf der Index dort direkt eingesetzt werden. Das Resultat erhält man dann im 4. und 5. Byte. 
  if(index < 0xff)
  {
    *byte0 = index & 0xff;
    *byte1 = 0x00;
    *byte2 = 0x00;
  }
  else
  {
    *byte0 = 0xfa;
    *byte1 = (index & 0xff00) >> 8;
    *byte2 = index & 0x00ff;
  }
  //ESP_LOGD("indexToBytes", "Byte0 = %02x, Byte1 = %02x, Byte2 = %02x", *byte0, *byte1, *byte2);

  return;
}

void printCanMsg(const char* tag, byte *msg)
{
      ESP_LOGI(tag, "%02x %02x %02x %02x %02x %02x %02x", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6]);
}

void sendCanMsg(byte *msg)
{
  if(canActive)
  {
    // send data:  ID = SENDER_ID, Standard CAN Frame, Data length = 7 bytes, 'msg' = array of data bytes to send
    byte sndStat = CAN0.sendMsgBuf(SENDER_ID, 0, 7, msg);
    if (sndStat == CAN_OK)
    {
      ESP_LOGI("CAN", "Message Sent Successfully!");
    }
    else
    {
      switch ((int)sndStat)
      {
      case  CAN_GETTXBFTIMEOUT:
        ESP_LOGW("CAN", "Error Sending Message: CAN_GETTXBFTIMEOUT");
        break;
      case CAN_SENDMSGTIMEOUT:
        ESP_LOGW("CAN", "Error Sending Message: CAN_SENDMSGTIMEOUT");
        break;
      default:
        ESP_LOGW("CAN", "Error Sending Message: Unknown Error");
        break;
      }
    }
    return;
  }
  return;
}

// Returns the index of KnownCanMembers if the CAN ID is a known. Returns 0 if the CAN ID is not known.
int lookupCanID(unsigned short id)
{
  int retVal = 0;
  for(int i = 0; i < KnownCanMembersCount; i++)
  {
    if(id == KnownCanMembers[i].id)
    {
      retVal = i;
      break;
    }
  }
  return retVal;
}

// Returns the index (position) within the ElsterTable if the given ElsterIndex is found. Returns 0 if the ElsterIndex is not found.
int lookupElsterIndex(unsigned short index)
{
  int retVal = 0;
  for(int i = 0; i < elsterTableSize; i++)
  {
    if(index == ElsterTable[i].Index)
    {
      retVal = i;
      break;
    }
  }
  return retVal;
}

void scan(canMember members[], int membersCount)
{
  for(int i = 0; i < membersCount; i++)
  {
    // Set Can ID
    canIdToBytes(members[i].id, t_read, &sendData[0], &sendData[1]);

    for(int j = 0; j < elsterTableSize; j++){
      ESP_LOGI("SCAN", "%s (%x):\t%s (0x%04x):", members[i].name, members[i].id, ElsterTable[j].Name, ElsterTable[j].Index);
      // Set Index to request and zero payload bytes.
      indexToBytes(ElsterTable[j].Index, &sendData[2], &sendData[3], &sendData[4]);
      sendData[5] = 0;
      sendData[6] = 0;
      printCanMsg("SCAN", sendData);
      sendCanMsg(sendData);

    }
    delay(10000);
  }
}


//ToDo Test this Function
bool decomposeMsg(byte *msg, const char *memberName, unsigned short *memberId, ElsterIndex *index, msgType *type)
{
  bool retVal = false;

  // Determine canMember ID
  *memberId = ((msg[0] & 0xf0) * 8) + (msg[1] & 0x0f);

  // Determine canMember Name
  int knownCanMembersIndex = lookupCanID(*memberId);
  if(knownCanMembersIndex)
  {
    memberName = KnownCanMembers[knownCanMembersIndex].name;
  }

  // Determine Message Type
  *type = (msgType)(msg[0] & 0x0f);

  // Determine ElsterIndex
  if(msg[2] == 0xfa)
  {
    index->Index = (msg[3] << 8) | msg[4];
  }
  else
  {
    index->Index = msg[2];
  }

  // Determine ElterIndex Name & Type
  int elsterIndexIndex = lookupElsterIndex(index->Index);
  if(elsterIndexIndex)
  {
    index->Name = ElsterTable[elsterIndexIndex].Name;
    index->Type = ElsterTable[elsterIndexIndex].Type;
  }

  //printCanMsg("DECOMP", msg);
  ESP_LOGI("DECOMP", "M.name = %s,\tM.id = 0x%04x,\tEIndex.Name = %s,\tEIndex.Index = 0x%04x,\tEIndex.Type = %0u,\tMsgType = %d", memberName, (unsigned int)*memberId, index->Name, index->Index, index->Type, *type);
  return retVal;
}



void receiveCanMsg()
{
  if(canActive)
    {
    unsigned long rxId = 0;
    byte len = 0;
    byte rxBuf[8];
    char msgString[128]; // Serial Output String Buffer

    while (CAN_MSGAVAIL == CAN0.checkReceive()) // check if data coming
    {
      CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
      if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
        sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
      else
        sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

      if ((rxId & 0x40000000) == 0x40000000)
      { // Determine if message is a remote request frame.
        sprintf(msgString, " REMOTE REQUEST FRAME");    }
      else
      {
        //for (byte i = 0; i < len; i++)
        //{
        //  sprintf(msgString, " 0x%.2X", rxBuf[i]);
        //}
        //printCanMsg("RECV", rxBuf);
        canMember member;
        member.id = 0xffff;
        member.name = "DummyM";
        ElsterIndex index;
        index.Index = 0xaaaa;
        index.Name = "DummyIndex";
        index.Type = et_err_nr;
        msgType type = t_systemRespond;
        ESP_LOGI("RECV", "Sender ID: 0x%.3lX", rxId);
        decomposeMsg(rxBuf, member.name, &member.id, &index, &type);
      }
    }
    return;
  }
  return;
}




void loop()
{
  receiveCanMsg();
  if (millis() > last_send + SEND_FREQ)
  {
    //scan();
    /*
    canMember member;
    member.id = 0xffff;
    member.name = "DummyMember";
    ElsterIndex index;
    index.Index = 0xaaaa;
    index.Name = "DummyIndex";
    index.Type = et_err_nr;
    msgType type = t_systemRespond;
    decomposeMsg(sendData, member.name, &member.id, &index, &type);
    */
    last_send = millis();
  }
}