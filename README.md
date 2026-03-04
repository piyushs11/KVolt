# KVolt ⚡

A high-performance, concurrent in-memory key-value store written in C++17 - inspired by Redis.

Built from scratch with a custom LRU eviction engine, TTL-based expiration, and a multi-threaded TCP server handling thousands of concurrent connections.

---

## Benchmark Results

| Metric | Value |
|---|---|
| Throughput | 24,000+ ops/sec |
| Latency p50 | 1.02ms |
| Latency p95 | 4.37ms |
| Latency p99 | 6.53ms |
| Concurrent clients | 500 |
| Total operations | 100,000 |
| Error rate | 0% |

> Benchmarked on Windows with 500 concurrent clients sending randomized GET/SET/DEL operations.

---

## Architecture
```
┌─────────────────────────────────────────┐
│            TCP Server (port 6379)       │
│         One thread per connection       │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│           Command Parser                │
│   SET / GET / DEL / TTL / FLUSH / PING  │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│           LRU Cache Engine              │
│   HashMap + Doubly Linked List          │
│   std::shared_mutex (concurrent R/W)    │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│           TTL Manager                   │
│   Background thread, sweeps every 1s    │
└─────────────────────────────────────────┘
```

---

## Features

- **O(1) GET/SET/DEL** via HashMap + Doubly Linked List
- **LRU Eviction** - automatically evicts least recently used key when capacity is exceeded
- **TTL Expiration** - keys auto-delete after N seconds
- **Thread Safety** - `std::shared_mutex` allows parallel reads, exclusive writes
- **Background Cleanup** - daemon thread sweeps expired keys every second
- **TCP Server** - telnet/socket compatible, one thread per client
- **Cross-platform** - runs natively on Windows and Linux (Docker)

---

## Commands

| Command | Description | Example |
|---|---|---|
| `SET key value` | Store a key | `SET name John` |
| `SET key value EX n` | Store with TTL (seconds) | `SET session abc EX 30` |
| `GET key` | Retrieve a value | `GET name` |
| `DEL key` | Delete a key | `DEL name` |
| `SIZE` | Number of keys in cache | `SIZE` |
| `FLUSH` | Clear all keys | `FLUSH` |
| `PING` | Health check | `PING` |

---

## Running Locally

**Prerequisites:** g++ (C++17), CMake, make
```bash
git clone https://github.com/piyushs11/KVolt.git
cd KVolt
mkdir build && cd build
cmake ..
cmake --build .
./kvolt
```

Server starts on port `6379`.

---

## Running with Docker
```bash
docker-compose up
```

---

## Running the Benchmark
```bash
python benchmark.py
```

---

## Project Structure
```
KVolt/
├── src/
│   ├── cache/
│   │   ├── lru_cache.hpp      # LRU Cache interface
│   │   ├── lru_cache.cpp      # HashMap + Doubly Linked List implementation
│   │   ├── ttl_manager.hpp    # TTL Manager interface
│   │   └── ttl_manager.cpp    # Background expiry thread
│   └── server/
│       ├── tcp_server.hpp     # TCP Server interface
│       └── tcp_server.cpp     # Socket handling, command parsing
├── src/main.cpp               # Entry point
├── benchmark.py               # Concurrent load testing script
├── test_client.py             # Manual testing client
├── Dockerfile                 # Multi-stage Docker build
├── docker-compose.yml         # Docker Compose config
└── CMakeLists.txt             # Build configuration
```

---

## Technical Deep Dive

### Why HashMap + Doubly Linked List?
A HashMap alone gives O(1) lookup but can't track usage order. A Doubly Linked List tracks order but gives O(n) lookup. Combined, we get O(1) for both - the HashMap stores pointers directly to list nodes, so we can move any node to the front in O(1) without searching.

### Why std::shared_mutex?
Key-value stores are read-heavy. A regular `std::mutex` would serialize all operations including reads, destroying throughput. `std::shared_mutex` allows unlimited concurrent readers while ensuring exclusive access for writes - exactly the right tradeoff for a cache.

### Why a background TTL thread instead of lazy expiration only?
Lazy expiration (deleting on access) leaves stale keys in memory indefinitely if they're never accessed again. The background thread guarantees memory is reclaimed within 1 second of expiry regardless of access patterns.