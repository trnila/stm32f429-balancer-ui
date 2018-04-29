#!/usr/bin/env python3
import logging
import queue
import argparse
import serial
import random
import struct
import time
import threading
import sseclient
import requests
import json
from cobs import cobs

parser = argparse.ArgumentParser()
parser.add_argument('--api-url', required=True, help='url of balancer api server')
parser.add_argument('--serial', required=True, help='path to serial device, ie. /dev/ttyUSB0')
parser.add_argument('--baudrate', default=9600, help='serial device baud rate')

args = parser.parse_args()

s = serial.Serial(args.serial, baudrate=args.baudrate, timeout=0.02)
response = requests.get("{}/events/measurements".format(args.api_url), stream=True)
client = sseclient.SSEClient(response)

width = 0
height = 0


Q = queue.Queue()


def position_encode(x, y):
    x = int(x * 255 / width)
    y = 255 - int(y * 255 / height)

    return x, y


def position_decode(x, y):
    x = int(x * width / 255)
    y = int(height - y * height / 255)
    return x, y


def command_processor():
    while True:
        data = Q.get()

        cmd = data[0]
        payload = data[1:]

        if cmd == Comm.CMD_SET_TARGET:
            x = payload[0]
            y = payload[1]

            x = int(x * width / 255)
            y = int(height - y * height / 255)

            # TODO: throttle
            requests.put("{}/api/set_target".format(args.api_url), json={'X': x, 'Y': y})
            print(x, y)
        else:
            logging.warning("Unknown command %d", cmd)


def uart_reader():
    buff = b""

    while True:
        pos = buff.find(b"\0")
        if pos >= 0:
            try:
                data = cobs.decode(buff[:pos])
                Q.put(data)
            except cobs.DecodeError as e:
                logging.exception(e)
            buff = buff[pos + 1:]
        buff += s.read(128)


class Comm:
    CMD_POS = 0x01
    CMD_TARGET = 0x02
    CMD_SET_TARGET = 0x82

    def __init__(self, s):
        self.s = s

    def set_target_pos(self, x, y):
        self.send(self.CMD_TARGET, bytes([x, y]))

    def set_pos(self, x, y):
        self.send(self.CMD_POS, bytes([x, y]))

    def send(self, cmd, data):
        packed = bytes([cmd]) + data
        encoded = cobs.encode(packed) + b"\0"
        self.s.write(encoded)


logging.basicConfig(level=logging.INFO)

t = threading.Thread(target=uart_reader)
t.setDaemon(True)
t.start()

t = threading.Thread(target=command_processor)
t.setDaemon(True)
t.start()

comm = Comm(s)

for event in client.events():
        payload = json.loads(event.data)

        if event.event == "dimension":
            width = payload['Width']
            height = payload['Height']
        elif event.event == 'measurement':
            if width != 0 and height != 0:
                x, y = position_encode(payload['POSX'], payload['POSY'])
                comm.set_pos(x, y)
            else:
                logging.info("No width, height received from balancer")
        elif event.event == 'target_position':
            x, y = position_encode(payload['X'], payload['Y'])

            logging.info("target_position = [%d, %d]", x, y)
            comm.set_target_pos(x, y)
        else:
            logging.info("Unknown sse event: %s", event.event)
