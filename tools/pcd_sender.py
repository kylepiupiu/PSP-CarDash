#!/usr/bin/env python3
import math
import socket
import time

PORT = 4210
DEST = ("255.255.255.255", PORT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

seq = 0
start = time.monotonic()

print("PSP CAR DASH desktop UDP simulator")
print("Broadcasting to UDP 4210. Ctrl-C to stop.")

while True:
    t = time.monotonic() - start

    speed = max(0, 78 + int(20 * math.sin(t * 0.65)))
    rpm = max(700, 2700 + int(1050 * math.sin(t * 1.05)))
    coolant = 92 + int(2 * math.sin(t * 0.09))
    throttle = max(0, min(100, 29 + int(20 * math.sin(t * 0.81))))
    voltage10 = 141 + int(2 * math.sin(t * 0.17))
    intake = 31 + int(2 * math.sin(t * 0.21))
    load = max(0, min(100, 42 + int(18 * math.sin(t * 0.73))))
    fuel10 = 70 + int(18 * math.sin(t * 0.58))
    oiltemp = 96 + int(2 * math.sin(t * 0.11))
    oilpress10 = 36 + rpm // 1800

    pkt = (
        f"PCD1,{seq},{speed},{rpm},{coolant},{throttle},"
        f"{voltage10},{intake},{load},{fuel10},{oiltemp},{oilpress10}"
    )

    sock.sendto(pkt.encode("ascii"), DEST)
    print(pkt)
    seq += 1
    time.sleep(0.1)
