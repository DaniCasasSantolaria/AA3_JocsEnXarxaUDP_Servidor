#pragma once

#include <SFML/Network.hpp>
#include <map>
#include <string>
#include "Client.h"
#include "TCPServer.h"
#include "Match.h"
#include <mutex>
#include <functional>
#include <queue>
#include <SFML/System.hpp>

#define PM PacketManager::Instance()

#define MAX_PLAYERS 2
#define NUM_MAX_THREADS 5

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

enum mapRequestType {
    MAP_VERSION_CHECK,
    MAP_UP_TO_DATE,
    MAP_UPDATE
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

    std::queue<std::function<void()>> taskQueue;

    //Reloj para el movimiento
    sf::Clock movementClock;

    //MUTEX
    std::mutex taskQueue_mutex;

    std::mutex udp_mutex;
    std::mutex cosole_mutex;
    std::mutex movement_mutex;


    //MAPA
    std::vector<std::string> mapLines;
    bool mapLoaded = false;

    bool LoadMap();
    bool IsSolidTile(char tile) const;
    bool IsPositionInsideSolid(float x, float y);


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

    void Worker();
    void AddTask(std::function<void()> task);

    void RequestMap();
    void HandleMapRequest(sf::Packet& packet);

    unsigned short LoadLocalMapVersion();
    void SaveLocalMap(const std::string& mapContent);
    void SaveLocalMapVersion(unsigned short version);
};