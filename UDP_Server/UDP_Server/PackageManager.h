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
#include <vector>
#include <SFML/System.hpp>

#define PM PacketManager::Instance()

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

enum mapRequestType {
    MAP_VERSION_CHECK,
    MAP_UP_TO_DATE,
    MAP_UPDATE
};

//UDP Paquetes
enum udpPacketType {
    MOVEMENT,
    REGISTER_CLIENT,
    SHOOT,
    HIT,
    PING,
    PONG,
    DISCONNECTED_PLAYER,
    IRREGULARITY_WARNING,
    MATCH_FINISHED
};

enum movementPacketType {
    SEND_RAW_MOVEMENT,
    RECEIVE_VALIDATED_MOVEMENT
};

class PacketManager {
private:
    TCPServer* tcpServer = nullptr;

    std::map<unsigned short, Client> clients;

    std::map<unsigned int, Match> activeMatches;
    std::map<unsigned short, unsigned int> clientToMatchId;

    //Reloj para el movimiento
    sf::Clock movementClock;

    //THREADS QUEUE
    std::queue<std::function<void()>> taskQueue;
    std::queue<std::function<void()>> urgentTaskQueue;

    //MUTEX
    std::mutex taskQueue_mutex;
    std::mutex clients_mutex;
    std::mutex udp_mutex;
    std::mutex console_mutex;
    std::mutex movement_mutex;
    std::mutex urgentTaskQueue_mutex;


    //MAPA
    std::vector<std::string> mapLines;
    bool mapLoaded = false;

    bool LoadMap();
    bool IsSolidTile(char tile) const;
    bool IsPositionInsideSolid(float x, float y);

    //Ping Pong
    const float PING_THRESHOLD = 1.0f;
    const float PING_INTERVAL = 0.5f;
    const float TIMEOUT = 2.0f;


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

    //Handlers de paquetes TCP y UDP
    void HandleTCPServerPacket(sf::Packet& packet);
    void HandleUDPClientPacket(const char* buffer, std::size_t receivedSize, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);

    //Movimiento
    void HandleUDPMovement(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);
    void SendValidatedMovement(sf::UdpSocket& udpSocket, const Client& client);
    void BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient);

	//Task Queue
    void Worker();
    void AddTask(std::function<void()> task);

	//Urgent Task Queue
    void UrgentWorker();
    void AddUrgentTask(std::function<void()> task);

    //Mapa
    void RequestMap();
    void HandleMapRequest(sf::Packet& packet);
    unsigned short LoadLocalMapVersion();
    void SaveLocalMap(const std::string& mapContent);
    void SaveLocalMapVersion(unsigned short version);

    //Ping
    void SendPing(sf::UdpSocket& udpSocket, Client& client);
    void HandlePing(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket);
    void HandlePong(const char* buffer, std::size_t receivedSize, std::size_t readPos);
    void UpdatePingSystem(sf::UdpSocket& udpSocket);
    void HandleClientTimeout(unsigned short clientId, sf::UdpSocket& udpSocket);

	//Validaciones de hacks
    void SendIrregularityWarning(sf::UdpSocket& udpSocket, const Client& client, unsigned int movementID);
    void FinishMatchByIrregularities(unsigned short loserId, sf::UdpSocket& udpSocket);
};
