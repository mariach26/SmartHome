import smtplib

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

latest_data = {
    "water_pct": 0,
    "fire_alert": False,
    "shock_alert": False,
    "radar_alert": False,
    "device": "Waiting..."
}

email_sent=False



def send_my_email(subject,message):
    email="mariachatz26@gmail.com"
    receiver_email="maraki279@gmail.com"
    text=f"Subject: {subject}\n\n{message}".encode('utf-8')
    server= smtplib.SMTP("smtp.gmail.com", 587)
    server.starttls()
    server.login(email, "")
    server.sendmail(email, receiver_email, text)
    server.quit()
    print("email has been sent to "+ receiver_email)


# --- MQTT LOGIC ---
def on_message(client, userdata, msg):
    global latest_data
    try:
        new_payload = json.loads(msg.payload.decode())

        # Λεξικό για να μεταφράζουμε τα κλειδιά σε ωραία Ελληνικά
        sensor_names = {
            "water_pct": "Στάθμη Νερού (%)",
            "fire_alert": "Αισθητήρας Φωτιάς",
            "shock_alert": "Αισθητήρας Κραδασμών",
            "radar_alert": "Αισθητήρας Κίνησης (Radar)",
            "device": "Κατάσταση Συσκευής"
        }

        changes = []
        for key in new_payload:
            # Αν η τιμή είναι διαφορετική από την τελευταία αποθηκευμένη
            if key in latest_data and new_payload[key] != latest_data[key]:
                # Παίρνουμε το ελληνικό όνομα αν υπάρχει, αλλιώς κρατάμε το αγγλικό κλειδί
                friendly_name = sensor_names.get(key, key)
                
                # Προσθήκη στη λίστα αλλαγών
                change_text = f"Εντοπίστηκε αλλαγή στον αισθητήρα: {friendly_name} ({latest_data[key]} -> {new_payload[key]})"
                changes.append(change_text)
                
                # Εκτύπωση στην κονσόλα για κάθε αλλαγή ξεχωριστά
                print(change_text)

        # Αν βρέθηκαν αλλαγές, στείλε το email
        if changes:
            changes_str = "\n".join(changes)
            message_body = f"Ειδοποίηση Συστήματος:\n\n{changes_str}"
            
            # Ενημέρωση της μνήμης
            latest_data = new_payload 
            
            # Αποστολή email σε ξεχωριστό thread (για να μην κολλάει το MQTT)
            threading.Thread(
                target=send_my_email, 
                args=("ΣΥΝΑΓΕΡΜΟΣ - Αλλαγή Κατάστασης", message_body)
            ).start()
            
    except Exception as e:
        print(f"Error: {e}")


mqtt_client = mqtt.Client()
mqtt_client.tls_set()
mqtt_client.username_pw_set("ntomata", "Ntomata1")
mqtt_client.on_message = on_message
mqtt_client.connect("f1f3a9c62d2c4c4baff79aa00bdfdcde.s1.eu.hivemq.cloud", 8883)
mqtt_client.subscribe("esp32/sensors/data")


threading.Thread(target=mqtt_client.loop_forever, daemon=True).start()

# --- API ENDPOINT ---
@app.get("/api/status")
async def get_status():
    return latest_data

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
