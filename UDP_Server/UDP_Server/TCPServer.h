#pragma once
#include <optional>
#include <SFML/Network.hpp>

class TCPServer {
private:
    sf::TcpSocket socket;

    std::optional<sf::IpAddress> address;
    unsigned short port = 0;

    bool connected = false;

public:
    TCPServer()
        : address(std::nullopt), port(0), connected(false) {}

    TCPServer(const sf::IpAddress& address, unsigned short port)
        : address(address), port(port), connected(false) {}

    inline sf::TcpSocket& GetSocket() { return socket; }

    inline bool IsConnected() const { return connected; }

    inline bool HasAddresAndPort() const {
        return address.has_value() && port != 0;
    }

    inline bool Connect() {
        if (!HasAddresAndPort()) return false;

        connected = socket.connect(address.value(), port) == sf::Socket::Status::Done;
        return connected;
    }

    inline void Disconnect() {
        socket.disconnect();
        connected = false;
    }

    inline bool Send(sf::Packet& packet) {
        if (!connected) return false;
        return socket.send(packet) == sf::Socket::Status::Done;
    }
};