#pragma once

#include "lru_cache.hpp"
#include <thread>
#include <atomic>
#include <chrono>

namespace kvolt {

class TTLManager {
public:
    // Start background cleanup thread
    explicit TTLManager(LRUCache& cache, int interval_seconds = 1);

    // Stop the thread cleanly
    ~TTLManager();

    // Non-copyable
    TTLManager(const TTLManager&) = delete;
    TTLManager& operator=(const TTLManager&) = delete;

private:
    LRUCache& cache_;
    int interval_seconds_;
    std::atomic<bool> running_;  // flag to stop the thread
    std::thread thread_;

    void run();
};

} 