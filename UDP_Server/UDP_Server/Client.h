#pragma once

#include <optional>
#include <SFML/Network.hpp>

class Client
{
private:
    unsigned short id;

    std::optional<sf::IpAddress> address;
    unsigned short port = 0;

    float x = 0.0f;
    float y = 0.0f;
    unsigned int lastProcessedMovementID = 0;
    bool hasProcessedMovement = false;

    float lastMovementTime = 0.0f;
    bool hasMovementTime = false;

    float lastPacketTime = 0.0f;
    float lastPingTime = 0.0f;
    bool waitingPong = false;
    unsigned int lastPingId = 0;
    bool disconnected = false;

    //Validación de hacks
    unsigned short irregularityCount = 0;

public:
    Client()
        : id(0), address(std::nullopt), port(0) {}

    Client(unsigned short id)
        : id(id), address(std::nullopt), port(0) {}

    inline unsigned short GetId() const { return id; }

    inline std::optional<sf::IpAddress> GetIpAddress() const { return address; }
    inline unsigned short GetPort() const { return port; }
    inline bool HasAddresAndPort() const { return address.has_value() && port != 0; }
   
    inline void SetAddress(const std::optional<sf::IpAddress> newAddress) { address = newAddress; }
    inline void SetPort(unsigned short newPort) { port = newPort; }


    //Movimiento
    inline void SetPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }
    inline void SetLastProcessedMovementID(unsigned int newMovementID) {
        lastProcessedMovementID = newMovementID;
        hasProcessedMovement = true;
    }
    inline float GetLastMovementTime() const { return lastMovementTime; }
    inline bool HasMovementTime() const { return hasMovementTime; }
    inline void SetLastMovementTime(float time) {
        lastMovementTime = time;
        hasMovementTime = true;
    }
    inline float GetX() const { return x; }
    inline float GetY() const { return y; }
    inline unsigned int GetLastProcessedMovementID() const { return lastProcessedMovementID; }
    inline bool HasProcessedMovement() const { return hasProcessedMovement; }


    //Ping
    inline float GetLastPacketTime() const { return lastPacketTime; }
    inline float GetLastPingTime() const { return lastPingTime; }
    inline bool IsWaitingPong() const { return waitingPong; }
    inline unsigned int GetLastPingId() const { return lastPingId; }
    inline bool IsDisconnected() const { return disconnected; }

    inline void SetLastPacketTime(float time) { lastPacketTime = time; }
    inline void SetLastPingTime(float time) { lastPingTime = time; }
    inline void SetWaitingPong(bool waiting) { waitingPong = waiting; }
    inline void SetLastPingId(unsigned int pingId) { lastPingId = pingId; }
    inline void SetDisconnected(bool value) { disconnected = value; }


	//Validación de hacks
    inline unsigned short GetIrregularityCount() const { return irregularityCount; }

    inline void AddIrregularity() {
        irregularityCount++;
    }

    inline void ResetIrregularities() {
        irregularityCount = 0;
    }
};
