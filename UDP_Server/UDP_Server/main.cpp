#include <SFML/Network.hpp>
#include <iostream>
#include "PackageManager.h"
#include "TCPServer.h"
#include <thread>

#define TCP_SERVER_IP sf::IpAddress(10, 8, 0, 3)
#define TCP_SERVER_PORT 55007
#define UDP_SERVER_PORT 55008

int main()
{
    TCPServer tcpServer(TCP_SERVER_IP, TCP_SERVER_PORT);
    sf::UdpSocket udpSocket;
    sf::SocketSelector selector;

    if (!tcpServer.Connect()) {
        std::cerr << "No se pudo conectar con el servidor TCP en puerto "
            << TCP_SERVER_PORT << std::endl;
        return -1;
    }

    std::cout << "Conectado al servidor TCP" << std::endl;

    sf::Packet handshakePacket;

    handshakePacket << static_cast<short>(SERVER_HANDSHAKE);

    if (tcpServer.Send(handshakePacket)) {
        std::cout << "TCP_SERVER_HANDSHAKE enviado al TCP Server" << std::endl;
    }
    else {
        std::cerr << "No se pudo enviar TCP_SERVER_HANDSHAKE al TCP Server" << std::endl;
    }

    tcpServer.GetSocket().setBlocking(false);
    selector.add(tcpServer.GetSocket());

    if (udpSocket.bind(UDP_SERVER_PORT) != sf::Socket::Status::Done) {
        std::cerr << "Error al bindear UDP en puerto " << UDP_SERVER_PORT << std::endl;
        return -1;
    }

    udpSocket.setBlocking(false);
    selector.add(udpSocket);

    PM->SetTCPServer(&tcpServer);
    PM->RequestMap();

    //THREADS
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_MAX_THREADS; i++)
    {
        threads.push_back(std::thread(&PacketManager::Worker, PM));
    }

    while (true) {
        PM->UpdatePingSystem(udpSocket);
        PM->ResendCriticalPackets(udpSocket);

        if (!selector.wait(sf::milliseconds(10)))
            continue;

        if (tcpServer.IsConnected() && selector.isReady(tcpServer.GetSocket())) {
            sf::Packet packet;
            sf::Socket::Status status = tcpServer.GetSocket().receive(packet);

            if (status == sf::Socket::Status::Done) {
                PM->HandleTCPServerPacket(packet);
            }
            else if (status == sf::Socket::Status::Disconnected) {
                selector.remove(tcpServer.GetSocket());
                tcpServer.Disconnect();

                std::cout << "Desconectado del servidor TCP" << std::endl;
            }
        }

        if (selector.isReady(udpSocket)) {
            char buffer[1024];
            std::size_t receivedSize = 0;
            std::optional<sf::IpAddress> senderIP;
            unsigned short senderPort = 0;

            while(udpSocket.receive(buffer, sizeof(buffer), receivedSize, senderIP, senderPort) == sf::Socket::Status::Done) {
                if (senderIP.has_value()) {

                    std::vector<char> packetData(buffer, buffer + receivedSize);
                    sf::IpAddress clientIP = senderIP.value();

                    udpPacketType peekedType = MOVEMENT;
                    if (packetData.size() >= sizeof(udpPacketType))
                        std::memcpy(&peekedType, packetData.data(), sizeof(udpPacketType));

                    if (peekedType == SHOOT || peekedType == SHOOT_ACK || peekedType == HIT)
                    {
                        PM->AddUrgentTask([packetData, clientIP, senderPort, &udpSocket]() {
                            PM->HandleUDPClientPacket(
                                packetData.data(),
                                packetData.size(),
                                clientIP,
                                senderPort,
                                udpSocket
                            );
                        });
                    }
                    else
                    {
                        PM->AddTask([packetData, clientIP, senderPort, &udpSocket]() {
                            PM->HandleUDPClientPacket(
                                packetData.data(),
                                packetData.size(),
                                clientIP,
                                senderPort,
                                udpSocket
                            );
                        });
                    }
                }
            }
        }
    }

    return 0;
}
