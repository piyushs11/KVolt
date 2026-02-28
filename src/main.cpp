#include <iostream>
#include "server/tcp_server.hpp"

int main() {
    kvolt::TCPServer server(6379, 1000); // port 6379, capacity 1000 keys
    server.start();
    return 0;
}