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
#include <thread>
#include <condition_variable>

#define PM PacketManager::Instance()

#define NUM_MAX_THREADS std::thread::hardware_concurrency()

#define NORMAL_PACKET 0b00000000
#define URGENT_PACKET 0b00000001
#define CRITIC_PACKET 0b00000010

#define BUFFER_SIZE 1024


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
    TAUNT,
    PING,
    PONG,
    DISCONNECTED_PLAYER,
    IRREGULARITY_WARNING,
    PLAYER_HEALTH_UPDATE,
    MATCH_FINISHED,
    SHOOT_ACK,
    SHOOT_CONFIRMED
};

enum movementPacketType {
    SEND_RAW_MOVEMENT,
    RECEIVE_VALIDATED_MOVEMENT
};

enum matchFinishReason {
    FINISH_BY_LIVES,
    FINISH_BY_DISCONNECT,
    FINISH_BY_IRREGULARITY
};

enum matchResult {
    MATCH_RESULT_LOSE,
    MATCH_RESULT_WIN
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
    std::queue<std::function<void()>> urgentCriticTaskQueue;
    std::condition_variable taskQueue_cv;

    struct CriticalDelivery {
        unsigned short criticalPacketId;
        std::vector<char> packetData;
        unsigned short targetClientId;
        unsigned short shooterClientId;
        float lastSendTime;
        float firstSendTime;
    };

    std::map<unsigned short, CriticalDelivery> pendingCriticalDeliveries;
    unsigned short criticalPacketIdCounter = 0;

    //MUTEX
    std::mutex taskQueue_mutex;
    std::mutex clients_mutex;
    std::mutex udp_mutex;
    std::mutex console_mutex;
    std::mutex movement_mutex;
    std::mutex criticalDeliveries_mutex;

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

    //Movimiento
    const float playerMoveSpeed = 300.0f;
    const float maxVerticalSpeed = 2000.0f;
    const float tolerance = 2.0f;
    const float margin = 15.0f;
	unsigned const short MAX_IRREGULARITY_COUNT = 3;

    //Paquetes criticos
    const float resendInterval = 0.1f;
    const float giveUpAfter = 5.0f;


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

    //Health
    void HandlePlayerHealthUpdate(const char* buffer, std::size_t receivedSize, std::size_t readPos, sf::UdpSocket& udpSocket);
    void BroadcastHealthToOthers(sf::UdpSocket& udpSocket, const Client& damagedClient, short lives, short health);

	//Match Finished
    void SendMatchFinished(sf::UdpSocket& udpSocket, const Client& targetClient, matchResult result, matchFinishReason reason);

    //Disparo
    void HandleUDPShoot(const char* buffer, std::size_t receivedSize, std::size_t readPos, sf::UdpSocket& udpSocket);
    void HandleShootAck(const char* buffer, std::size_t receivedSize, std::size_t readPos, sf::UdpSocket& udpSocket);
    void SendShootConfirmed(sf::UdpSocket& udpSocket, unsigned short shooterClientId);
    void ResendCriticalPackets(sf::UdpSocket& udpSocket);

    //Golpe
    void HandleUDPHit(const char* buffer, std::size_t receivedSize, std::size_t readPos, sf::UdpSocket& udpSocket);

    //Burla
	void HandleUDPTaunt(const char* buffer, std::size_t receivedSize, std::size_t readPos, sf::UdpSocket& udpSocket);
    void BroadcastTauntToOthers(sf::UdpSocket& udpSocket, const Client& tauntingClient, unsigned int tauntId);

	//Task Queue
    void Worker();
    void AddTask(std::function<void()> task);
    void AddUrgentCriticTask(std::function<void()> task);

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
