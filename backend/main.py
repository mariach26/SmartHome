import paho.mqtt.client as mqtt
import json
import time

# --- ΡΥΘΜΙΣΕΙΣ HIVEMQ CLOUD ---
MQTT_SERVER = "f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_USER = "ntomata"
MQTT_PASSWORD = "Ntomata1"
MQTT_TOPIC = "esp32/sensors/data"

# Συναρτήση που εκτελείται όταν συνδεόμαστε στον Broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to HiveMQ Cloud!")
        # Κάνουμε subscribe στο topic που στέλνει το ESP32
        client.subscribe(MQTT_TOPIC)
        print(f"📡 Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"❌ Connection failed with code {rc}")

# Συναρτήση που εκτελείται όταν έρχεται νέο μήνυμα
def on_message(client, userdata, msg):
    try:
        # Μετατροπή του payload από bytes σε string και μετά σε JSON (dictionary)
        data = json.loads(msg.payload.decode())
        
        print("\n--- New Data Received ---")
        device = data.get("device", "Unknown")
        water = data.get("water_percent", 0)
        fire = "🔥 ALERT" if data.get("fire_alarm") else "Safe"
        shock = "⚠️ SHOCK" if data.get("shock_alarm") else "Stable"
        radar = "🚶 MOTION" if data.get("radar_alarm") else "Clear"

        print(f"Device: {device}")
        print(f"Water Level: {water}%")
        print(f"Fire Status: {fire}")
        print(f"Shock Status: {shock}")
        print(f"Radar Status: {radar}")
        
        # Εδώ μπορείς να προσθέσεις κώδικα για αποθήκευση σε Database 
        # ή αποστολή ειδοποίησης στο κινητό σου.

    except Exception as e:
        print(f"Error parsing JSON: {e}")

# Ρύθμιση του Client
client = mqtt.Client()

# Επειδή το HiveMQ χρησιμοποιεί TLS/SSL (θύρα 8883), ενεργοποιούμε το secure connection
client.tls_set() 
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

# Ορισμός των callbacks
client.on_connect = on_connect
client.on_message = on_message

# Σύνδεση
print("Connecting to Broker...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
except Exception as e:
    print(f"Could not connect: {e}")
    exit()

# Ξεκινάει το loop που περιμένει μηνύματα για πάντα
client.loop_forever()
