#!/usr/bin/env python3

import random
import socket

PORT = 4242

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('', PORT))

while True:
    rand = random.randint(0, 10)
    message, address = server_socket.recvfrom(1024)
    print(address)
    message = message.upper()
    if rand >= 4:
        server_socket.sendto(message, address)


