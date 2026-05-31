#include "PackageManager.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>
#include <fstream>

sf::Packet& operator <<(sf::Packet& packet, packetType type) {
    return packet << static_cast<short>(type);
}

sf::Packet& operator >>(sf::Packet& packet, packetType& type) {
    short temp;
    packet >> temp;
    type = static_cast<packetType>(temp);
    return packet;
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
    // Miramos si se ha cargado el mapa, y si no se ha cargado hacemos que todo sea un collider
    if (!LoadMap()) {
        return true;
    }

    const float tileSize = 48.0f;
    const float playerHalfSize = 48.0f;

    float playerLeft = x - playerHalfSize;
    float playerRight = x + playerHalfSize;
    float playerTop = y - playerHalfSize;
    float playerBottom = y + playerHalfSize;

    for (unsigned short row = 0; row < mapLines.size(); row++) {
        for (unsigned short col = 0; col < mapLines[row].size(); col++) {
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

void PacketManager::HandleTCPServerPacket(sf::Packet& packet) {

    packetType type;
    packet >> type;

    switch (type) {
    case MATCH_CREATED:
    {
        unsigned short matchId = 0;
        short modeValue = 0;

        unsigned short p1Id = 0;
        std::string p1Username;

        unsigned short p2Id = 0;
        std::string p2Username;

        packet >> matchId >> modeValue
            >> p1Id >> p1Username
            >> p2Id >> p2Username;

        console_mutex.lock();
        std::cout << "TCP_MATCH_CREATED recibido" << std::endl;
        std::cout << "MatchId: " << matchId << std::endl;
        std::cout << "Mode: " << modeValue << std::endl;
        std::cout << "P1: id=" << p1Id << " username=" << p1Username << std::endl;
        std::cout << "P2: id=" << p2Id << " username=" << p2Username << std::endl;
        console_mutex.unlock();

        Match match(p1Id, p2Id);

        clients_mutex.lock();
        clients[p1Id] = Client(p1Id);
        clients[p2Id] = Client(p2Id);
        activeMatches[matchId] = match;

        clientToMatchId[p1Id] = matchId;
        clientToMatchId[p2Id] = matchId;
        clients_mutex.unlock();

        console_mutex.lock();
        std::cout << "Match guardada en UDP Server. MatchId: " << matchId << " | P1: " << p1Id << " | P2: " << p2Id << std::endl;
        console_mutex.unlock();
        break;
    }
    case GAME_RESULT:
    {
        clients_mutex.lock();
        clients.clear();
        clientToMatchId.clear();
        activeMatches.clear();
        clients_mutex.unlock();

        console_mutex.lock();
        std::cout << "TCP_GAME_RESULT recibido" << std::endl;
        console_mutex.unlock();
        break;
    }
    case MAP_REQUEST:
    {
        console_mutex.lock();
        std::cout << "TCP_MAP_REQUEST recibido" << std::endl;
        console_mutex.unlock();
        HandleMapRequest(packet);
        break;
	}
    default:
        console_mutex.lock();
        std::cout << "Paquete TCP desconocido" << std::endl;
        console_mutex.unlock();
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
        console_mutex.lock();
        std::cout << "UDP_SHOOT recibido" << std::endl;
        console_mutex.unlock();
        break;

    case HIT:
        console_mutex.lock();
        std::cout << "UDP_HIT recibido" << std::endl;
        console_mutex.unlock();
        break;

    case PING:
        // Si el cliente hace ping, se responde con un pong
        HandlePing(buffer, receivedSize, readPos, senderIP, senderPort, udpSocket);
        break;

    case PONG:
		// Si el cliente responde a un ping, se actualiza su estado
        HandlePong(buffer, receivedSize, readPos);
        break;

    default:
        console_mutex.lock();
        std::cout << "Paquete UDP desconocido" << std::endl;
        console_mutex.unlock();
        break;
    }
}

void PacketManager::HandleUDPMovement(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket) {
    movement_mutex.lock();

    unsigned short clientId = 0;

    std::memcpy(&clientId, buffer + readPos, sizeof(clientId));
    readPos += sizeof(clientId);

	clients_mutex.lock();
    std::map<unsigned short, Client>::iterator clientIt = clients.find(clientId);

    if (clientIt == clients.end()) {
        console_mutex.lock();
        std::cout << "Movimiento UDP de cliente no autorizado por TCP. id="
            << clientId << std::endl;
        console_mutex.unlock();
        movement_mutex.unlock();
        clients_mutex.unlock();
        return;
    }

    Client* client = &clientIt->second;

    client->SetAddress(senderIP);
    client->SetPort(senderPort);

    float currentTime = movementClock.getElapsedTime().asSeconds();
    client->SetLastPacketTime(currentTime);
    client->SetWaitingPong(false);

    if (client == nullptr) {
        console_mutex.lock();
        std::cout << "Movimiento UDP de cliente no registrado" << std::endl;
        console_mutex.unlock();
        movement_mutex.unlock();
        clients_mutex.unlock();
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
        clients_mutex.unlock();
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
        clients_mutex.unlock();
        return;
    }

    if (client->HasMovementTime()) {
        float deltaTime = currentTime - client->GetLastMovementTime();

        if (deltaTime < 0.025f) {
            deltaTime = 0.025f;
        }

        float dx = receivedX - client->GetX();
        float dy = receivedY - client->GetY();

        float absDx = std::abs(dx);
        float absDy = std::abs(dy);

        const float playerMoveSpeed = 300.0f;
        const float maxVerticalSpeed = 2000.0f;
        const float tolerance = 2.0f;
        const float margin = 15.0f;

        float maxAllowedX = playerMoveSpeed * deltaTime * tolerance + margin;
        float maxAllowedY = maxVerticalSpeed * deltaTime * tolerance + margin;

        if (absDx > maxAllowedX || absDy > maxAllowedY) {
            client->SetLastProcessedMovementID(movementID);
            client->AddIrregularity();

            Client correctedClient = *client;
            bool shouldFinishMatch = client->GetIrregularityCount() >= 3;

            clients_mutex.unlock();

            SendIrregularityWarning(udpSocket, correctedClient, movementID);
            SendValidatedMovement(udpSocket, correctedClient);

            movement_mutex.unlock();

            if (shouldFinishMatch) {
                FinishMatchByIrregularities(correctedClient.GetId(), udpSocket);
            }

            return;
        }
    }

    if (IsPositionInsideSolid(receivedX, receivedY)) {
        client->SetLastProcessedMovementID(movementID);

        Client correctedClient = *client;

        clients_mutex.unlock();

        SendValidatedMovement(udpSocket, correctedClient);

        movement_mutex.unlock();

        client->SetLastMovementTime(currentTime);
        return;
    }

    client->SetPosition(receivedX, receivedY);
    client->SetLastProcessedMovementID(movementID);
    client->SetLastMovementTime(currentTime);

    Client movedClient = *client;

    clients_mutex.unlock();

    SendValidatedMovement(udpSocket, movedClient);
    BroadcastMovementToOthers(udpSocket, movedClient);


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
        console_mutex.lock();
        std::cerr << "Error al enviar movimiento validado a cliente id = " << client.GetId() << std::endl;
        console_mutex.unlock();
    }

    udp_mutex.unlock();
}

void PacketManager::BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient)
{
    clients_mutex.lock();

    std::map<unsigned short, unsigned int>::iterator matchIdIt =
        clientToMatchId.find(movedClient.GetId());

    if (matchIdIt == clientToMatchId.end()) {
        clients_mutex.unlock();
        return;
    }

    std::map<unsigned int, Match>::iterator matchIt =
        activeMatches.find(matchIdIt->second);

    if (matchIt == activeMatches.end()) {
        clients_mutex.unlock();
        return;
    }

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

        if (target.GetId() == movedClient.GetId() || !match.HasPlayer(target.GetId()) || !target.HasAddresAndPort())
            continue;

        if (udpSocket.send(buffer, size, target.GetIpAddress().value(), target.GetPort()) != sf::Socket::Status::Done) {
            console_mutex.lock();
            std::cerr << "Error al enviar movimiento validado a cliente id = " << target.GetId() << std::endl;
            console_mutex.unlock();
        }
    }
    udp_mutex.unlock();

    clients_mutex.unlock();
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
        console_mutex.lock();
        std::cerr << "Failed to request map" << std::endl;
        console_mutex.unlock();
    }
}

void PacketManager::HandleMapRequest(sf::Packet& packet) {
    short requestTypeValue;
    packet >> requestTypeValue;

    mapRequestType requestType = static_cast<mapRequestType>(requestTypeValue);

    if (requestType == MAP_UP_TO_DATE) {
        unsigned short serverVersion;
        packet >> serverVersion;

        console_mutex.lock();
        std::cout << "UDP Server map already updated. Version: " << serverVersion << std::endl;
        console_mutex.unlock();
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

        console_mutex.lock();
        std::cout << "UDP Server map updated to version: " << serverVersion << std::endl;
        console_mutex.unlock();
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
        console_mutex.lock();
        std::cerr << "Map file could not be opened" << std::endl;
        console_mutex.unlock();
        return;
    }

    file << mapContent;
}

void PacketManager::SaveLocalMapVersion(unsigned short version) {
    std::ofstream file("resources/Maps/map_version.txt");

    if (!file.is_open()) {
        console_mutex.lock();
        std::cerr << "Map version file could not be opened" << std::endl;
        console_mutex.unlock();
        return;
    }

    file << version;
}

void PacketManager::SendPing(sf::UdpSocket& udpSocket, Client& client) {
    if (!client.HasAddresAndPort() || client.IsDisconnected())
        return;

    udpPacketType packetType = PING;
    unsigned short clientId = client.GetId();
    unsigned int pingId = client.GetLastPingId() + 1;

    char buffer[1024];
    std::size_t size = 0;

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &clientId, sizeof(clientId));
    size += sizeof(clientId);

    std::memcpy(buffer + size, &pingId, sizeof(pingId));
    size += sizeof(pingId);

    udp_mutex.lock();

    if (udpSocket.send(buffer, size, client.GetIpAddress().value(), client.GetPort()) == sf::Socket::Status::Done) {
        float currentTime = movementClock.getElapsedTime().asSeconds();

        client.SetLastPingId(pingId);
        client.SetLastPingTime(currentTime);
        client.SetWaitingPong(true);

        console_mutex.lock();
        std::cout << "PING enviado a cliente id=" << clientId << " pingId=" << pingId << std::endl;
        console_mutex.unlock();
    }
    else {
        console_mutex.lock();
        std::cerr << "Error enviando PING a cliente id=" << clientId << std::endl;
        console_mutex.unlock();
    }

    udp_mutex.unlock();
}

void PacketManager::HandlePing(const char* buffer, std::size_t receivedSize, std::size_t readPos, const sf::IpAddress& senderIP, unsigned short senderPort, sf::UdpSocket& udpSocket) {
    if (readPos + sizeof(unsigned short) + sizeof(unsigned int) > receivedSize)
        return;

    unsigned short clientId = 0;
    unsigned int pingId = 0;

    std::memcpy(&clientId, buffer + readPos, sizeof(clientId));
    readPos += sizeof(clientId);

    std::memcpy(&pingId, buffer + readPos, sizeof(pingId));
    readPos += sizeof(pingId);

    clients_mutex.lock();
    std::map<unsigned short, Client>::iterator clientIt = clients.find(clientId);

    if (clientIt == clients.end()) {
		clients_mutex.unlock();
        return;
    }

    Client& client = clientIt->second;

    float currentTime = movementClock.getElapsedTime().asSeconds();
    client.SetAddress(senderIP);
    client.SetPort(senderPort);
    client.SetLastPacketTime(currentTime);
    client.SetWaitingPong(false);

    clients_mutex.unlock();

    udpPacketType responseType = PONG;

    char responseBuffer[1024];
    std::size_t responseSize = 0;

    std::memcpy(responseBuffer + responseSize, &responseType, sizeof(responseType));
    responseSize += sizeof(responseType);

    std::memcpy(responseBuffer + responseSize, &clientId, sizeof(clientId));
    responseSize += sizeof(clientId);

    std::memcpy(responseBuffer + responseSize, &pingId, sizeof(pingId));
    responseSize += sizeof(pingId);

    udp_mutex.lock();
    if (udpSocket.send(responseBuffer, responseSize, senderIP, senderPort) != sf::Socket::Status::Done) {
		console_mutex.lock();
        std::cerr << "Error enviando PONG a cliente id=" << clientId << std::endl;
        console_mutex.unlock();
    }
    udp_mutex.unlock();

    console_mutex.lock();
    std::cout << "PING recibido. PONG enviado a cliente id=" << clientId << std::endl;
    console_mutex.unlock();
}

void PacketManager::HandlePong(const char* buffer, std::size_t receivedSize, std::size_t readPos) {
    if (readPos + sizeof(unsigned short) + sizeof(unsigned int) > receivedSize)
        return;

    unsigned short clientId = 0;
    unsigned int pingId = 0;

    std::memcpy(&clientId, buffer + readPos, sizeof(clientId));
    readPos += sizeof(clientId);

    std::memcpy(&pingId, buffer + readPos, sizeof(pingId));
    readPos += sizeof(pingId);

    clients_mutex.lock();

    std::map<unsigned short, Client>::iterator clientIt = clients.find(clientId);

    if (clientIt == clients.end()) {
        clients_mutex.unlock();
        return;
    }

    Client& client = clientIt->second;

    if (pingId != client.GetLastPingId()) {
        clients_mutex.unlock();
        return;
    }

    float currentTime = movementClock.getElapsedTime().asSeconds();

    client.SetLastPacketTime(currentTime);
    client.SetWaitingPong(false);

    clients_mutex.unlock();

    console_mutex.lock();
    std::cout << "PONG recibido de cliente id=" << clientId << " pingId=" << pingId << std::endl;
    console_mutex.unlock();
}



void PacketManager::UpdatePingSystem(sf::UdpSocket& udpSocket) {
    float currentTime = movementClock.getElapsedTime().asSeconds();

    std::vector<unsigned short> timedOutClients;

    clients_mutex.lock();

    for (std::map<unsigned short, Client>::iterator it = clients.begin(); it != clients.end(); it++) {
        Client& client = it->second;

        if (!client.HasAddresAndPort() || client.IsDisconnected())
            continue;

        float timeSinceLastPacket = currentTime - client.GetLastPacketTime();

        if (timeSinceLastPacket >= TIMEOUT) {
            client.SetDisconnected(true);
            timedOutClients.push_back(client.GetId());
            continue;
        }

        if (timeSinceLastPacket >= PING_THRESHOLD) {
            bool shouldSendPing =
                !client.IsWaitingPong() ||
                currentTime - client.GetLastPingTime() >= PING_INTERVAL;

            if (shouldSendPing) {
                SendPing(udpSocket, client);
            }
        }
    }

    clients_mutex.unlock();

    for (unsigned int i = 0; i < timedOutClients.size(); i++) {
        HandleClientTimeout(timedOutClients[i], udpSocket);
    }
}

void PacketManager::HandleClientTimeout(unsigned short clientId, sf::UdpSocket& udpSocket) {
    clients_mutex.lock();

    std::map<unsigned short, Client>::iterator disconnectedIt = clients.find(clientId);

    if (disconnectedIt == clients.end()) {
        clients_mutex.unlock();
        return;
    }

    console_mutex.lock();
    std::cout << "Cliente desconectado por timeout UDP. id=" << clientId << std::endl;
    console_mutex.unlock();

    std::map<unsigned short, unsigned int>::iterator matchIdIt = clientToMatchId.find(clientId);

    if (matchIdIt == clientToMatchId.end()) {
        clients.erase(clientId);
        clients_mutex.unlock();
        return;
    }

    unsigned int matchId = matchIdIt->second;

    std::map<unsigned int, Match>::iterator matchIt = activeMatches.find(matchId);

    if (matchIt == activeMatches.end()) {
        clientToMatchId.erase(clientId);
        clients.erase(clientId);
        clients_mutex.unlock();
        return;
    }

    Match& match = matchIt->second;

    char buffer[1024];
    std::size_t size = 0;

    udpPacketType packetType = DISCONNECTED_PLAYER;
    unsigned short disconnectedClientId = clientId;

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &disconnectedClientId, sizeof(disconnectedClientId));
    size += sizeof(disconnectedClientId);

    std::vector<Client> targets;

    for (std::map<unsigned short, Client>::iterator it = clients.begin(); it != clients.end(); it++) {
        Client& target = it->second;

        if (target.GetId() == clientId || !match.HasPlayer(target.GetId()) || !target.HasAddresAndPort() || target.IsDisconnected())
            continue;

        targets.push_back(target);
    }

    for (std::map<unsigned short, unsigned int>::iterator it = clientToMatchId.begin(); it != clientToMatchId.end();) {
        if (it->second == matchId) {
            it = clientToMatchId.erase(it);
        }
        else {
            it++;
        }
    }

    activeMatches.erase(matchId);
    clients.erase(clientId);

    clients_mutex.unlock();

    udp_mutex.lock();

    for (unsigned int i = 0; i < targets.size(); i++) {
        Client& target = targets[i];

        if (udpSocket.send(buffer, size, target.GetIpAddress().value(), target.GetPort()) != sf::Socket::Status::Done) {
            console_mutex.lock();
            std::cerr << "Error enviando DISCONNECTED_PLAYER a cliente id=" << target.GetId() << std::endl;
            console_mutex.unlock();
        }
    }

    udp_mutex.unlock();

    if (tcpServer != nullptr) {
        sf::Packet packet;
        packet << GAME_RESULT << clientId;
        tcpServer->Send(packet);
    }
}


void PacketManager::SendIrregularityWarning(sf::UdpSocket& udpSocket, const Client& client, unsigned int movementID) {
    if (!client.HasAddresAndPort())
        return;

    char buffer[1024];
    std::size_t size = 0;

    udpPacketType packetType = IRREGULARITY_WARNING;

    unsigned short clientId = client.GetId();
    unsigned short irregularityCount = client.GetIrregularityCount();
    float validX = client.GetX();
    float validY = client.GetY();

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &clientId, sizeof(clientId));
    size += sizeof(clientId);

    std::memcpy(buffer + size, &movementID, sizeof(movementID));
    size += sizeof(movementID);

    std::memcpy(buffer + size, &validX, sizeof(validX));
    size += sizeof(validX);

    std::memcpy(buffer + size, &validY, sizeof(validY));
    size += sizeof(validY);

    std::memcpy(buffer + size, &irregularityCount, sizeof(irregularityCount));
    size += sizeof(irregularityCount);

    udp_mutex.lock();

    if (udpSocket.send(buffer, size, client.GetIpAddress().value(), client.GetPort()) != sf::Socket::Status::Done) {
        console_mutex.lock();
        std::cerr << "Error enviando IRREGULARITY_WARNING a cliente id=" << clientId << std::endl;
        console_mutex.unlock();
    }

    udp_mutex.unlock();
}

void PacketManager::FinishMatchByIrregularities(unsigned short loserId, sf::UdpSocket& udpSocket) {
    clients_mutex.lock();

    std::map<unsigned short, unsigned int>::iterator matchIdIt = clientToMatchId.find(loserId);

    if (matchIdIt == clientToMatchId.end()) {
        clients_mutex.unlock();
        return;
    }

    unsigned int matchId = matchIdIt->second;

    std::map<unsigned int, Match>::iterator matchIt = activeMatches.find(matchId);

    if (matchIt == activeMatches.end()) {
        clients_mutex.unlock();
        return;
    }

    Match match = matchIt->second;

    char buffer[1024];
    std::size_t size = 0;

    udpPacketType packetType = MATCH_FINISHED;
    unsigned short reason = 1;
    unsigned short loserClientId = loserId;

    std::memcpy(buffer + size, &packetType, sizeof(packetType));
    size += sizeof(packetType);

    std::memcpy(buffer + size, &loserClientId, sizeof(loserClientId));
    size += sizeof(loserClientId);

    std::memcpy(buffer + size, &reason, sizeof(reason));
    size += sizeof(reason);

    std::vector<Client> targets;
    std::vector<unsigned short> clientsToErase;

    for (std::map<unsigned short, Client>::iterator it = clients.begin(); it != clients.end(); it++) {
        Client& target = it->second;

        if (!match.HasPlayer(target.GetId()))
            continue;

        clientsToErase.push_back(target.GetId());

        if (target.HasAddresAndPort() && !target.IsDisconnected()) {
            targets.push_back(target);
        }
    }

    for (std::map<unsigned short, unsigned int>::iterator it = clientToMatchId.begin(); it != clientToMatchId.end();) {
        if (it->second == matchId) {
            it = clientToMatchId.erase(it);
        }
        else {
            it++;
        }
    }

    activeMatches.erase(matchId);

    for (unsigned int i = 0; i < clientsToErase.size(); i++) {
        clients.erase(clientsToErase[i]);
    }

    clients_mutex.unlock();

    udp_mutex.lock();

    for (unsigned int i = 0; i < targets.size(); i++) {
        Client& target = targets[i];

        if (udpSocket.send(buffer, size, target.GetIpAddress().value(), target.GetPort()) != sf::Socket::Status::Done) {
            console_mutex.lock();
            std::cerr << "Error enviando MATCH_FINISHED a cliente id=" << target.GetId() << std::endl;
            console_mutex.unlock();
        }
    }

    udp_mutex.unlock();

    console_mutex.lock();
    std::cout << "Partida finalizada por irregularidades. Perdedor id=" << loserId << std::endl;
    console_mutex.unlock();

    if (tcpServer != nullptr) {
        sf::Packet packet;
        packet << GAME_RESULT << loserId;
        tcpServer->Send(packet);
    }
}