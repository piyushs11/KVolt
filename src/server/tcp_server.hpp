#pragma once

#include <string>
#include <atomic>
#include "cache/lru_cache.hpp"
#include "cache/ttl_manager.hpp"

namespace kvolt {

class TCPServer {
public:
    explicit TCPServer(int port, size_t cache_capacity = 1000);
    ~TCPServer();

    // Start listening — blocks until stop() is called
    void start();

    // Signal server to stop
    void stop();

private:
    int port_;
    int server_fd_;                // listening socket file descriptor
    std::atomic<bool> running_;
    LRUCache cache_;
    TTLManager ttl_manager_;

    // Handle a single client connection — runs in its own thread
    void handle_client(int client_fd);

    // Parse and execute a command, return response string
    std::string process_command(const std::string& command);
};

} 