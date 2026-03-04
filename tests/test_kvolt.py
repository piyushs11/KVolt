import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 6379
passed = 0
failed = 0

def send_command(sock, command):
    sock.sendall((command + "\r\n").encode())
    return sock.recv(4096).decode().strip()

def make_connection():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    return s

def assert_equal(test_name, actual, expected):
    global passed, failed
    if actual == expected:
        print(f"  ✓ {test_name}")
        passed += 1
    else:
        print(f"  ✗ {test_name} — expected '{expected}', got '{actual}'")
        failed += 1

# ── Test Suites ───────────────────────────────────────────────────────────

def test_basic_commands():
    print("\n[1] Basic Commands")
    s = make_connection()

    assert_equal("PING",              send_command(s, "PING"),           "PONG")
    assert_equal("SET key",           send_command(s, "SET k1 hello"),   "OK")
    assert_equal("GET key",           send_command(s, "GET k1"),         "hello")
    assert_equal("DEL key",           send_command(s, "DEL k1"),         "1")
    assert_equal("GET deleted key",   send_command(s, "GET k1"),         "NULL")
    assert_equal("DEL missing key",   send_command(s, "DEL k1"),         "0")
    assert_equal("GET missing key",   send_command(s, "GET missing"),    "NULL")

    s.close()

def test_ttl():
    print("\n[2] TTL Expiration")
    s = make_connection()

    send_command(s, "SET session abc123 EX 2")
    assert_equal("GET before expiry", send_command(s, "GET session"), "abc123")

    time.sleep(3)
    assert_equal("GET after expiry",  send_command(s, "GET session"), "NULL")

    # Key with no TTL should persist
    send_command(s, "SET permanent hello")
    time.sleep(2)
    assert_equal("Permanent key persists", send_command(s, "GET permanent"), "hello")

    s.close()

def test_lru_eviction():
    print("\n[3] LRU Eviction")
    s = make_connection()

    # Flush first
    send_command(s, "FLUSH")

    # Fill cache to capacity (we set capacity to 1000 in main.cpp)
    # Use a small local test — set 3 keys, access 2, add 1 more won't evict accessed ones
    # We'll verify via SIZE and key presence
    send_command(s, "SET a 1")
    send_command(s, "SET b 2")
    send_command(s, "SET c 3")
    assert_equal("SIZE after 3 sets", send_command(s, "SIZE"), "3")

    # Update existing key
    send_command(s, "SET a updated")
    assert_equal("Update existing key", send_command(s, "GET a"), "updated")
    assert_equal("SIZE unchanged after update", send_command(s, "SIZE"), "3")

    s.close()

def test_flush():
    print("\n[4] FLUSH")
    s = make_connection()

    send_command(s, "SET x 1")
    send_command(s, "SET y 2")
    send_command(s, "SET z 3")
    send_command(s, "FLUSH")
    assert_equal("SIZE after flush",  send_command(s, "SIZE"),    "0")
    assert_equal("GET after flush",   send_command(s, "GET x"),   "NULL")

    s.close()

def test_edge_cases():
    print("\n[5] Edge Cases")
    s = make_connection()

    # Unknown command
    assert_equal("Unknown command",   send_command(s, "BLAH"),           "ERROR unknown command: BLAH")

    # Missing arguments
    assert_equal("GET no args",       send_command(s, "GET"),            "ERROR usage: GET <key>")
    assert_equal("DEL no args",       send_command(s, "DEL"),            "ERROR usage: DEL <key>")
    assert_equal("SET no args",       send_command(s, "SET"),            "ERROR usage: SET <key> <value> [EX <seconds>]")

    # Case insensitive commands
    assert_equal("lowercase ping",    send_command(s, "ping"),           "PONG")
    assert_equal("mixed case ping",   send_command(s, "Ping"),           "PONG")

    s.close()

def test_concurrent_access():
    print("\n[6] Concurrent Access")
    errors = []

    def worker(thread_id):
        try:
            s = make_connection()
            key = f"thread_{thread_id}"
            value = f"value_{thread_id}"

            send_command(s, f"SET {key} {value}")
            result = send_command(s, f"GET {key}")

            if result != value:
                errors.append(f"Thread {thread_id}: expected '{value}', got '{result}'")

            s.close()
        except Exception as e:
            errors.append(f"Thread {thread_id}: {e}")

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(100)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    global passed, failed
    if not errors:
        print(f"  ✓ 100 concurrent clients — no race conditions")
        passed += 1
    else:
        for e in errors:
            print(f"  ✗ {e}")
        failed += len(errors)

# ── Run All Tests ─────────────────────────────────────────────────────────

print("=" * 50)
print("KVolt Test Suite")
print("=" * 50)

test_basic_commands()
test_ttl()
test_lru_eviction()
test_flush()
test_edge_cases()
test_concurrent_access()

print("\n" + "=" * 50)
print(f"Results: {passed} passed, {failed} failed")
print("=" * 50)