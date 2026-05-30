#include "PackageManager.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>

sf::Packet& operator <<(sf::Packet& packet, packetType type) {
    return packet << static_cast<short>(type);
}

sf::Packet& operator >>(sf::Packet& packet, packetType& type) {
    short temp;
    packet >> temp;
    type = static_cast<packetType>(temp);
    return packet;
}

Client* PacketManager::GetClientByIP(const std::string& ip, unsigned short port)
{
    std::string key = MakeIPKey(ip, port);

    std::map<std::string, unsigned short>::iterator endpointIt = endpointToClientId.find(key);
    if (endpointIt == endpointToClientId.end())
        return nullptr;

    std::map<unsigned short, Client>::iterator clientIt = clients.find(endpointIt->second);
    if (clientIt == clients.end())
        return nullptr;

    return &clientIt->second;
}

bool PacketManager::RegisterClientIP(unsigned short clientId, const std::string& ip, unsigned short port)
{
    std::string key = MakeIPKey(ip, port);

    if (endpointToClientId.find(key) != endpointToClientId.end())
        return false;

    std::map<unsigned short, Client>::iterator clientIt = clients.find(clientId);

    // Hecho con IA para pasar el string a IP
    std::optional<sf::IpAddress> ipAdress = sf::IpAddress::resolve(ip);

    if (clientIt != clients.end()) {
        Client& existingClient = clientIt->second;

        if (existingClient.HasAddresAndPort())
            return false;

        existingClient.SetAddress(ipAdress);
        existingClient.SetPort(port);
        endpointToClientId[key] = clientId;
        return true;
    }

    if (clients.size() >= MAX_PLAYERS)
        return false;

    Client newClient;
    newClient.SetId(clientId);
    newClient.SetAddress(ipAdress);
    newClient.SetPort(port);

    clients[clientId] = newClient;
    endpointToClientId[key] = clientId;

    return true;
}

bool PacketManager::IsSolidTile(char tile) const {
    return tile == '#' || tile == 'P';
}

bool PacketManager::LoadMap() {
    if (mapLoaded) {
        return true;
    }

    std::ifstream file("resources/Maps/Map.txt");

    if (!file.is_open()) {
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {
        mapLines.push_back(line);
    }

    mapLoaded = true;
    return true;
}

bool PacketManager::IsPositionInsideSolid(float x, float y) {
    if (!LoadMap()) {
        return true;
    }

    const float tileSize = 48.0f;
    const float playerHalfSize = 48.0f;

    float playerLeft = x - playerHalfSize;
    float playerRight = x + playerHalfSize;
    float playerTop = y - playerHalfSize;
    float playerBottom = y + playerHalfSize;

    for (unsigned int row = 0; row < mapLines.size(); row++) {
        for (unsigned int col = 0; col < mapLines[row].size(); col++) {
            if (!IsSolidTile(mapLines[row][col])) {
                continue;
            }

            float tileLeft = col * tileSize;
            float tileRight = tileLeft + tileSize;
            float tileTop = row * tileSize;
            float tileBottom = tileTop + tileSize;

            bool overlap =
                playerRight > tileLeft &&
                playerLeft < tileRight &&
                playerBottom > tileTop &&
                playerTop < tileBottom;

            if (overlap) {
                return true;
            }
        }
    }

    return false;
}

void PacketManager::HandleTCPServerPacket(sf::Packet& packet)
{
    //short rawType = 0;
    //packet >> rawType;

    //tcpServerPacketType type = static_cast<tcpServerPacketType>(rawType);

    packetType type;
    packet >> type;

    switch (type) {
    case MATCH_CREATED:
    {
        unsigned short matchId = 0;
        short modeValue = 0;

        unsigned short p1Id = 0;
        std::string p1Username;
        std::string p1IP;
        unsigned short p1Port = 0;

        unsigned short p2Id = 0;
        std::string p2Username;
        std::string p2IP;
        unsigned short p2Port = 0;

        packet >> matchId >> modeValue
            >> p1Id >> p1Username
            >> p2Id >> p2Username;

        std::cout << "TCP_MATCH_CREATED recibido" << std::endl;
        std::cout << "MatchId: " << matchId << std::endl;
        std::cout << "Mode: " << modeValue << std::endl;
        std::cout << "P1: id=" << p1Id << " username=" << p1Username << " IP=" << p1IP << " Port=" << p1Port << std::endl;
        std::cout << "P2: id=" << p2Id << " username=" << p2Username << " IP=" << p2IP << " Port=" << p2Port << std::endl;

  /*      std::map<unsigned short, Client>::iterator p1It = clients.find(p1Id);

        if (p1It == clients.end()) {
            clients[p1Id] = Client(p1Id, p1Username);
        }
        else {
            p1It->second.SetUsername(p1Username);
        }

        std::map<unsigned short, Client>::iterator p2It = clients.find(p2Id);

        if (p2It == clients.end()) {
            clients[p2Id] = Client(p2Id, p2Username);
        }
        else {
            p2It->second.SetUsername(p2Username);
        }*/

        clients[p1Id] = Client(p1Id, p1Username);
        clients[p2Id] = Client(p2Id, p2Username);

        Match match(matchId, modeValue, p1Id, p2Id);
        activeMatches[matchId] = match;

        clientToMatchId[p1Id] = matchId;
        clientToMatchId[p2Id] = matchId;

        //if (RegisterClientIP(p1Id, p1IP, p1Port)) {
        //    std::cout << "Cliente UDP registrado: id=" << p1Id << std::endl;
        //}
        //else {
        //    std::cout << "Cliente UDP repetido o invalido: id=" << p1Id << std::endl;
        //}

        //if (RegisterClientIP(p2Id, p2IP, p2Port)) {
        //    std::cout << "Cliente UDP registrado: id=" << p2Id << std::endl;
        //}
        //else {
        //    std::cout << "Cliente UDP repetido o invalido: id=" << p1Id << std::endl;
        //}

        std::cout << "Match guardada en UDP Server. MatchId: " << matchId << " | P1: " << p1Id << " | P2: " << p2Id << std::endl;
        break;
    }
    case GAME_RESULT:
    {
        clients.clear();
        endpointToClientId.clear();
        std::cout << "TCP_GAME_RESULT recibido" << std::endl;
        break;
    }
    case MAP_REQUEST:
    {
        std::cout << "TCP_MAP_REQUEST recibido" << std::endl;
        HandleMapRequest(packet);
        break;
	}
    default:
        std::cout << "Paquete TCP desconocido" << std::endl;
        break;
    }
}

void PacketManager::HandleUDPClientPacket(const char* buffer, std::size_t receivedSize, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket) {
    std::size_t readPos = 0;

    if (readPos + sizeof(udpPacketType) > receivedSize)
        return;

    udpPacketType packetType;
    std::memcpy(&packetType, buffer + readPos, sizeof(packetType));
    readPos += sizeof(packetType);

    switch (packetType) {
    case MOVEMENT:
        HandleUDPMovement(buffer, receivedSize, readPos, senderIP, senderPort, udpSocket);
        break;

    case SHOOT:
        std::cout << "UDP_SHOOT recibido" << std::endl;
        break;

    case HIT:
        std::cout << "UDP_HIT recibido" << std::endl;
        break;

    case PING:
        std::cout << "UDP_PING recibido" << std::endl;
        break;

    default:
        std::cout << "Paquete UDP desconocido" << std::endl;
        break;
    }
}

void PacketManager::HandleUDPMovement(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket) {
    movement_mutex.lock();

    unsigned short clientId = 0;

    std::memcpy(&clientId, buffer + readPos, sizeof(clientId));
    readPos += sizeof(clientId);

    std::map<unsigned short, Client>::iterator clientIt = clients.find(clientId);

    if (clientIt == clients.end()) {
        std::cout << "Movimiento UDP de cliente no autorizado por TCP. id="
            << clientId << std::endl;
        movement_mutex.unlock();
        return;
    }

    Client* client = &clientIt->second;

    client->SetAddress(senderIP);
    client->SetPort(senderPort);

    if (client == nullptr) {
        std::cout << "Movimiento UDP de cliente no registrado" << std::endl;
        movement_mutex.unlock();
        return;
    }

    movementPacketType movementType;
    unsigned int movementID = 0;
    float receivedX = 0.0f;
    float receivedY = 0.0f;

    std::memcpy(&movementType, buffer + readPos, sizeof(movementType));
    readPos += sizeof(movementType);

    if (movementType != SEND_RAW_MOVEMENT) {
        movement_mutex.unlock();
        return;
    }

    std::memcpy(&movementID, buffer + readPos, sizeof(movementID));
    readPos += sizeof(movementID);

    std::memcpy(&receivedX, buffer + readPos, sizeof(receivedX));
    readPos += sizeof(receivedX);

    std::memcpy(&receivedY, buffer + readPos, sizeof(receivedY));
    readPos += sizeof(receivedY);

    if (client->HasProcessedMovement() &&
        movementID <= client->GetLastProcessedMovementID()) {
        movement_mutex.unlock();
        return;
    }

    float currentTime = movementClock.getElapsedTime().asSeconds();

    if (client->HasMovementTime()) {
        float deltaTime = currentTime - client->GetLastMovementTime();

        float dx = receivedX - client->GetX();
        float dy = receivedY - client->GetY();

        float absDx = std::abs(dx);
        float absDy = std::abs(dy);

        const float playerMoveSpeed = 300.0f;
        const float maxVerticalSpeed = 1200.0f;
        const float tolerance = 2.0f;
        const float margin = 20.0f;

        float maxAllowedX = playerMoveSpeed * deltaTime * tolerance + margin;
        float maxAllowedY = maxVerticalSpeed * deltaTime * tolerance + margin;

        if (absDx > maxAllowedX || absDy > maxAllowedY) {
            client->SetLastProcessedMovementID(movementID);
            SendValidatedMovement(udpSocket, *client);
            movement_mutex.unlock();
            return;
        }
    }

    if (IsPositionInsideSolid(receivedX, receivedY)) {
        client->SetLastProcessedMovementID(movementID);
        SendValidatedMovement(udpSocket, *client);
        movement_mutex.unlock();
        return;
    }

    client->SetPosition(receivedX, receivedY);
    client->SetLastProcessedMovementID(movementID);
    client->SetLastMovementTime(currentTime);

    SendValidatedMovement(udpSocket, *client);
    BroadcastMovementToOthers(udpSocket, *client);

    movement_mutex.unlock();
}

void PacketManager::SendValidatedMovement(sf::UdpSocket& udpSocket, const Client& client)
{
    if (!client.HasAddresAndPort())
        return;

    udp_mutex.lock();

    char buffer[1024];
    std::size_t size = 0;

    udpPacketType packetType = MOVEMENT;
    movementPacketType movementType = RECEIVE_VALIDATED_MOVEMENT;

    unsigned short clientId = client.GetId();
    unsigned int lastMovementID = client.GetLastProcessedMovementID();
    float x = client.GetX();
    float y = client.GetY();

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &movementType, sizeof(movementType));
    size += sizeof(movementType);

    std::memcpy(buffer + size, &clientId, sizeof(clientId));
    size += sizeof(clientId);

    std::memcpy(buffer + size, &lastMovementID, sizeof(lastMovementID));
    size += sizeof(lastMovementID);

    std::memcpy(buffer + size, &x, sizeof(x));
    size += sizeof(x);

    std::memcpy(buffer + size, &y, sizeof(y));
    size += sizeof(y);

    if (udpSocket.send(buffer, size, client.GetIpAddress().value(), client.GetPort()) != sf::Socket::Status::Done) {
        std::cerr << "Error al enviar movimiento validado a cliente id = " << client.GetId() << std::endl;
    }

    udp_mutex.unlock();

    //std::cout << "Movimiento validado enviado a cliente id = " << client.GetId() << std::endl;
}

void PacketManager::BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient)
{
    std::map<unsigned short, unsigned int>::iterator matchIdIt =
        clientToMatchId.find(movedClient.GetId());

    if (matchIdIt == clientToMatchId.end())
        return;

    std::map<unsigned int, Match>::iterator matchIt =
        activeMatches.find(matchIdIt->second);

    if (matchIt == activeMatches.end())
        return;

    const Match& match = matchIt->second;

    char buffer[1024];
    std::size_t size = 0;

    udpPacketType packetType = MOVEMENT;
    movementPacketType movementType = RECEIVE_VALIDATED_MOVEMENT;

    unsigned short clientId = movedClient.GetId();
    unsigned int lastMovementID = movedClient.GetLastProcessedMovementID();
    float x = movedClient.GetX();
    float y = movedClient.GetY();

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &movementType, sizeof(movementType));
    size += sizeof(movementType);

    std::memcpy(buffer + size, &clientId, sizeof(clientId));
    size += sizeof(clientId);

    std::memcpy(buffer + size, &lastMovementID, sizeof(lastMovementID));
    size += sizeof(lastMovementID);

    std::memcpy(buffer + size, &x, sizeof(x));
    size += sizeof(x);

    std::memcpy(buffer + size, &y, sizeof(y));
    size += sizeof(y);

    udp_mutex.lock();
    for (std::map<unsigned short, Client>::iterator it = clients.begin(); it != clients.end(); it++) {
        Client& target = it->second;

        if (target.GetId() == movedClient.GetId())
            continue;

        if (!match.HasPlayer(target.GetId()))
            continue;

        if (!target.HasAddresAndPort())
            continue;

        if (udpSocket.send(buffer, size, target.GetIpAddress().value(), target.GetPort()) != sf::Socket::Status::Done) {
            std::cerr << "Error al enviar movimiento validado a cliente id = " << target.GetId() << std::endl;
        }
    }
    udp_mutex.unlock();
}

void PacketManager::Worker()
{
    bool closeThread = false;

    while (!closeThread)
    {
        std::function<void()> task;

        taskQueue_mutex.lock();

        if (!taskQueue.empty())
        {
            task = taskQueue.front();
            taskQueue.pop();
        }

        taskQueue_mutex.unlock();

        if (task)
        {
            task();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void PacketManager::AddTask(std::function<void()> task)
{
    taskQueue_mutex.lock();
    taskQueue.push(task);
    taskQueue_mutex.unlock();
}

void PacketManager::RequestMap() {
    unsigned short localVersion = LoadLocalMapVersion();

    sf::Packet packet;
    packet << MAP_REQUEST << static_cast<short>(MAP_VERSION_CHECK) << localVersion;

    if (tcpServer->Send(packet) == false) {
        std::cerr << "Failed to request map" << std::endl;
    }
}

void PacketManager::HandleMapRequest(sf::Packet& packet) {
    short requestTypeValue;
    packet >> requestTypeValue;

    mapRequestType requestType = static_cast<mapRequestType>(requestTypeValue);

    if (requestType == MAP_UP_TO_DATE) {
        unsigned short serverVersion;
        packet >> serverVersion;

        std::cout << "UDP Server map already updated. Version: " << serverVersion << std::endl;
        return;
    }
    else if (requestType == MAP_UPDATE) {
        unsigned short serverVersion;
        std::string mapContent;

        packet >> serverVersion >> mapContent;

        SaveLocalMap(mapContent);
        SaveLocalMapVersion(serverVersion);

        mapLoaded = false;
        mapLines.clear();
        LoadMap();

        std::cout << "UDP Server map updated to version: " << serverVersion << std::endl;
    }
}

unsigned short PacketManager::LoadLocalMapVersion() {
    std::ifstream file("resources/Maps/map_version.txt");

    unsigned short version = 0;

    if (file.is_open()) {
        file >> version;
    }

    return version;
}

void PacketManager::SaveLocalMap(const std::string& mapContent) {
    std::ofstream file("resources/Maps/Map.txt");

    if (!file.is_open()) {
        std::cerr << "Map file could not be opened" << std::endl;
        return;
    }

    file << mapContent;
}

void PacketManager::SaveLocalMapVersion(unsigned short version) {
    std::ofstream file("resources/Maps/map_version.txt");

    if (!file.is_open()) {
        std::cerr << "Map version file could not be opened" << std::endl;
        return;
    }

    file << version;
}