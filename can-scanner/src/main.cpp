#include <mcp_can.h>
#include <SPI.h>
#include <Arduino.h>
#include <stdio.h>
#include "inc/KElsterTable.h"
#include "inc/ElsterTable.inc"
#include "esp_log.h"
#include "CanMembers.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>



#define SENDER_ID 0x700
#define SEND_FREQ 2000 // The wait time between sending two messages

const char* ssid = "Airport";
const char* password = "Schwarzwald1";
const char* mqtt_server = "192.168.0.201";
const char* mqtt_endpoint = "home/heating/canmessages";
const char* mqtt_subscription = "home/heating/control";
int mqtt_port = 1883;


WiFiClient* wifi_client;
PubSubClient* mqtt_client;

MCP_CAN CAN0(5); // Set CS pin

bool canActive = true; // Indicates whether CAN has been set up correctly
bool mcp2515Connected = true; // If set to false, CAN setup and communications is omitted. (Useful for local development & testing)

byte sendData[] = {0xC1, 0x01, 0x69, 0x00, 0x00, 0x00, 0x00};

const int canMembersToScan[] = {0x180, 0x500, 0x480};
const int canMembersToScanCount = 3;

const int elsterTableSize = 3610;

unsigned long last_send = 0;



/**
  Sets up the WiFi Connection.
*/
void setupWIFI() {

  ESP_LOGI("WIFI", "Connecting to WiFi '%s'", ssid);

	WiFi.begin(ssid, password);

	WiFi.status();
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		ESP_LOGI("WIFI", "%d", WiFi.status());
	}
	ESP_LOGI("WIFI", "Connected to WiFi '%s' with IP %s", ssid, WiFi.localIP().toString().c_str());
}



/**
  Needs to be executed regularly (e.g. in every loop) and checks whether WiFi is still connected. If not, it reconnects.
*/
void loopWIFI()
{
	if (WiFi.status() != WL_CONNECTED) {
		ESP_LOGI("WIFI", "Connection seems to be missing, Starting WiFi Setup");
		setupWIFI();
	}
}



/**
  Callback Method to handle incoming MQTT Messages

  @param topic the MQTT topic under which the message was received
  @param payload the payload of the received MQTT message
  @param length the length of the payload
*/
void processMessage(char* topic, byte* payload, unsigned int length) {
	ESP_LOGI("MQTT", 
  	"Message arrived: Topic: %s, Length: %d, Payload: [%s]",
		topic, length, payload
		);

	//controlValves((char*) payload);
}



/**
  (Re)connects to the MQTT-Broker. 
*/
void reconnectMQTT() {
  // Loop until we're reconnected
  while (!mqtt_client->connected()) {
    ESP_LOGI("MQTT", "Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt_client->connect(clientId.c_str())) {
      ESP_LOGI("MQTT", "connected");
      // Once connected, publish an announcement...
      mqtt_client->publish(mqtt_endpoint, "hello world");
      // ... and resubscribe
      mqtt_client->subscribe(mqtt_subscription);
    } else {
      ESP_LOGI("MQTT", "failed, rc=");
      Serial.print(mqtt_client->state());
      ESP_LOGI("MQTT", " try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
	return;
}



/**
  Handles the initial setup of the MQTT connection
*/
void setupMQTT() {

	wifi_client = new WiFiClient();
	mqtt_client = new PubSubClient(*wifi_client);
	mqtt_client->setServer(mqtt_server, mqtt_port);
	mqtt_client->setCallback(processMessage);
	reconnectMQTT();

}



/**
  Needs to be executed regularly (e.g. in every loop) and checks whether MQTT is still connected. If not, it reconnects.
*/
void loopMQTT() {
	// Check connection status
	if (!mqtt_client->connected()) {
		ESP_LOGI("MQTT", "No Connection. Trying to establish new connection.");
		reconnectMQTT();
	}
	mqtt_client->loop();
}



/**
  The Main setup method of the Arduino Sketch
*/
void setup()
{

  //strcpy(canMembers[0].name, "Kessel");
  //canMembers[0].id = 0x180;

  //strcpy(canMembers[1].name, "Heizmodul");
  //canMembers[1].id = 0x500;

  //strcpy(canMembers[2].name, "Manager");
  //canMembers[2].id = 0x480;


  Serial.begin(115200);
  
  setupWIFI();
  setupMQTT();
  
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



/**
  Translates a given CAN-ID to the corresponding two bytes as specified here:
  http://juerg5524.ch/data/Telegramm-Aufbau.txt

  @param id the CAN-ID to convert.
  @param byte0 a pointer to the first byte which shall contain the converted CAN-ID 
  @param byte1 a pointer to the second byte which shall contain the converted CAN-ID 
*/
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



/**
  Translates a given Index of an Elster Message to the corresponding two bytes as specified here:
  http://juerg5524.ch/data/Telegramm-Aufbau.txt
 

  @param index the CAN-ID to convert.
  @param byte0 a pointer to the first byte which shall contain the converted Index 
  @param byte1 a pointer to the second byte which shall contain the converted Index
  @param byte2 a pointer to the third byte which shall contain the converted Index

*/
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



/**
  Takes a given Byte-Array of 7 bytes and prints it in raw Hex Values.
  (Useful for debugging)

  @param tag the Tag to use for the logging function
  @param msg a pointer to a 7-byte array, containing the message

*/
void printCanMsg(const char* tag, byte *msg)
{
      ESP_LOGI(tag, "%02x %02x %02x %02x %02x %02x %02x", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6]);
}



/**
  Takes a pointer to a Byte-Array of 7 bytes and sends it on the CAN.

  @param msg a pointer to a 7-byte array, containing the message

*/
void sendCanMsg(byte *msg)
{
  // TODO: Check if the message is really 7 bytes big
  // TODO: Implement reasonable return codes
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



/**
  Returns the index of KnownCanMembers if the CAN ID is a known. 

  @param msg a pointer to a 7-byte array, containing the message
  @return -1 if the CAN ID is not known, the identified CAN-ID otherwise.

*/
int lookupCanID(unsigned short id)
{
  int retVal = -1;
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



/**
  Returns the index (position) within the ElsterTable if the given ElsterIndex is found.

  @param msg a pointer to a 7-byte array, containing the message
  @return 0 if the ElsterIndex is not found, the identified index otherwise

*/
int lookupElsterIndex(unsigned short index)
{
  // TODO: Not Found Return Code auf -1 ändern.
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



/**
  Scans for available CAN-Members and for which messages they react to by iterating over all given CAN members.
  It requests messages for all messages defined in the Elster Table.
  This can take a while, since <CAN-Members> x <Elster Messages> are sent in total.

  @param members[] an Array of canMember, containing the members, for which shall be scanned.
  @param membersCount the amount of members in the Array.

*/
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



/**
  Takes a CAN message and decomposes it into individual data fields.

  @param msg Pointer to an Array of at least 7 bytes, containing the CAN Message.
  @param membername Pointer to a Char-Array, where the resolved CAN Member Name of the sender will be written to.
  @param memberID Pointer to an unsigned short, where the resolved CAN Member ID will be written to.
  @param index Pointer to an ElsterIndex, which will be filled based on the found data in the message
  @param type Pointer to a msgType where the identified message type will be written to.
  @return true if the message was decomposed successfully, false otherwise.
  
*/
bool decomposeMsg(byte *msg, const char *memberName, unsigned short *memberId, ElsterIndex *index, msgType *type)
{
  bool retVal = false;

  // Determine canMember ID
  *memberId = ((msg[0] & 0xf0) * 8) + (msg[1] & 0x0f);

  // Determine canMember Name
  int knownCanMembersIndex = lookupCanID(*memberId);
  if(knownCanMembersIndex != -1)
  {
    memberName = KnownCanMembers[knownCanMembersIndex].name;
  } else {
    memberName = "MemberNotFound";
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

  // Determine ElsterIndex Name & Type
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


/**
  If there is a CAN message availabe, it is received, decomposed and sent to the specified MQTT broker as a JSON.  
*/
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

        //build JSON based on Message
        StaticJsonDocument<200> jsonMsg;
        jsonMsg["canSenderId"] = member.id;
        jsonMsg["canSenderName"] = member.name;
        jsonMsg["elsterIndex"] = index.Index;
        jsonMsg["elsterIndexName"] = index.Name;
        jsonMsg["elsterIndexType"] = index.Type;
        jsonMsg["messageType"] = type;

        // send received CAN message via MQTT to the Broker
        char serializedJsonMsg[200];
        serializeJson(jsonMsg, serializedJsonMsg, 200);
        mqtt_client->publish(mqtt_endpoint, serializedJsonMsg);

      }
    }
    return;
  }
  return;
}



/**
  The Main Loop of the Arduino Sketch
*/
void loop()
{
  loopWIFI();
  loopMQTT();


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