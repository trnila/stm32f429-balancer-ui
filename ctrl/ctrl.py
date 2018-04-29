import serial
import random
import struct
import time
import threading
import sseclient
import requests
import pprint
import json

BASE_ADDR = "http://172.20.6.43"

s = serial.Serial("/dev/ttyUSB0", baudrate=9600)

response = requests.get("{}/events/measurements".format(BASE_ADDR), stream=True)
client = sseclient.SSEClient(response)

width = 0
height = 0

def reader():
    while True:
        x, y = struct.unpack("BB", s.read(2))

        x = int(x * width / 255)
        y = int(height - y * height / 255)

        requests.put("{}/api/set_target".format(BASE_ADDR), json={'X': x, 'Y': y}) 

        print(x, y)


t = threading.Thread(target=reader)
t.setDaemon(True)
t.start()

for event in client.events():
        payload = json.loads(event.data)

        if event.event == "dimension":
            width = payload['Width']
            height = payload['Height']
        elif event.event == 'measurement' and width != 0 and height != 0:
            x = int(payload['POSX'] * 255 / width)
            y = 255 - int(payload['POSY'] * 255 / height)

#            print("curpos", x, y)

            packed = struct.pack("BB", x, y)
            s.write(packed)

while True:
    x = random.randint(0, 100)
    y = random.randint(0, 100)
    for x in range(0, 255):
        for y in range(0, 255, 10):
            packed = struct.pack("BB", x, y)

            s.write(packed)
            #print(packed)
            time.sleep(0.02)


