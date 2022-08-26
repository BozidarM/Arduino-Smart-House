from flask import Flask, render_template, jsonify, request
import serial
from threading import Thread
import time
from tempfile import tempdir
import urllib.request
from KontrolaEmail import sendEmail
from datetime import datetime

PORT = "COM6"
BAUD_RATE = 9600
TSKEY = "MM96V1T7ZMKCB13T"


objectArduino = {}

objectArduino['DC_MOTOR'] = {"value": 0, "lastUpdate": "00:00:00"}
objectArduino['BRIGHTNESS_PIN'] = {"value": 0, "lastUpdate": "00:00:00"}
objectArduino['TEMP_PIN'] = {"value": 0, "lastUpdate": "00:00:00"}
objectArduino['DOOR_COUNTER'] = {"value": 0, "lastUpdate": "00:00:00"}
objectArduino['RELAY_COUNTER'] = {"value": 0, "lastUpdate": "00:00:00"}
objectArduino['LIGHT'] = {"value": "LOW", "lastUpdate": "00:00:00"}
objectArduino['SERVO_MOTOR'] = {"value": 0, "lastUpdate": "00:00:00"}

running = True
serialConnection = serial.Serial(PORT, BAUD_RATE)

def receive(serialConnection):
    global running
    
    while running:
        
        if serialConnection.in_waiting > 0:
            #b = python byte string
            messageFromArduino = serialConnection.read_until(b';').decode('ascii')
            processMessage(messageFromArduino)
        time.sleep(0.1)

def processMessage(message):
    # ODGOVOR : "ARDUINO_ID:PIN|VREDNOST;ARDUINO_ID:PIN|VREDNOST;"
    splitObjects = message[:-1].split(";")

    for obj in splitObjects:
        explode = obj[:-1].split(":")
        arudinoId = int(explode[0])
        pin = str(explode[1].split("|")[0])
        value = float(explode[1].split("|")[1])

        objectArduino[pin]['value'] = value

    urllib.request.urlopen('https://api.thingspeak.com/update?api_key={}&field1={}&field2={}&field3={}&field4={}'.format(TSKEY, objectArduino['TEMP_PIN']['value'], objectArduino['BRIGHTNESS_PIN']['value'], objectArduino['DOOR_COUNTER']['value'],objectArduino['RELAY_COUNTER']['value']))

threadReceiver = Thread(target=receive, args=(serialConnection,))
threadReceiver.start()

threadReceiver = Thread(target=sendEmail)
threadReceiver.start()

app = Flask(__name__)

@app.route('/')
def dashboard():
    global objectArduino
    return render_template("dashboard.html", data=objectArduino)

@app.route('/change/<pin_id>', methods=['GET'])
def change(pin_id):
    global serialConnection
    global objectArduino
    text = getWriteMessageWithoutValue(0, pin_id)
    if (pin_id == 'RELAY_PIN'):
        objectArduino['RELAY_COUNTER']['lastUpdate'] = str(datetime.now())
    else:
        objectArduino[pin_id]['lastUpdate'] = str(datetime.now())
    serialConnection.write(text.encode('ascii'))
    return render_template("dashboard.html", data=objectArduino)

@app.route('/setMotorSpeed/<pin_id>/<value>', methods=['GET'])
def setMotorSpeed(pin_id, value):
    global serialConnection
    global objectArduino
    text = getWriteMessage(0, pin_id, value)
    objectArduino[pin_id]['lastUpdate'] = str(datetime.now())
    serialConnection.write(text.encode('ascii'))
    return render_template("dashboard.html", data=objectArduino)

def getWriteMessage(controllerId, pin, value):
    return str(controllerId) + ":W:" + str(pin) + ":" + str(value) + ";"

def getWriteMessageWithoutValue(controllerId, pin):
    return str(controllerId) + ":W:" + str(pin) + ";"

if __name__ == "__main__":
    app.run(port=5000, debug=True)