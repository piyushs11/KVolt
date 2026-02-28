#include <iostream>
#include <thread>
#include <chrono>
#include "cache/lru_cache.hpp"
#include "cache/ttl_manager.hpp"

int main() {
    kvolt::LRUCache cache(10);
    kvolt::TTLManager ttl_manager(cache, 1); // cleanup every 1 second

    // Set keys with TTL
    cache.set("session1", "abc123", 2); // expires in 2 seconds
    cache.set("session2", "xyz789", 5); // expires in 5 seconds
    cache.set("permanent", "hello");    // no TTL

    std::cout << "Initial state:\n";
    std::cout << "Cache size: " << cache.size() << "\n"; // 3

    std::cout << "\nWaiting 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nAfter 3 seconds:\n";
    std::cout << "Cache size: " << cache.size() << "\n";          // should be 2
    std::cout << "GET session1: '" << cache.get("session1") << "'\n"; // "" expired
    std::cout << "GET session2: " << cache.get("session2") << "\n";   // xyz789
    std::cout << "GET permanent: " << cache.get("permanent") << "\n"; // hello

    std::cout << "\nWaiting 3 more seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nAfter 6 seconds total:\n";
    std::cout << "Cache size: " << cache.size() << "\n";          // should be 1
    std::cout << "GET session2: '" << cache.get("session2") << "'\n"; // "" expired
    std::cout << "GET permanent: " << cache.get("permanent") << "\n"; // hello

    std::cout << "\nTTL Manager test passed!\n";
    return 0;
}