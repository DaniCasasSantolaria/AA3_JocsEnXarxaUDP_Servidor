#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <mutex>
#include <functional>
#include <queue>

#define PM PacketManager::Instance()

#define LISTENER_PORT 55007
#define NUM_MAX_THREADS 4

enum packetType { HANDSHAKE, LOGIN, REGISTER, RANKING, CREATE_LOBBY, JOIN_LOBBY, GAME_RESULT };

enum udpPacketType { MOVEMENT };

class PacketManager {
private:
    const sf::IpAddress SERVER_IP = sf::IpAddress(192, 168, 50, 124);

    sf::TcpSocket socket;

    PacketManager() = default;
    PacketManager(PacketManager&) = delete;
    PacketManager& operator =(const PacketManager&) = delete;
    ~PacketManager() = default;

	std::queue<std::function<void()>> taskQueue;
    std::mutex taskQueue_mutex;
    std::mutex cosole_mutex;
    std::mutex movement_mutex;
    std::mutex bullet_mutex;
    std::mutex flex_mutex;


public:
    inline static PacketManager* Instance() {
        static PacketManager instance;
        return &instance;
    }

    void PairToPair() {};

    void HandlePacket(Client& client, sf::Packet& packet, DataBase& db, LobbyManager& lobbyManager, std::unordered_map<std::string, std::vector<std::vector<std::string>>>& gameResults);

    void DisconnectClient(Client* client, LobbyManager& lobbyManager, sf::SocketSelector& selector);

    void SendData(sf::TcpSocket& client, sf::Packet& packet);

    void Worker();
    void AddTask(std::function<void()> task);
};