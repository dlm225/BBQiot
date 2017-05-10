

#include <AmazonDynamoDBClient.h>
#include <AmazonKinesisClient.h>
#include <AmazonSNSClient.h>
#include <AWSClient.h>
#include <AWSClient2.h>
#include <AWSFoundationalTypes.h>
#include <DeviceIndependentInterfaces.h>
#include <Esp8266AWSImplementations.h>
#include <jsmn.h>
#include <sha256.h>
#include <Utils.h>
#include <keys.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

const char* ssid = "IOT1234";                     // Wifi SSID
const char* password = "iottest123";              // Wifi password
const char* mqtt_server = "192.168.11.74";        // MQTT broker address

#define ONE_WIRE_BUS D4
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

DeviceAddress meatTemp, grillTemp;

WiFiClient espClient;
PubSubClient client(espClient);
 
void callback(char* topic, byte* payload, unsigned int length) {
    char receivedChar = (char) payload[0];
    Serial.print(receivedChar);
}
 
void reconnect() {
  while (!client.connected()) {                     // Loop until we're reconnected
    Serial.print("Attempting MQTT connection...");
    if (client.connect("BBQtx Client")) {           // Attempt to connect
      Serial.println("connected");
      client.subscribe("localgateway_to_awsiot");   // Subscribe to topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);                                  // Wait 5 seconds before retrying 
    }
  }
}
 
void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);                                // Sets ESP8266 to STATION mode
  WiFi.begin(ssid, password);                         // Launch wifi connect process

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  sensors.begin();
}

void loop() {
  if (!client.connected()) {
      reconnect();
  }
  
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  for (uint8_t s=0; s < sensors.getDeviceCount(); s++) {
    // get the unique address 
    sensors.getAddress(grillTemp, s);
    // just look at bottom two bytes, which is pretty likely to be unique
    int smalladdr = (grillTemp[6] << 8) | grillTemp[7];
    
    Serial.print("Temperature for the device #"); Serial.print(s); 
    Serial.print(" with ID #"); Serial.print(smalladdr);
    Serial.print(" is: ");
    float temp = sensors.getTempFByIndex(s);
    Serial.println(temp);
    
    String pubstring;
    static char pubchar[14];
    static char tempF[8];
    String sensorid;

    sensorid = String(s);
    dtostrf(temp, 6, 2, tempF);
    pubstring = sensorid + ' ' + tempF;
    pubstring.toCharArray(pubchar,14);
    client.publish("localgateway_to_awsiot", pubchar);
  }
  
  delay(15000);
  
  client.loop();
}
