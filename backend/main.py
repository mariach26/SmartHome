import paho.mqtt.client as mqtt
import json
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
import threading

app = FastAPI()

# Επιτρέπουμε στο React να διαβάζει τα δεδομένα (CORS)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Εδώ αποθηκεύουμε την τελευταία κατάσταση των αισθητήρων
latest_data = {
    "water_pct": 0,
    "fire_alert": False,
    "shock_alert": False,
    "radar_alert": False,
    "device": "Waiting..."
}

# --- MQTT LOGIC ---
def on_message(client, userdata, msg):
    global latest_data
    try:
        latest_data = json.loads(msg.payload.decode())
    except Exception as e:
        print(f"Error: {e}")

mqtt_client = mqtt.Client()
mqtt_client.tls_set()
mqtt_client.username_pw_set("ntomata", "Ntomata1")
mqtt_client.on_message = on_message
mqtt_client.connect("f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud", 8883)
mqtt_client.subscribe("esp32/sensors/data")

# Τρέχουμε το MQTT σε δικό του thread για να μην κολλάει το API
threading.Thread(target=mqtt_client.loop_forever, daemon=True).start()

# --- API ENDPOINT ---
@app.get("/api/status")
async def get_status():
    return latest_data

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
