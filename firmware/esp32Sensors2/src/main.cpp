#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- ΟΡΙΣΜΟΣ PINS ΑΙΣΘΗΤΗΡΩΝ ---
#define WATER_SENSOR_PIN 32 // Αναλογικό pin για νερό
#define POWER_PIN 4         // Power pin για νερό
#define FLAME_D0_PIN 27     // Ψηφιακό pin για φωτιά (D0)
#define FLAME_A0_PIN 33     // Αναλογικό pin για φωτιά (A0)
#define SHOCK_PIN 26        // Ψηφιακό pin για κραδασμό (D0)
#define TRIG_PIN 5          // Pin αποστολής ήχου για ραντάρ (Ultrasonic)
#define ECHO_PIN 18         // Pin λήψης ήχου για ραντάρ (Ultrasonic)

// --- ΡΥΘΜΙΣΕΙΣ WIFI ---
const char* ssid = "INALAN_2.4G_tdiEiP"; 
const char* password = "X2DKiYdC";

// --- ΡΥΘΜΙΣΕΙΣ HIVEMQ CLOUD ---
const char* mqtt_server = "f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud"; 
const int mqtt_port = 8883; 
const char* mqtt_user = "ntomata"; 
const char* mqtt_password = "Ntomata1"; 

// Ορισμός Clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- ΜΕΤΑΒΛΗΤΕΣ ΓΙΑ ΧΡΟΝΟΜΕΤΡΑ ---
unsigned long lastMsg = 0;           // Για την αποστολή MQTT (5 δευτερόλεπτα)
const long interval = 5000; 
unsigned long lastRadarCheck = 0;    // Για το ραντάρ
const long radarInterval = 100;      // Διαβάζει το ραντάρ κάθε 100ms

// --- ΜΕΤΑΒΛΗΤΕΣ ΓΙΑ ΡΑΝΤΑΡ (ULTRASONIC) ---
const int baselineDistance = 70; // Η "κανονική" απόσταση
int tolerance = 15;              // Ανοχή (θα χτυπάει αν διαβάσει < 55 cm)

// Μεταβλητές για να "θυμάται" τα στιγμιαία γεγονότα
bool shockDetected = false; 
bool radarDetected = false; 

// Συνάρτηση που μετράει και επιστρέφει την απόσταση
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;
  
  return distance;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected to HiveMQ!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); 
  
  // Ρύθμιση για τον αισθητήρα νερού
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW); 

  // Ρύθμιση για τον αισθητήρα φωτιάς & κραδασμού
  pinMode(FLAME_D0_PIN, INPUT);
  pinMode(SHOCK_PIN, INPUT);

  // Ρύθμιση για το ραντάρ
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  setup_wifi();

  espClient.setInsecure(); // Παρακάμπτει τον έλεγχο πιστοποιητικού 
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Κρατάει ζωντανή τη σύνδεση με το MQTT

  unsigned long now = millis();

  // --- 1. ΣΥΝΕΧΗΣ ΕΛΕΓΧΟΣ ΚΡΑΔΑΣΜΟΥ ---
  // Διαβάζει ασταμάτητα. Αν γίνει σεισμός/χτύπημα, το καταγράφει!
  if(digitalRead(SHOCK_PIN) == LOW) {
    shockDetected = true;
  }

  // --- 2. ΕΛΕΓΧΟΣ ΡΑΝΤΑΡ ΚΑΘΕ 100ms ---
  if (now - lastRadarCheck >= radarInterval) {
    lastRadarCheck = now;
    int currentDistance = getDistance();
    
    // Αν η απόσταση είναι μικρότερη από 55 cm (70 - 15)
    if (currentDistance > 0 && currentDistance < (baselineDistance - tolerance)) {
      radarDetected = true;
    }
  }

  // --- 3. ΑΠΟΣΤΟΛΗ ΔΕΔΟΜΕΝΩΝ ΚΑΘΕ 5 ΔΕΥΤΕΡΟΛΕΠΤΑ ---
  if (now - lastMsg >= interval) {
    lastMsg = now;

    // Διάβασμα αισθητήρα νερού
    digitalWrite(POWER_PIN, HIGH); 
    delay(10); 
    int waterValue = analogRead(WATER_SENSOR_PIN); 
    digitalWrite(POWER_PIN, LOW); 
    float waterLevelPercentage = (waterValue / 4095.0) * 100;

    // Διάβασμα αισθητήρα φωτιάς
    int flameDigital = digitalRead(FLAME_D0_PIN);
    int flameAnalog = analogRead(FLAME_A0_PIN);
    bool fireAlarm = (flameDigital == HIGH);

    // Εκτύπωση των μετρήσεων στο Serial Monitor
    Serial.print("Water: "); Serial.print(waterLevelPercentage); Serial.print("% | ");
    Serial.print("Fire: "); Serial.print(fireAlarm ? "YES" : "NO"); Serial.print(" | ");
    Serial.print("Shock: "); Serial.print(shockDetected ? "YES" : "NO"); Serial.print(" | ");
    Serial.print("Radar: "); Serial.println(radarDetected ? "YES" : "NO");

    // Δημιουργία και Αποστολή JSON
    JsonDocument doc; 
    
    doc["device"] = "ESP32_Node_1";
    doc["water_raw"] = waterValue;
    doc["water_percent"] = waterLevelPercentage;
    doc["fire_raw"] = flameAnalog;
    doc["fire_alarm"] = fireAlarm;
    doc["shock_alarm"] = shockDetected;
    doc["radar_alarm"] = radarDetected;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);
    
    // Αποστολή στο MQTT
    client.publish("esp32/sensors/data", jsonBuffer);

    // --- 4. ΕΠΑΝΑΦΟΡΑ ΣΤΙΓΜΙΑΙΩΝ ΣΥΝΑΓΕΡΜΩΝ ---
    // Αφού στείλαμε το πακέτο, "ξεχνάμε" τα γεγονότα μέχρι την επόμενη φορά
    shockDetected = false; 
    radarDetected = false; 
  }
}