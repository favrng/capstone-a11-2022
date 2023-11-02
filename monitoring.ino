#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <WiFiManager.h>
#include <strings_en.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>


#define LED 2
Adafruit_ADS1115 ads; /* Use this for the 16-bit version */


//Identitas MQTT
const char* mqttServer = "e5c95919220940e6b39b0652f51e501e.s2.eu.hivemq.cloud";
const int mqttPort = 8883;
const char* mqttUser = "capstonea112022";
const char* mqttPassword = "capstonetanpaseminar";


// Defining variables
int RawValue = 0;
const float factor = 30;
float multiplier = 0.125F;
float voltage_intercept = -0.076875472; // to be adjusted based on calibration testin
float voltage_slope = 2796.730807;
float voltage_offset = 2.51;
float current_slope = 4.2919;
float current_intercept = 0.0641;


//
int randomNumber;
int timeout = 120;
unsigned long previousMillis = 0;
unsigned long interval = 5000;
int16_t adc2; //A2 ADS1115


WiFiClientSecure espClient;
PubSubClient client(espClient);
static const char* fingerprint PROGMEM = "44 14 9A 3F C3 E9 F1 F3 84 1A B4 9F B6 4D 19 8A B2 92 31 D6";


void setup() {
  Serial.begin(115200); // start the serial port
  Serial.println("Serial started");

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.begin(115200);
  setup_wifi();
  
  espClient.setInsecure(); // <-- Change #3: Set the SHA1 fingerprint
    // Alternative: espClient.setInsecure;
    
  client.setServer(mqttServer, mqttPort); // <-- Change #4: Set the port number to 8883
  client.setCallback(callback);
  client.publish("esp/test", "Hello from ESP32");
  client.subscribe("esp/test");

  ads.setGain(GAIN_ONE);
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
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
  if (!client.loop()) {
    client.connect("ESP8266Client", mqttUser, mqttPassword);
    digitalWrite(LED, HIGH); //indikator MQTT+Wifi terhubung
  }

  float volt_rms = getVoltage();
  float volt = (volt_rms*voltage_slope) + voltage_intercept;
  if (volt < 0){
  volt = 0;
  }

  float current_rms = getCurrent();
  float current = (current_rms*current_slope) + current_intercept;
  char voltString[8];
  char currentString[8];
  dtostrf(volt, 1, 2, voltString);
  dtostrf(current, 1, 2, currentString);
  delay(500);
  client.publish("publishvoltage", voltString);
  client.publish("publishcurrent", currentString);
}


float getVoltage()
{
  long Vtempo = millis();
  long VrawAdc = ads.readADC_SingleEnded(2);
  float voltage = ads.computeVolts(VrawAdc)-voltage_offset;
  float VminRaw = voltage;
  float VmaxRaw = voltage;
  while (millis() - Vtempo < 1000)
  {
    VrawAdc = ads.readADC_SingleEnded(2);
    voltage = ((VrawAdc*multiplier)/1000)-voltage_offset;
    VmaxRaw = VmaxRaw > voltage ? VmaxRaw : voltage;
    VminRaw = VminRaw > voltage ? VminRaw : voltage;
  }
  VmaxRaw = VmaxRaw > VminRaw ? VmaxRaw : VminRaw;
  float Voltage_RMS = VmaxRaw * 0.70710678118;
  if (Voltage_RMS < 0.01){
    Voltage_RMS = 0;
  }
return(Voltage_RMS);
}


float getCurrent()
{
  long tempo = millis();
  long rawAdc = ads.readADC_Differential_0_1();
  long minRaw = rawAdc;
  long maxRaw = rawAdc;
  while (millis() - tempo < 1000)
  {
    rawAdc = ads.readADC_Differential_0_1();
    maxRaw = maxRaw > rawAdc ? maxRaw : rawAdc;
    minRaw = minRaw > rawAdc ? minRaw : rawAdc;
  }
  maxRaw = maxRaw > minRaw ? maxRaw : minRaw;
  float voltagePeak = maxRaw * multiplier / 1000;
  float VoltageRMS = voltagePeak * 0.70710678118;
  float currentRMS = VoltageRMS * factor;
  return(currentRMS);
}


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
  if (topic == "connection/auth2") {
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
      client.subscribe("connection/auth2");
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
