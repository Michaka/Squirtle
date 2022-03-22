/*  Arduino Ethernet MQTT Client
    Subscribing, and publishing messeages to an MQTT broker for controlling irrigation system
    Componets used Arduino UNO + Ethernet shiled + CD4051BE + 74HC595 + i2c
*/

//INCLUDES
#include<SPI.h>
#include<Ethernet.h>
//Download from https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

//Constants
//MAC address of this  device (normally printed on a sticker on the shield).
//If not known , make your own unique 6-hexadicimal value
const byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
//Unique IP address to assign to this device ( if DHCP fails)
const IPAddress deviceIP (192, 168, 0, 31);
// IP address of the machine on the nethwork running the MQTT broker
const IPAddress mqttServerIP (192, 168, 0, 106);
// port of the machine on the nethwork running the MQTT broker
const int mqttServerPort = 1883;
//Unique name of this device, used as client ID to connect to MQTT server
// and also topic mane for messages published to this server
const char* deviceID = "GArduino";

//GLOBALS
//Create an instance of the Ehternet client
EthernetClient ethernetClient;
//Create an isntance of the MQTT client based on the ethernet client
PubSubClient MQTTclient(ethernetClient);
//The time (for millis()) at witch last message was published
long lastMsgTime = 0;
//A buffer to hold messages to be sent/ have been received
char msg[64];
//The topic in witch to publish a message
char topic[32];
//counter for number of heartbeat pulses sent
int pulseCount = 0;

//The miliseconds we want before sending new sensor data
int desiredDelay = 1000;

//Time we last send sensor data 
unsigned long lastTimeSensorDataWasSend = 0;

//Time we last updated the relays
unsigned long lastTimeRelayWasUpdated = 0;

//Pin connected to 12 of 74HC595
const int latchPin = 3;
//Pin connected to 11 of 74HC595
const int clockPin = 9;
//Pin connected to 14 of 74HC595
const int dataPin = 8;

//Pins connected to 11,10,9 of CD4051BE
const int selectPins[3] = {5,6,7};
const int inhibPin = 4;
//Pin connected to 3 of CD4051BE
const int analogInput = A0;

//holders for information you're going to pass to shifting function
byte data;

const char* potatoSensor = "garden/potato/moisture";
const char* tomatoSensor = "garden/tomato/moisture";
const char* watermelonSensor = "garden/watermelon/moisture";
const char* cucumberSensor = "garden/cucumber/moisture";
const char* pipperSensor = "garden/pepper/moisture";
const char* irrigationPumpSensor = "garden/pump/irrigation/flow";
const char* sinkPumpSensor = "garden/pump/sink/flow";
const char* sensorArray[7] = {
                               potatoSensor,
                               tomatoSensor,
                               watermelonSensor,
                               cucumberSensor,
                               pipperSensor,
                               irrigationPumpSensor,
                               sinkPumpSensor
                               };


const char* potatoTopic = "garden/potato/valve/state";
const char* tomatoTopic = "garden/tomato/valve/state";
const char* cucumberTopic = "garden/cucumber/valve/state";
const char* pipperTopic = "garden/pepper/valve/state";
const char* watermelonTopic = "garden/watermelon/valve/state";
const char* irrigationPumpTopic = "garden/irrigation/pump/state";
const char* sinkPumpTopic = "garden/sink/pump/state";
const char* topicArray[7] = {
                               potatoTopic,
                               tomatoTopic,
                               watermelonTopic,
                               cucumberTopic,
                               pipperTopic,
                               irrigationPumpTopic,
                               sinkPumpTopic
                               };

//Callback function each time a message is published in any of
// the topics to hich this client is subscribed
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  //The messaged "payload" passed to this function is byte*
  //Let`s first copy its contents to the msg char[] array
  memcpy(msg, payload, length);
  //Add a NULL terminator
  msg[length] = '\0';

  //Act upon message recieved
  if(strcmp(msg, "ON") == 0){
    powerOn(topic);
  }
  else if(strcmp(msg, "OFF") == 0) {
    powerOff(topic);
  }
}
void ethernetSetup() {
  if(!Serial) {
    //Start the serial connection
    Serial.begin(9600);
  }

  //Attempt to connect to the specified nethwork
  Serial.print("Connecting to nethwork");

  //First, try to create a connection using DHCP
  if(Ethernet.begin(mac) == 0) {
    //Try to connect using specific IP address instead of DHCP
    Ethernet.begin(mac, deviceIP);
  }

  //Give the Ethernet shield a couple of seconds to initialize:
  delay(2000);

  //Print debug info about the connection.
  Serial.print("Connected! IP adress: ");
  Serial.println(Ethernet.localIP());  
}

void mqttSetup(){
  // Define some settings for the MQTT client
  MQTTclient.setServer(mqttServerIP, mqttServerPort);
  MQTTclient.setCallback(mqttCallback);
}

void sensorSetup(){
   // Set up the select pins as outputs:
   for (int i=0; i < 3; i++)
   {
     pinMode(selectPins[i], OUTPUT);
     digitalWrite(selectPins[i], HIGH);
   }
   pinMode(inhibPin, OUTPUT);
   digitalWrite(inhibPin, LOW);
}

void relaySetup(){
   // Set up the select pins as outputs:
   pinMode(latchPin, OUTPUT);
   pinMode(dataPin, OUTPUT);
   pinMode(clockPin, OUTPUT);
}


void mqttLoop(){
  //Ensure there is a connection to the MQTT server
  while (!MQTTclient.connected()){
    //Debug info
    //convert this message for LCD
    Serial.print("connected to MQTT broker at ");
    Serial.println(mqttServerIP);
    
    //Check if connected to broker
    if(MQTTclient.connect(deviceID)){
      //Debug info
      Serial.println("Connected to MQTT beoker");

      //Subsribe to topics meant for all devices
      for(int i=0; i <= 6; i++){
        MQTTclient.subscribe(topicArray[i]);
      }
    }
    else{
      //Debug info why connection could not be made

      //convert this message for LCD
      Serial.print("Connection to MQTT server failed, rc= ");
      Serial.print(MQTTclient.state());
      Serial.println(" trying andin in 5 seconds");

      delay(5000);
    }
  }

  //Call the mian loop to check for and publish message
  MQTTclient.loop();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  SPI.begin();
  delay(100);

  ethernetSetup();
  mqttSetup();
  sensorSetup();
  relaySetup();
  
  
}

void loop() {

  sendSensorDataToMqtt();
  mqttLoop();
}

/**
 * Collect and send sensor data to Mqqt broker 
 */

void sendSensorDataToMqtt(){
  int j=0;
  if(millis() - lastTimeSensorDataWasSend > desiredDelay){
     lastTimeSensorDataWasSend = millis();
     for (byte pin=0; pin <= 6; pin++){
        for (int i=0; i < 3; i++) {
          digitalWrite(selectPins[i], pin & (1 << i)?HIGH:LOW);
        }
        int inputValue = analogRead(analogInput);
        pushSensorData(sensorArray[j], inputValue);
        j++;
        Serial.print(String(inputValue) + "\t");
     }
     Serial.println();
  }
}

/**
 * Send sensor data to mosquitto
 */
void pushSensorData(char* sensor, long sensorPin){
  snprintf(topic, 32, "%s", sensor);
  long sensorData = ((sensorPin)*100)/1018;
  Serial.print(String(sensorData) + "%"  + "\t");
  snprintf(msg, 64, "%d", sensorData);
  MQTTclient.publish(topic, msg);
}

/**
 * Called when the payload is ON
 */
void powerOn(char* topic){
  Serial.println("switch was turned on!");

  for(int i; i<=6; i++){
    if(strcmp(topic, topicArray[i]) == 0){
      bitSet(data, i);
      Serial.println(i);
    }
  }
  pushRelayData();

}

/**
 * Called when the payload is OFF
 */
void powerOff(char* topic){
  Serial.println("switch was turned off!");

  for(int i; i<=6; i++){
    if(strcmp(topic, topicArray[i]) == 0){
      bitClear(data, i);
      Serial.println(i);
    }
  }
  
  pushRelayData();
}

/**
 * Send modified relay data bytes
 */
void pushRelayData(){
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, LSBFIRST,data);
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  delay(1000);
}
