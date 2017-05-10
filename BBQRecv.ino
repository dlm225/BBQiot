#include <AWSFoundationalTypes.h>
#include <DeviceIndependentInterfaces.h>
#include <Esp8266AWSImplementations.h>
#include <jsmn.h>
#include <sha256.h>
#include <Utils.h>
#include <keys.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN            14                         // NeoPixel Arduino Pin (GPIO)
#define NUMPIXELS      3                          // Num NeoPixels attached
const char* ssid = "IOT1234";                     // Wifi SSID
const char* password = "iottest123";              // Wifi password
const char* mqtt_server = "192.168.11.74";        // MQTT broker address
String message = "";
float meatTemp = 0;
float grillTemp = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);
 
void callback(char* topic, byte* payload, unsigned int length) {
  for (int i=0;i<length;i++) {  
    char receivedChar = (char) payload[i];
    Serial.print(receivedChar);
    message = String(message + receivedChar);
  }
  Serial.println(message[0]);
  Serial.println();

  if (message[0] == '0') {                                  // Select grill thermometer and
    String msgTemp = message.substring(2);                  // set grillTemp variable to stripped
    grillTemp = msgTemp.toFloat();                          // message string
  }
  else if (message[0] == '1') {                             // Select meat thermometer and
    String msgTemp = message.substring(2);                  // set meatTemp variable to stripped
    meatTemp = msgTemp.toFloat();                           // message string
  }

  // NeoPixel color selections for grillTemp
  if ((grillTemp < 225.00) || (grillTemp > 300.00)) {       // LED 3 turns red blinking at 300degF
    for (int j=0;j<10;j++) {                                // USDA highest temp for smoking meats 
      pixels.setPixelColor(2, pixels.Color(0,0,0));         // OR
      pixels.show();                                        // turns red blinking at 225degF or lower
      delay(1000);                                          // USDA lowest temp for smoking meats
      pixels.setPixelColor(2, pixels.Color(255,0,0));         
      pixels.show();
      delay(1000);
    }
  }
  else if ((grillTemp >= 225.00) && (grillTemp <= 300.00)) {// LED 3 turns green btwn 225-300 degF
      pixels.setPixelColor(2, pixels.Color(0,255,0));       // USDA rec. grill temp for smoking meats
  }
  else {                                                    // Failover LED 3 to OFF
    pixels.setPixelColor(2, pixels.Color(0,0,0));
  } 
  
  // NeoPixel color selections for meatTemp
  if (meatTemp >= 165.00) {                                 // LEDs 1&2 turn blue blinking at 165degF
    for (int j=0;j<10;j++) {                                // USDA rec. temp for smoking poultry 
      pixels.setPixelColor(0, pixels.Color(0,0,0));         
      pixels.setPixelColor(1, pixels.Color(0,0,0));
      pixels.show();
      delay(1000);
      pixels.setPixelColor(0, pixels.Color(0,0,255));         
      pixels.setPixelColor(1, pixels.Color(0,0,255));
      pixels.show();
      delay(1000);
    }
  }
  else if ((meatTemp >= 160.00) && (meatTemp < 165.00)) {   // LEDs 1&2 turn blue/green at 160degF
      pixels.setPixelColor(0, pixels.Color(0,255,200));     // USDA rec. temp for smoking ground meats
      pixels.setPixelColor(1, pixels.Color(0,255,200));     // other than poultry   
  }
  else if ((meatTemp >= 145.00) && (meatTemp < 160.00)) {   // LEDs 1&2 turn green blinking at 145degF
    for (int j=0;j<10;j++) {                                // USDA rec. temp for smoking whole beef,
      pixels.setPixelColor(0, pixels.Color(0,0,0));         // venison, lamb, etc
      pixels.setPixelColor(1, pixels.Color(0,0,0));         
      pixels.show();
      delay(1000);
      pixels.setPixelColor(0, pixels.Color(0,255,0));         
      pixels.setPixelColor(1, pixels.Color(0,255,0));
      pixels.show();
      delay(1000);
    }            
  }
  else if ((meatTemp >= 140.00) && (meatTemp < 145.00)) {   // LEDs 1&2 turn yellow(ish) at 140degF
    pixels.setPixelColor(0, pixels.Color(255,255,0));
    pixels.setPixelColor(1, pixels.Color(255,255,0));
  }
  else if ((meatTemp >= 120.00) && (meatTemp < 140.00)) {   // LEDs 1&2 turn orange(ish) at 120degF
    pixels.setPixelColor(0, pixels.Color(255,120,0));
    pixels.setPixelColor(1, pixels.Color(255,120,0));
  }
  else if ((meatTemp >= 0.00) && (meatTemp < 120.00)) {     // LEDs 1&2 are red from 0-120degF
    pixels.setPixelColor(0, pixels.Color(255,0,0));
    pixels.setPixelColor(1, pixels.Color(255,0,0));
  }
  else {                                                    // Failover LEDs 1&2 to OFF
    pixels.setPixelColor(0, pixels.Color(0,0,0));
    pixels.setPixelColor(1, pixels.Color(0,0,0));
  }
  pixels.show();                                            // Send updated pixel color
  message = "";                                             // Reset message variable
}
 
void reconnect() {
  while (!client.connected()) {                             // Loop until we're reconnected
    Serial.print("Attempting MQTT connection...");
    if (client.connect("BBQRecvr Client")) {                // Attempt to connect
      Serial.println("connected");
      client.subscribe("localgateway_to_awsiot");           // Subscribe to topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);                                          // Wait 5 seconds before retrying 
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
  pixels.begin();                                     // Initializes NeoPixel library
  pinMode(PIN, OUTPUT);
  pixels.setPixelColor(0, pixels.Color(0,0,0));
  pixels.setPixelColor(1, pixels.Color(0,0,0));
  pixels.show();                                      // Send updated pixel color
}

void loop() {
  if (!client.connected()) {
      reconnect();
  }
  client.loop();
}
