#pragma once

#include <string>
#include <optional>
#include <SFML/Network.hpp>

class Client
{
private:
    unsigned short id;
    std::string username;

    std::optional<sf::IpAddress> address;
    unsigned short port = 0;

    float x = 0.0f;
    float y = 0.0f;
    unsigned int lastProcessedMovementID = 0;

public:
    Client()
        : id(0), username(""), address(std::nullopt), port(0) {}

    Client(unsigned short id, const std::string& username)
        : id(id), username(username), address(std::nullopt), port(0) {}

    Client(unsigned short id, const std::string& username, const sf::IpAddress& address, unsigned short port)
        : id(id), username(username), address(address), port(port) {}

    inline unsigned short GetId() const { return id; }
    inline const std::string& GetUsername() const { return username; }

    inline std::string GetAddress() const {
        if (!address.has_value()) return "";
        return address.value().toString();
    }

    inline std::optional<sf::IpAddress> GetIpAddress() const { return address; }
    inline unsigned short GetPort() const { return port; }
    inline bool HasAddresAndPort() const { return address.has_value() && port != 0; }

    inline float GetX() const { return x; }
    inline float GetY() const { return y; }
    inline unsigned int GetLastProcessedMovementID() const { return lastProcessedMovementID; }

    inline void SetUsername(const std::string& newUsername) { username = newUsername; }
    inline void SetId(unsigned short newId) { id = newId; }
    inline void SetAddress(const sf::IpAddress& newAddress) { address = newAddress; }
    inline void SetPort(unsigned short newPort) { port = newPort; }

    inline void SetPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }

    inline void SetLastProcessedMovementID(unsigned int newMovementID) { lastProcessedMovementID = newMovementID; }
};