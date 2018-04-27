import serial
import random
import struct
import time

s = serial.Serial("/dev/ttyUSB0", baudrate=9600)

while True:
    x = random.randint(0, 100)
    y = random.randint(0, 100)
    for x in range(0, 255):
        for y in range(0, 255, 10):
            packed = struct.pack("BB", x, y)

            s.write(packed)
            print(packed)
            time.sleep(0.001)


while True:
    print(s.read())
