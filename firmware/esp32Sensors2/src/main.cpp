#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- ΡΥΘΜΙΣΕΙΣ WIFI ---
const char* ssid = "INALAN_2.4G_tdiEiP";
const char* password = "X2DKiYdC";

// --- ΡΥΘΜΙΣΕΙΣ HIVEMQ CLOUD ---
const char* mqtt_server = "f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud"; // Το URL σου (χωρίς το mqtts:// στην αρχή)
const int mqtt_port = 8883; // Το port για ασφαλή σύνδεση
const char* mqtt_user = "ntomata"; // π.χ. ntomata
const char* mqtt_password = "Ntomata1"; 

// Ορισμός Clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Μεταβλητές για χρονόμετρο (για να μη στέλνουμε συνέχεια)
unsigned long lastMsg = 0;
const long interval = 5000; // Αποστολή δεδομένων κάθε 5 δευτερόλεπτα

// Συνάρτηση για σύνδεση στο WiFi
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

// Συνάρτηση για σύνδεση (και επανασύνδεση) στο HiveMQ
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Δημιουργούμε ένα τυχαίο Client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Προσπάθεια σύνδεσης με Username και Password
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
  
  setup_wifi();

  // Απαραίτητο για το HiveMQ Cloud (παρακάμπτει τον έλεγχο πιστοποιητικού για ευκολία)
  espClient.setInsecure(); 
  
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Κάθε 'interval' (5 δευτερόλεπτα) στέλνουμε δεδομένα
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    // Εδώ βάζεις τις πραγματικές τιμές των αισθητήρων σου. 
    // Για τώρα βάζουμε τυχαίες τιμές για να κάνουμε τη δοκιμή μας!
    float temp = random(200, 300) / 10.0; // Τυχαία θερμοκρασία 20.0 - 30.0
    float hum = random(400, 600) / 10.0;  // Τυχαία υγρασία 40.0 - 60.0

    // Φτιάχνουμε το JSON
    JsonDocument doc; // Χρήση του νέου API της ArduinoJson (v7)
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["device"] = "ESP32_Node_1";

    // Μετατροπή του JSON σε κείμενο (string) για να το στείλουμε
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    // Αποστολή (Publish) στο topic "esp32/sensors/data"
    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);
    client.publish("esp32/sensors/data", jsonBuffer);
  }
}