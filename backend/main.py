from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import paho.mqtt.client as mqtt
import ssl
import json

# --- ΡΥΘΜΙΣΕΙΣ HIVEMQ ---
BROKER = "f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud"
PORT = 8883
USERNAME = "ntomata"
PASSWORD = "Ntomata1"
TOPIC = "esp32/sensors/data"

# Εδώ θα αποθηκεύουμε προσωρινά τα τελευταία δεδομένα που έρχονται
latest_data = {
    "temperature": 0.0,
    "humidity": 0.0,
    "device": "Waiting for data..."
}

# --- ΡΥΘΜΙΣΗ FASTAPI ---
app = FastAPI()

# Προσθήκη CORS (Πολύ σημαντικό για να μην μπλοκάρει το React τις κλήσεις)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], 
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- ΣΥΝΑΡΤΗΣΕΙΣ MQTT ---
def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("✅ Συνδέθηκε στο HiveMQ!")
        client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    global latest_data
    payload = msg.payload.decode("utf-8")
    try:
        data = json.loads(payload)
        # Ενημερώνουμε τη μεταβλητή με τα νέα δεδομένα
        latest_data["temperature"] = data.get("temperature", 0)
        latest_data["humidity"] = data.get("humidity", 0)
        latest_data["device"] = data.get("device", "Unknown")
        print(f"📥 Νέα δεδομένα: {latest_data}")
    except json.JSONDecodeError:
        pass

# Ρύθμιση MQTT Client
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="FastAPI_Backend")
mqtt_client.tls_set(tls_version=ssl.PROTOCOL_TLS_CLIENT)
mqtt_client.username_pw_set(USERNAME, PASSWORD)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# Ξεκινάμε το MQTT στο background (loop_start αντί για loop_forever)
mqtt_client.connect(BROKER, PORT)
mqtt_client.loop_start()

# --- API ENDPOINTS ---
@app.get("/")
def read_root():
    return {"message": "Το Backend λειτουργεί κανονικά!"}

@app.get("/api/sensor-data")
def get_sensor_data():
    # Όταν το React ζητάει δεδομένα, του δίνουμε τα τελευταία που ήρθαν
    return latest_data