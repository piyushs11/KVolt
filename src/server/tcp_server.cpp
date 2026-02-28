#include "tcp_server.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <string>

// Windows socket headers
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace kvolt {

TCPServer::TCPServer(int port, size_t cache_capacity)
    : port_(port),
      server_fd_(-1),
      running_(false),
      cache_(cache_capacity),
      ttl_manager_(cache_, 1)
{}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::start() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    // Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return;
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // Bind to port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(server_fd_);
        WSACleanup();
        return;
    }

    // Listen for connections
    if (listen(server_fd_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(server_fd_);
        WSACleanup();
        return;
    }

    running_ = true;
    std::cout << "KVolt server running on port " << port_ << "\n";

    // Accept loop — each client gets its own thread
    while (running_) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) {
            if (running_) std::cerr << "Accept failed\n";
            break;
        }

        // Spawn thread for this client
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }

    closesocket(server_fd_);
    WSACleanup();
}

void TCPServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        closesocket(server_fd_);
        server_fd_ = -1;
    }
}

void TCPServer::handle_client(int client_fd) {
    char buffer[4096];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) break; // client disconnected

        std::string command(buffer, bytes);

        // Remove trailing \r\n
        while (!command.empty() && (command.back() == '\n' || command.back() == '\r')) {
            command.pop_back();
        }

        if (command.empty()) continue;

        std::string response = process_command(command);
        response += "\r\n";
        send(client_fd, response.c_str(), (int)response.size(), 0);
    }

    closesocket(client_fd);
}

std::string TCPServer::process_command(const std::string& command) {
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return "ERROR empty command";

    std::string cmd = tokens[0];

    // Convert command to uppercase
    for (auto& c : cmd) c = toupper(c);

    // GET key
    if (cmd == "GET") {
        if (tokens.size() < 2) return "ERROR usage: GET <key>";
        std::string value = cache_.get(tokens[1]);
        if (value.empty()) return "NULL";
        return value;
    }

    // SET key value [EX seconds]
    if (cmd == "SET") {
        if (tokens.size() < 3) return "ERROR usage: SET <key> <value> [EX <seconds>]";
        int ttl = 0;
        if (tokens.size() >= 5) {
            std::string ex = tokens[3];
            for (auto& c : ex) c = toupper(c);
            if (ex == "EX") {
                ttl = std::stoi(tokens[4]);
            }
        }
        cache_.set(tokens[1], tokens[2], ttl);
        return "OK";
    }

    // DEL key
    if (cmd == "DEL") {
        if (tokens.size() < 2) return "ERROR usage: DEL <key>";
        bool deleted = cache_.del(tokens[1]);
        return deleted ? "1" : "0";
    }

    // SIZE — returns number of keys
    if (cmd == "SIZE") {
        return std::to_string(cache_.size());
    }

    return "ERROR unknown command: " + tokens[0];
}

} 