#include <iostream>
#include <thread>
#include <chrono>
#include "cache/lru_cache.hpp"

int main() {
    kvolt::LRUCache cache(3); // capacity of 3 keys

    // Test SET and GET
    cache.set("name", "John");
    cache.set("city", "Chicago");
    cache.set("lang", "C++");

    std::cout << "GET name: " << cache.get("name") << "\n";   // John
    std::cout << "GET city: " << cache.get("city") << "\n";   // Chicago
    std::cout << "GET lang: " << cache.get("lang") << "\n";   // C++

    // Test LRU eviction — adding 4th key should evict least recently used
    // "name" was accessed most recently, "city" least recently
    // Access order from last: lang, city, name (name is MRU, city is LRU)
    // Let's access name and lang to make city the LRU
    cache.get("name");
    cache.get("lang");
    cache.set("country", "USA"); // should evict "city"

    std::cout << "\nAfter eviction:\n";
    std::cout << "GET city: '" << cache.get("city") << "'\n";     // should be "" (evicted)
    std::cout << "GET country: " << cache.get("country") << "\n"; // USA

    // Test DEL
    cache.del("name");
    std::cout << "\nAfter DEL name:\n";
    std::cout << "GET name: '" << cache.get("name") << "'\n"; // should be ""

    // Test TTL
    cache.set("session", "abc123", 2); // expires in 2 seconds
    std::cout << "\nTTL test:\n";
    std::cout << "GET session (before expiry): " << cache.get("session") << "\n"; // abc123
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "GET session (after expiry):  '" << cache.get("session") << "'\n"; // ""

    std::cout << "\nAll tests passed!\n";
    return 0;
}