/*
 MQTT server in a Arduino board with ESP8266 

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ATT_IOT.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <CborBuilder.h>
#include "sensirion_common.h"
#include "sgp30.h"

#define DHTPIN 13       // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

//Constructor
DHT dht(DHTPIN, DHTTYPE);

/*
Enter the values of the Wi-Fi connection and the raspberry broker
*/
// Update these with values suitable for your network.

const char* ssid = "WiFi_TFG"; //Wi-Fi name
const char* password = "71361323m"; //Wi-Fi password
const char* mqtt_server = "10.3.141.1"; //Broker

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

/*
This method iniciates the Wi-Fi connection
*/
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println()
  ;
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

/*
This method is to reconnect is we lose the connection
*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

 pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
     //Start the serial port for debugging
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  Serial.println("Init sensors!");

  //Power on the Grove Sensors on Wio Link Board
  #if defined(ESP8266)
          pinMode(15,OUTPUT);
          digitalWrite(15,1);
          Serial.println("Set wio link power!");
          delay(500);
  #endif
    //Start the DHT sensor
  dht.begin();
  /*Init module,Reset all baseline,The initialisation takes up to around 15 seconds, during which
  all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
  while (sgp_probe() != STATUS_OK) {
         Serial.println("SGP init failed");
         while(1);
         }

  Serial.println("ready to sample the sensors!");

   // Connect to the DS IoT Cloud MQTT Broker
  Serial.println("Subscribe MQTT");

  Serial.println("retrying");
  
}
/*
This method executed all the code
*/
void loop() {
	// If we are not connected we invoke to reconnected
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  s16 err=0;
  u16 tvoc_ppb, co2_eq_ppm;
  
  //We subscribe to the topics
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    // Temperatura
    Serial.println("sample DHT sensor");
    float temperature = dht.readTemperature(false);
    snprintf (msg, 50, "Temperature: #%lf", temperature);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("universidad/laboratorio/temperatura", msg);
    //Humedad
    float humidity = dht.readHumidity();
    snprintf (msg, 50, "Humedad: #%lf", humidity);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("universidad/laboratorio/humedad", msg);
    //Compuestos volatiles y CO2
      /* start a tVOC and CO2-eq measurement and to readout the values*/
     err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
   if (err != STATUS_OK) {
      Serial.println("error reading tvoc_ppb & co2_eq_ppm values\n");
      } 
      snprintf (msg, 200, "CO2: #%d", co2_eq_ppm);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("universidad/laboratorio/co2", msg);

      snprintf (msg, 200, "TVOC: #%d", tvoc_ppb);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("universidad/laboratorio/tvoc", msg);
  }
}
