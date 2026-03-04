#include "lru_cache.hpp"
#include <shared_mutex>

namespace kvolt {

// Constructor — initialize dummy head and tail nodes
LRUCache::LRUCache(size_t capacity) : capacity_(capacity) {
    head_ = new Node();
    tail_ = new Node();
    head_->next = tail_;
    tail_->prev = head_;
}

// Destructor — clean up all nodes
LRUCache::~LRUCache() {
    Node* curr = head_;
    while (curr) {
        Node* next = curr->next;
        delete curr;
        curr = next;
    }
}

// ─── Private Helpers ───────────────────────────────────────────────────────

// Remove a node from its current position in the list
void LRUCache::remove_node(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// Move a node to the front (most recently used position)
void LRUCache::move_to_front(Node* node) {
    remove_node(node);  // remove from current position first
    node->next = head_->next;
    node->prev = head_;
    head_->next->prev = node;
    head_->next = node;
}

// Evict the least recently used node (node just before tail)
void LRUCache::evict_lru() {
    if (map_.empty()) return;
    Node* lru = tail_->prev;
    remove_node(lru);
    map_.erase(lru->key);
    delete lru;
}

// Check if a node has expired
bool LRUCache::is_expired(Node* node) {
    if (!node->has_ttl) return false;
    return std::chrono::steady_clock::now() > node->expiry;
}

// ─── Public Interface ──────────────────────────────────────────────────────

std::string LRUCache::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it == map_.end()) return ""; // key not found

    Node* node = it->second;

    // If expired, delete and return ""
    if (is_expired(node)) {
        remove_node(node);
        map_.erase(it);
        delete node;
        return "";
    }

    // Move to front — this key was just accessed
    move_to_front(node);
    return node->value;
}

void LRUCache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = map_.find(key);

    if (it != map_.end()) {
        // Key exists — update value and move to front
        Node* node = it->second;
        node->value = value;
        if (ttl_seconds > 0) {
            node->has_ttl = true;
            node->expiry = std::chrono::steady_clock::now() +
                           std::chrono::seconds(ttl_seconds);
        } else {
            node->has_ttl = false;
        }
        move_to_front(node); // safe — node is already in list
        return;
    }

    // Key doesn't exist — create new node and insert at front directly
    Node* node = new Node();
    node->key = key;
    node->value = value;
    if (ttl_seconds > 0) {
        node->has_ttl = true;
        node->expiry = std::chrono::steady_clock::now() +
                       std::chrono::seconds(ttl_seconds);
    }

    // Insert at front manually (node not in list yet, can't use move_to_front)
    node->next = head_->next;
    node->prev = head_;
    head_->next->prev = node;
    head_->next = node;

    map_[key] = node;

    // If over capacity, evict LRU
    if (map_.size() > capacity_) {
        evict_lru();
    }
}

bool LRUCache::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it == map_.end()) return false;

    Node* node = it->second;
    remove_node(node);
    map_.erase(it);
    delete node;
    return true;
}

void LRUCache::evict_expired() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    Node* curr = head_->next;
    while (curr != tail_) {
        Node* next = curr->next;
        if (is_expired(curr)) {
            remove_node(curr);
            map_.erase(curr->key);
            delete curr;
        }
        curr = next;
    }
}

void LRUCache::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Delete all nodes except dummy head and tail
    Node* curr = head_->next;
    while (curr != tail_) {
        Node* next = curr->next;
        delete curr;
        curr = next;
    }

    // Reset list
    head_->next = tail_;
    tail_->prev = head_;
    map_.clear();
}

size_t LRUCache::size() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return map_.size();
}

} // namespace kvolt