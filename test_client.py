import socket

def send_command(sock, command):
    sock.sendall((command + "\r\n").encode())
    response = sock.recv(4096).decode().strip()
    print(f"{command} => {response}")

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 6379))

# Test PING
send_command(s, "PING")

# Test FLUSH
send_command(s, "SET key1 value1")
send_command(s, "SET key2 value2")
send_command(s, "SET key3 value3")
send_command(s, "SIZE")
send_command(s, "FLUSH")
send_command(s, "SIZE")
send_command(s, "GET key1")

s.close()