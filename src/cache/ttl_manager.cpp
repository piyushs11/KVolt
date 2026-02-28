#include "ttl_manager.hpp"

namespace kvolt {

TTLManager::TTLManager(LRUCache& cache, int interval_seconds)
    : cache_(cache),
      interval_seconds_(interval_seconds),
      running_(true),
      thread_(&TTLManager::run, this)  // start background thread immediately
{}

TTLManager::~TTLManager() {
    running_ = false;          // signal thread to stop
    thread_.join();            // wait for thread to finish cleanly
}

void TTLManager::run() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds_));
        if (running_) {
            cache_.evict_expired();
        }
    }
}

} 