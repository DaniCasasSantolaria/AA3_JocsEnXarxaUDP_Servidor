#pragma once

#include <SFML/Network.hpp>
#include <map>
#include <string>
#include "Client.h"
#include "TCPServer.h"

#define PM PacketManager::Instance()

#define MAX_PLAYERS 4

enum tcpServerPacketType {
    TCP_HANDSHAKE,
    TCP_PLAYER_AUTHORIZED,
    TCP_MATCH_CREATED,
    TCP_MATCH_CLOSED,
    TCP_GAME_RESULT
};

// UDP_MOVEMENT debe seguir siendo 0 para cuadrar con el cliente actual.
enum udpClientPacketType {
    UDP_MOVEMENT,
    UDP_REGISTER_CLIENT,
    UDP_SHOOT,
    UDP_HIT,
    UDP_PING
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

    std::string MakeEndpointKey(const sf::IpAddress& ip, unsigned short port);

    Client* GetClientByEndpoint(const sf::IpAddress& ip, unsigned short port);
    bool RegisterClientEndpoint(unsigned short clientId, const sf::IpAddress& ip, unsigned short port);

    PacketManager() = default;
    PacketManager(PacketManager&) = delete;
    PacketManager& operator =(const PacketManager&) = delete;
    ~PacketManager() = default;

public:
    inline static PacketManager* Instance() {
        static PacketManager instance;
        return &instance;
    }

    void SetTCPServer(TCPServer* server);

    void HandleTCPServerPacket(sf::Packet& packet);

    void HandleUDPClientPacket(const char* buffer, std::size_t receivedSize, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);

    void HandleUDPRegisterClient(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort);

    void HandleUDPMovement(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);

    void SendValidatedMovement(sf::UdpSocket& udpSocket, const Client& client);
    void BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient);
};