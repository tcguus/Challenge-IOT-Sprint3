#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ultrasonic.h>
#include <DHT.h>

#define WIFI_SSID       "Wokwi-GUEST"
#define WIFI_PASS       ""

#define MQTT_HOST       "broker.hivemq.com"
#define MQTT_PORT       1883

#define DEVICE_ID       "vagaA"
#define SITE_BASE       "mottu/v1/parking/" DEVICE_ID

#define TRIG_PIN     5
#define ECHO_PIN     18
#define DHT_PIN      4
#define DHT_TYPE     DHT22
#define LED_OCC_PIN  2    
#define LED_MNT_PIN  14   
#define BUZZER_PIN   15
#define BTN_PIN      13

const float HYST_LOW_CM       = 18.0;   
const float HYST_HIGH_CM      = 22.0;   
const uint8_t SMA_WINDOW      = 5;      
const uint32_t PUBLISH_MIN_MS = 2000;   
const uint32_t TELEMETRY_MS   = 5000;   
const uint16_t BTN_DEBOUNCE_MS= 35;     

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);
DHT dht(DHT_PIN, DHT_TYPE);

float smaBuf[SMA_WINDOW] = {0};
uint8_t smaIdx = 0, smaCount = 0;

bool isOccupied = false;
bool maintenance = false;
uint32_t lastPublish = 0;
uint32_t lastTelemetry = 0;

bool btnLastStable = HIGH;     
bool btnLastRead   = HIGH;
uint32_t btnLastChangeMs = 0;

String topicStatus    = String(SITE_BASE) + "/status";
String topicState     = String(SITE_BASE) + "/state";
String topicTelemetry = String(SITE_BASE) + "/telemetry";
String topicCmd       = String(SITE_BASE) + "/cmd";

float smaAdd(float v){
  smaBuf[smaIdx] = v;
  smaIdx = (smaIdx + 1) % SMA_WINDOW;
  if (smaCount < SMA_WINDOW) smaCount++;
  float sum=0;
  for (uint8_t i=0;i<smaCount;i++) sum += smaBuf[i];
  return sum / smaCount;
}

long nowSec(){ return millis()/1000; }

void setOccLed(bool on){ digitalWrite(LED_OCC_PIN, on ? HIGH : LOW); }
void setMntLed(bool on){ digitalWrite(LED_MNT_PIN, on ? HIGH : LOW); }
void setBuzzer(bool on){ digitalWrite(BUZZER_PIN, on ? HIGH : LOW); }

void beep(uint8_t times=1, uint16_t onMs=120, uint16_t offMs=90){
  for(uint8_t i=0;i<times;i++){
    setBuzzer(true);  delay(onMs);
    setBuzzer(false); delay(offMs);
  }
}

void printBanner(){
  Serial.println();
  Serial.println("[BOOT] Challenge-IOT");
  Serial.print  ("[INFO] device="); Serial.println(DEVICE_ID);
}

void printNet(){
  Serial.print("[NET ] WiFi OK | ip=");
  Serial.println(WiFi.localIP());
}

void printMqtt(){
  Serial.print("[MQTT] Connected | broker=");
  Serial.print(MQTT_HOST); Serial.print(":"); Serial.println(MQTT_PORT);
  Serial.print("[TOPC] base="); Serial.println(SITE_BASE);
}

void onMqtt(char* topic, byte* payload, unsigned int len){
  StaticJsonDocument<128> doc;
  DeserializationError e = deserializeJson(doc, payload, len);
  if (e) return;
  if (doc.containsKey("led")) {
    bool on = String(doc["led"])=="on";
    setOccLed(on); 
    Serial.printf("[CMD ] led(occ)=%s\n", on?"ON":"OFF");
  }
  if (doc.containsKey("buzzer")) {
    bool on = String(doc["buzzer"])=="on";
    setBuzzer(on);
    Serial.printf("[CMD ] buzzer=%s\n", on?"ON":"OFF");
  }
}

void connectWiFi(){
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[NET ] Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED){ delay(300); Serial.print("."); }
  Serial.println();
  printNet();
}

void connectMQTT(){
  if (mqtt.connected()) return;
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);

  
  String clientId = "esp32-" + String(DEVICE_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("[MQTT] Connecting");
  while (!mqtt.connected()){
    if (mqtt.connect(clientId.c_str(), nullptr, nullptr,
                     topicStatus.c_str(), 1, true, "offline")){
      mqtt.publish(topicStatus.c_str(), "online", true);
      mqtt.subscribe(topicCmd.c_str(), 1);
      Serial.println();
      printMqtt();
    } else {
      Serial.print(".");
      delay(800);
    }
  }
}

void publishState(){
  StaticJsonDocument<160> doc;
  doc["deviceId"]    = DEVICE_ID;
  doc["status"]      = isOccupied ? "ocupada" : "livre";
  doc["maintenance"] = maintenance;
  doc["ts"]          = (long)nowSec();

  char buf[180];
  size_t n = serializeJson(doc, buf);
  mqtt.publish(topicState.c_str(), (const uint8_t*)buf, n, true); 
  lastPublish = millis();

  Serial.print("[STATE] "); Serial.write((const uint8_t*)buf, n); Serial.println();
}

void publishTelemetry(float dist, float t, float h){
  StaticJsonDocument<300> doc;
  doc["deviceId"]    = DEVICE_ID;
  doc["distance_cm"] = dist;
  if (!isnan(t)) doc["temp_c"] = t;
  if (!isnan(h)) doc["hum_pct"] = h;
  doc["rssi"]        = WiFi.RSSI();
  doc["maintenance"] = maintenance;
  doc["ts"]          = (long)nowSec();

  char buf[320];
  size_t n = serializeJson(doc, buf);
  mqtt.publish(topicTelemetry.c_str(), (const uint8_t*)buf, n, false);
  lastTelemetry = millis();

  Serial.print("[TEL ] "); Serial.write((const uint8_t*)buf, n); Serial.println();
}

void setup(){
  pinMode(LED_OCC_PIN, OUTPUT);
  pinMode(LED_MNT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  setOccLed(false);
  setMntLed(false);
  setBuzzer(false);

  Serial.begin(115200);
  while(!Serial){;}
  printBanner();

  dht.begin();
  mqtt.setBufferSize(512); 

  connectWiFi();
  connectMQTT();
}

void loop(){
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  bool btnRaw = digitalRead(BTN_PIN); 
  if (btnRaw != btnLastRead){
    btnLastRead = btnRaw;
    btnLastChangeMs = millis();
  }
  if ((millis() - btnLastChangeMs) > BTN_DEBOUNCE_MS){
    if (btnLastStable != btnRaw){
      btnLastStable = btnRaw;
      if (btnRaw == LOW){ 
        maintenance = !maintenance;        
        setMntLed(maintenance);            
        if (maintenance){ beep(1); Serial.println("[EVNT] maintenance=ON  (LED2 ON)"); }
        else            { beep(2); Serial.println("[EVNT] maintenance=OFF (LED2 OFF)"); }
        publishState();
      }
    }
  }

  
  float cm = ultrasonic.read(); 
  if (cm > 0 && cm <= 400) cm = smaAdd(cm); 
  else cm = NAN;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  bool wasOccupied = isOccupied;
  if (!isnan(cm)){
    if (!isOccupied && cm <= HYST_LOW_CM)  isOccupied = true;
    if (isOccupied  && cm >= HYST_HIGH_CM) isOccupied = false;
  }

  setOccLed(isOccupied);

  if (wasOccupied != isOccupied && (millis() - lastPublish > PUBLISH_MIN_MS)){
    publishState();
  }

  if (millis() - lastTelemetry > TELEMETRY_MS){
    publishTelemetry(cm, t, h);
  }

  delay(30);
}
