#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <chrono>

namespace kvolt {

// A single node in our doubly linked list
struct Node {
    std::string key;
    std::string value;
    std::chrono::steady_clock::time_point expiry; // TTL expiry time
    bool has_ttl = false;                          // whether TTL is set
    Node* prev = nullptr;
    Node* next = nullptr;
};

class LRUCache {
public:
    explicit LRUCache(size_t capacity);
    ~LRUCache();

    // Returns value if key exists and hasn't expired, else returns ""
    std::string get(const std::string& key);

    // Set key-value. Optional ttl_seconds = 0 means no expiry
    void set(const std::string& key, const std::string& value, int ttl_seconds = 0);

    // Delete a key. Returns true if key existed
    bool del(const std::string& key);

    // Remove all expired keys — called by background thread
    void evict_expired();

    // Current number of keys
    size_t size();

private:
    size_t capacity_;
    std::unordered_map<std::string, Node*> map_;
    std::shared_mutex mutex_;  // allows concurrent reads, exclusive writes

    // Doubly linked list — head = most recent, tail = least recent
    Node* head_; // dummy head
    Node* tail_; // dummy tail

    // Internal helpers — call these only when lock is already held
    void move_to_front(Node* node);
    void remove_node(Node* node);
    void evict_lru();
    bool is_expired(Node* node);
};

} // namespace kvolt