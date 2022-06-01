import socket
import threading
import time

PORT_SENDER   = 6342
PORT_RECEIVER = 6343

data_dict = {}

def listen_to_sender():
	print("listen_to_sender() - START\n")
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.bind(('0.0.0.0',PORT_SENDER))
	s.listen()
	while True:
		conn, addr = s.accept()
		data_dict['data'] = []
		while True:
			try: nibble = conn.recv(1)
			except: break
			if not nibble: break
			data_dict['data'].append(nibble)
		try: conn.close()
		except: pass
		print("data = " + str(data_dict['data']) + "\n")

def listen_to_receiver():
	print("listen_to_receiver() - START\n")
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.bind(('0.0.0.0',PORT_RECEIVER))
	s.listen()
	while True:
		conn, addr = s.accept()
		print("sending data = " + str(data_dict['data']) + "\n")
		for nibble in data_dict['data']:
			conn.send(nibble)
		try: conn.close()
		except: pass


threading.Thread(target=listen_to_sender,   name=None, args=[]).start()
threading.Thread(target=listen_to_receiver, name=None, args=[]).start()


print(f"running on ports {PORT_SENDER}, {PORT_RECEIVER}")
while True:
	time.sleep(1)
