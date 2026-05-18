#include "PackageManager.h"

#include <iostream>
#include <cmath>
#include <cstring>

std::string PacketManager::MakeEndpointKey(const sf::IpAddress& ip, unsigned short port)
{
    return ip.toString() + ":" + std::to_string(port);
}

void PacketManager::SetTCPServer(TCPServer* server)
{
    tcpServer = server;
}

Client* PacketManager::GetClientByEndpoint(const sf::IpAddress& ip, unsigned short port)
{
    std::string key = MakeEndpointKey(ip, port);

    auto endpointIt = endpointToClientId.find(key);
    if (endpointIt == endpointToClientId.end())
        return nullptr;

    auto clientIt = clients.find(endpointIt->second);
    if (clientIt == clients.end())
        return nullptr;

    return &clientIt->second;
}

bool PacketManager::RegisterClientEndpoint(unsigned short clientId, const sf::IpAddress& ip, unsigned short port)
{
    std::string key = MakeEndpointKey(ip, port);

    if (endpointToClientId.find(key) != endpointToClientId.end())
        return false;

    auto clientIt = clients.find(clientId);

    if (clientIt != clients.end()) {
        Client& existingClient = clientIt->second;

        if (existingClient.HasAddresAndPort())
            return false;

        existingClient.SetAddress(ip);
        existingClient.SetPort(port);
        endpointToClientId[key] = clientId;
        return true;
    }

    if (clients.size() >= MAX_PLAYERS)
        return false;

    Client newClient;
    newClient.SetId(clientId);
    newClient.SetAddress(ip);
    newClient.SetPort(port);

    clients[clientId] = newClient;
    endpointToClientId[key] = clientId;

    return true;
}

void PacketManager::HandleTCPServerPacket(sf::Packet& packet)
{
    int rawType = 0;
    packet >> rawType;

    tcpServerPacketType type = static_cast<tcpServerPacketType>(rawType);

    switch (type) {
    case TCP_HANDSHAKE:
    {
        std::cout << "TCP_HANDSHAKE recibido del otro servidor" << std::endl;

        if (tcpServer != nullptr && tcpServer->IsConnected()) {
            sf::Packet response;
            response << static_cast<int>(TCP_HANDSHAKE);
            tcpServer->Send(response);
        }

        break;
    }

    case TCP_PLAYER_AUTHORIZED:
    {
        unsigned short clientId = 0;
        std::string username;

        packet >> clientId >> username;

        auto clientIt = clients.find(clientId);

        if (clientIt == clients.end()) {
            clients[clientId] = Client(clientId, username);
        }
        else {
            clientIt->second.SetUsername(username);
        }

        std::cout << "Cliente autorizado por TCP: id=" << clientId
            << " username=" << username << std::endl;

        break;
    }

    case TCP_MATCH_CREATED:
    {
        std::cout << "TCP_MATCH_CREATED recibido" << std::endl;
        break;
    }

    case TCP_MATCH_CLOSED:
    {
        std::cout << "TCP_MATCH_CLOSED recibido. Limpiando clientes UDP" << std::endl;

        clients.clear();
        endpointToClientId.clear();

        break;
    }

    case TCP_GAME_RESULT:
    {
        std::cout << "TCP_GAME_RESULT recibido" << std::endl;
        break;
    }

    default:
        std::cout << "Paquete TCP desconocido" << std::endl;
        break;
    }
}

void PacketManager::HandleUDPClientPacket(
    const char* buffer,
    std::size_t receivedSize,
    const sf::IpAddress& senderIP,
    unsigned short senderPort,
    sf::UdpSocket& udpSocket
) {
    std::size_t readPos = 0;

    if (readPos + sizeof(udpClientPacketType) > receivedSize)
        return;

    udpClientPacketType packetType;
    std::memcpy(&packetType, buffer + readPos, sizeof(packetType));
    readPos += sizeof(packetType);

    switch (packetType) {
    case UDP_MOVEMENT:
        HandleUDPMovement(buffer, receivedSize, readPos, senderIP, senderPort, udpSocket);
        break;

    case UDP_REGISTER_CLIENT:
        HandleUDPRegisterClient(buffer, receivedSize, readPos, senderIP, senderPort);
        break;

    case UDP_SHOOT:
        std::cout << "UDP_SHOOT recibido" << std::endl;
        break;

    case UDP_HIT:
        std::cout << "UDP_HIT recibido" << std::endl;
        break;

    case UDP_PING:
        std::cout << "UDP_PING recibido" << std::endl;
        break;

    default:
        std::cout << "Paquete UDP desconocido" << std::endl;
        break;
    }
}

void PacketManager::HandleUDPRegisterClient(
    const char* buffer,
    std::size_t receivedSize,
    std::size_t readPos,
    const sf::IpAddress& senderIP,
    unsigned short senderPort
) {
    if (readPos + sizeof(unsigned short) > receivedSize)
        return;

    unsigned short clientId = 0;
    std::memcpy(&clientId, buffer + readPos, sizeof(clientId));

    if (RegisterClientEndpoint(clientId, senderIP, senderPort)) {
        std::cout << "Cliente UDP registrado: id=" << clientId << std::endl;
    }
    else {
        std::cout << "Cliente UDP repetido o invalido: id=" << clientId << std::endl;
    }
}

void PacketManager::HandleUDPMovement(
    const char* buffer,
    std::size_t receivedSize,
    std::size_t readPos,
    const sf::IpAddress& senderIP,
    unsigned short senderPort,
    sf::UdpSocket& udpSocket
) {
    Client* client = GetClientByEndpoint(senderIP, senderPort);

    if (client == nullptr) {
        std::cout << "Movimiento UDP de cliente no registrado" << std::endl;
        return;
    }

    auto canRead = [&](std::size_t size) {
        return readPos + size <= receivedSize;
        };

    movementPacketType movementType;
    unsigned int movementID = 0;
    float receivedX = 0.0f;
    float receivedY = 0.0f;

    if (!canRead(sizeof(movementType))) return;
    std::memcpy(&movementType, buffer + readPos, sizeof(movementType));
    readPos += sizeof(movementType);

    if (movementType != SEND_RAW_MOVEMENT)
        return;

    if (!canRead(sizeof(movementID))) return;
    std::memcpy(&movementID, buffer + readPos, sizeof(movementID));
    readPos += sizeof(movementID);

    if (!canRead(sizeof(receivedX))) return;
    std::memcpy(&receivedX, buffer + readPos, sizeof(receivedX));
    readPos += sizeof(receivedX);

    if (!canRead(sizeof(receivedY))) return;
    std::memcpy(&receivedY, buffer + readPos, sizeof(receivedY));
    readPos += sizeof(receivedY);

    if (movementID <= client->GetLastProcessedMovementID())
        return;

    float dx = receivedX - client->GetX();
    float dy = receivedY - client->GetY();
    float distance = std::sqrt(dx * dx + dy * dy);

    const float maxAllowedDistance = 20.0f;

    if (distance <= maxAllowedDistance) {
        client->SetPosition(receivedX, receivedY);
    }

    client->SetLastProcessedMovementID(movementID);

    SendValidatedMovement(udpSocket, *client);
    BroadcastMovementToOthers(udpSocket, *client);
}

void PacketManager::SendValidatedMovement(sf::UdpSocket& udpSocket, const Client& client)
{
    if (!client.HasAddresAndPort())
        return;

    char buffer[1024];
    std::size_t size = 0;

    udpClientPacketType packetType = UDP_MOVEMENT;
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
}

void PacketManager::BroadcastMovementToOthers(sf::UdpSocket& udpSocket, const Client& movedClient)
{
    char buffer[1024];
    std::size_t size = 0;

    udpClientPacketType packetType = UDP_MOVEMENT;
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

    for (std::map<unsigned short, Client>::iterator it = clients.begin(); it != clients.end(); it++) {
        Client& target = it->second;

        if (target.GetId() == movedClient.GetId())
            continue;

        if (!target.HasAddresAndPort())
            continue;

        if(udpSocket.send(buffer, size, target.GetIpAddress().value(), target.GetPort()) != sf::Socket::Status::Done) {
			std::cerr << "Error al enviar movimiento validado a cliente id = " << target.GetId() << std::endl;
        }   
    }
}