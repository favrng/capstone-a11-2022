#include <WiFiManager.h>
#include <strings_en.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>


#define relay1 4
#define relay2 16
#define relay3 17
#define relay4 5
#define LED 2


//Identitas MQTT
const char* mqttServer = "e5c95919220940e6b39b0652f51e501e.s2.eu.hivemq.cloud";
const int mqttPort = 8883;
const char* mqttUser = "capstonea112022";
const char* mqttPassword = "capstonetanpaseminar";


//
int randomNumber;
int timeout = 120;
unsigned long previousMillis = 0;
unsigned long interval = 5000;


WiFiClientSecure espClient;
PubSubClient client(espClient);
static const char* fingerprint PROGMEM = "44 14 9A 3F C3 E9 F1 F3 84 1A B4 9F B6 4D 19 8A B2 92 31 D6";


// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  
  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();
  
  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

   bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  //res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}


void setup_wifi_ondemand(){
  WiFiManager wm;
  
  //reset settings - for testing
  //wm.resetSettings();
  
  // set configportal timeout
  wm.setConfigPortalTimeout(timeout);

  if (!wm.startConfigPortal("OnDemandAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  
  //if you get here you have connected to the WiFi
   Serial.println("connected...yeey :)");
}


void callback(String topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);
  Serial.println();
  Serial.println("-----------------------");
  if (topic == "controlling/relay1") {
    Serial.print("Kondisi Lampu1:");
    if (messageTemp == "on") {
      digitalWrite(relay1, HIGH);
      Serial.println("ON");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay1, LOW);
      Serial.println("OFF");
    }
  }
  if (topic == "controlling/relay2") {
    Serial.print("Kondisi Lampu2:");
    if (messageTemp == "on") {
      digitalWrite(relay2, HIGH);
      Serial.println("ON");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay2, LOW);
      Serial.println("OFF");
    }
  }
  if (topic == "controlling/relay3") {
    Serial.print("Kondisi Lampu3:");
    if (messageTemp == "on") {
      digitalWrite(relay3, HIGH);
      Serial.println("ON");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay3, LOW);
      Serial.println("OFF");
    }
   }
  if (topic == "controlling/relay4") {
    Serial.print("Kondisi Lampu4:");
    if (messageTemp == "on") {
      digitalWrite(relay4, HIGH);
      Serial.println("ON");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay4, LOW);
      Serial.println("OFF");
    }
  }
  if (topic == "connection/auth") {
     Serial.print("Merubah Wi-Fi..");
    if (messageTemp == "change_ok") {
      setup_wifi_ondemand();
    }
  }
}


// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
    YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
    To change the ESP device ID, you will have to give a new name to the ESP8266.
    Here's how it looks:
    if (client.connect("ESP8266Client")) {
    You can do it like this:
    if (client.connect("ESP1_Office")) {
    Then, for the other ESP:
    if (client.connect("ESP2_Garage")) {
    That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      digitalWrite(LED, HIGH); //indikator MQTT+Wifi terhubung
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("controlling/relay1");
      client.subscribe("controlling/relay2");
      client.subscribe("controlling/relay3");
      client.subscribe("controlling/relay4");
      client.subscribe("connection/auth");
      client.publish("status", "CONNECTED");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
  digitalWrite(LED, LOW);
  Serial.begin(115200);
  setup_wifi();
  
  espClient.setInsecure(); // <-- Change #3: Set the SHA1 fingerprint
  // Alternative: espClient.setInsecure;
  client.setServer(mqttServer, mqttPort); // <-- Change #4: Set the port number to 8883
  client.setCallback(callback);
  client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("esp/test");
  }

  
void loop() {
  //Ini adalah code untuk reconnect wi-fi jika lost connection secara tiba-tiba//
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.print(millis());
    Serial.println("DISCONNECTED! Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
  
///////////////////////////////////////////////////////////////////////////////
  if (!client.connected()) {
    digitalWrite(LED, LOW); //indikator MQTT+Wifi terputus
    reconnect();
  }
  else if (!client.loop()) {
  client.connect("ESP8266Client", mqttUser, mqttPassword);
  digitalWrite(LED, HIGH); //indikator MQTT+Wifi terhubung
  }
  int dummy = random(10);
  char dummyString[8];
  dtostrf(dummy, 1, 2, dummyString);
  delay(500);
  client.publish("publishtest", dummyString);
}
