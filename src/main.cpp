#include <Arduino.h>

/*
 Basic MQTT example with Authentication

  - connects to an MQTT server, providing username
    and password
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <sstream>
#include <String>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


using namespace std;

#define PFLANZE "Basilikum"

const char* ssid = "WLAN-CYJNZA";
const char* password = "VJ2507199529091996";
const char* mqtt_server = "elpollo.de";
const uint8_t pumpPin = D1;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

boolean pumpON = false;
unsigned long t1, t2;
int timeInput;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
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

void callback(char* topic, byte* _payload, unsigned int length) {
  char msg[length];
  if(pumpON == false){
    pumpON = true;
    //byte* to char[]
    for (int i = 0; i < length; i++) {
      msg[i] = (char)_payload[i];
    }

    sscanf(msg, "%d", &timeInput);

    if (timeInput < 0 || timeInput > 20000) {
      pumpON = false;
      return;
    }

    // starte timer
    t1 = millis();
    // setzte output high
    digitalWrite(pumpPin, HIGH);
    String msg_finished = String(PFLANZE) + ": started with irigation: " + timeInput;
    client.publish("logs", msg_finished.c_str());

    String topic = "/" + String(PFLANZE) + "/pump/status";
    client.publish(topic.c_str(), "1");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-Basilikum";
    clientId += ESP.getChipId();
    // Attempt to connect
    if(client.connect(clientId.c_str(), "julian", "Palme132.")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String msg = "Gießkanne 1 - " + String(PFLANZE) +  " - connected";
      client.publish("logs", msg.c_str());
      // ... and resubscribe
      client.subscribe(("/"+String(PFLANZE)).c_str());
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
  pinMode(pumpPin, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  ArduinoOTA.onStart([]() {
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  } else { // U_FS
    type = "filesystem";
    }
  // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
}

void loop() {
  //check if connected
  if (!client.connected()) {
    reconnect();
    Serial.print("connected");
  }

  //when connected handle ota
  ArduinoOTA.handle();

  // now handle your mqtt jobs
  client.loop();

  //after all that, check for internal logic
  if(pumpON == true){
    t2 = millis();
    unsigned long diff = t2-t1;
    if(diff > timeInput){
      digitalWrite(pumpPin, LOW);
      String msg_finished = "finished with irigation: "+ String(PFLANZE);
      client.publish("logs", msg_finished.c_str());
      pumpON = false;
      String topic = "/" + String(PFLANZE) + "/pump/status";
      client.publish(topic.c_str(), "0");
    }
  }
}
