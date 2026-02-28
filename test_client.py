import socket

def send_command(sock, command):
    sock.sendall((command + "\r\n").encode())
    response = sock.recv(4096).decode().strip()
    print(f"{command} => {response}")

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 6379))

send_command(s, "SET name John")
send_command(s, "GET name")
send_command(s, "SET session abc123 EX 5")
send_command(s, "GET session")
send_command(s, "DEL name")
send_command(s, "GET name")
send_command(s, "SIZE")

s.close()