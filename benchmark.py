import socket
import threading
import time
import random
import string

# ── Config ────────────────────────────────────────────────────────────────
HOST = "127.0.0.1"
PORT = 6379
NUM_THREADS = 500        # concurrent clients
OPS_PER_THREAD = 200    # operations per client
TOTAL_OPS = NUM_THREADS * OPS_PER_THREAD

# ── Shared counters ───────────────────────────────────────────────────────
success_count = 0
error_count = 0
latencies = []
lock = threading.Lock()

def random_key():
    return "key_" + "".join(random.choices(string.ascii_lowercase, k=6))

def send_command(sock, command):
    sock.sendall((command + "\r\n").encode())
    return sock.recv(4096).decode().strip()

def worker():
    global success_count, error_count

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))

        for _ in range(OPS_PER_THREAD):
            key = random_key()
            value = "val_" + key

            start = time.perf_counter()

            # Mix of SET, GET, DEL operations
            op = random.choice(["SET", "GET", "DEL"])

            if op == "SET":
                response = send_command(s, f"SET {key} {value}")
                success = response == "OK"
            elif op == "GET":
                response = send_command(s, f"GET {key}")
                success = True  # NULL is valid response
            else:
                response = send_command(s, f"DEL {key}")
                success = response in ["0", "1"]

            end = time.perf_counter()
            latency_ms = (end - start) * 1000

            with lock:
                if success:
                    success_count += 1
                else:
                    error_count += 1
                latencies.append(latency_ms)

        s.close()

    except Exception as e:
        with lock:
            error_count += 1
        print(f"Thread error: {e}")

# ── Run benchmark ─────────────────────────────────────────────────────────
print(f"Starting benchmark: {NUM_THREADS} concurrent clients, {OPS_PER_THREAD} ops each")
print(f"Total operations: {TOTAL_OPS}\n")

start_time = time.perf_counter()

threads = []
for _ in range(NUM_THREADS):
    t = threading.Thread(target=worker)
    threads.append(t)

# Start all threads simultaneously
for t in threads:
    t.start()

# Wait for all to finish
for t in threads:
    t.join()

end_time = time.perf_counter()
total_time = end_time - start_time

# ── Results ───────────────────────────────────────────────────────────────
latencies.sort()
p50 = latencies[int(len(latencies) * 0.50)]
p95 = latencies[int(len(latencies) * 0.95)]
p99 = latencies[int(len(latencies) * 0.99)]
ops_per_sec = TOTAL_OPS / total_time

print(f"Results:")
print(f"  Total time:      {total_time:.2f}s")
print(f"  Ops/sec:         {ops_per_sec:.0f}")
print(f"  Success:         {success_count}/{TOTAL_OPS}")
print(f"  Errors:          {error_count}")
print(f"  Latency p50:     {p50:.2f}ms")
print(f"  Latency p95:     {p95:.2f}ms")
print(f"  Latency p99:     {p99:.2f}ms")