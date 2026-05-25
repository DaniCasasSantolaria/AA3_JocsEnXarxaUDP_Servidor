#pragma once

#include <SFML/Network.hpp>
#include <map>
#include <string>
#include "Client.h"
#include "TCPServer.h"
#include "Match.h"

#define PM PacketManager::Instance()

#define MAX_PLAYERS 2

//TCP Paquetes
enum packetType {
    HANDSHAKE,
    LOGIN,
    REGISTER,
    RANKING,
    MATCHMAKE,
    WIN_NOTIFICATION,
    GAME_RESULT,
    MAP_REQUEST,
    SERVER_HANDSHAKE,
    MATCH_CREATED
};

enum matchMode {
    NON_COMPETITIVE,
    COMPETITIVE
};

// UDP_MOVEMENT debe seguir siendo 0 para cuadrar con el cliente actual.
enum udpPacketType {
    MOVEMENT,
    REGISTER_CLIENT,
    SHOOT,
    HIT,
    PING
};

enum movementPacketType {
    SEND_RAW_MOVEMENT,
    RECEIVE_VALIDATED_MOVEMENT
};

class PacketManager {
private:
    TCPServer* tcpServer = nullptr;

    std::map<unsigned short, Client> clients;
    std::map<std::string, unsigned short> endpointToClientId;

    std::map<unsigned int, Match> activeMatches;
    std::map<unsigned short, unsigned int> clientToMatchId;

    inline std::string MakeIPKey(const std::string& ip, unsigned short port) {
        return ip + ":" + std::to_string(port);
    }

    Client* GetClientByIP(const std::string& ip, unsigned short port);
    bool RegisterClientIP(unsigned short clientId, const std::string& ip, unsigned short port);

    PacketManager() = default;
    PacketManager(PacketManager&) = delete;
    PacketManager& operator =(const PacketManager&) = delete;
    ~PacketManager() = default;

public:
    inline static PacketManager* Instance() {
        static PacketManager instance;
        return &instance;
    }

    inline void SetTCPServer(TCPServer* server) {
        tcpServer = server;
    }

    void HandleTCPServerPacket(sf::Packet& packet);

    void HandleUDPClientPacket(const char* buffer, std::size_t receivedSize, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);

    void HandleUDPMovement(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);

    void SendValidatedMovement(sf::UdpSocket& udpSocket, const Client& client);
    void BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient);
};