#include <SFML/Network.hpp>
#include <iostream>
#include "PackageManager.h"
#include "TCPServer.h"
#include <thread>

#define TCP_SERVER_IP sf::IpAddress(10, 40, 2, 189)
#define TCP_SERVER_PORT 55007
#define UDP_SERVER_PORT 55008

int main() {
    TCPServer tcpServer(TCP_SERVER_IP, TCP_SERVER_PORT);
    sf::UdpSocket udpSocket;
    sf::SocketSelector selector;

    bool closeServer = false;


    if (!tcpServer.Connect()) {
        std::cerr << "No se pudo conectar con el servidor TCP en puerto "
            << TCP_SERVER_PORT << std::endl;
        closeServer = false;
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
        closeServer = true;
    }

    udpSocket.setBlocking(false);
    selector.add(udpSocket);

    PM->SetTCPServer(&tcpServer);
    PM->RequestMap();

    //THREADS
    std::vector<std::thread> threads;

    for (unsigned short i = 0; i < NUM_MAX_THREADS; i++)
    {
        threads.push_back(std::thread(&PacketManager::Worker, PM));
    }

    while (!closeServer) {
        PM->UpdatePingSystem(udpSocket);
        PM->ResendCriticalPackets(udpSocket);

        if (selector.wait(sf::seconds(0.01f))) {
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
                    closeServer = true;
                }
            }

            if (selector.isReady(udpSocket)) {
                char buffer[BUFFER_SIZE];
                std::size_t receivedSize = 0;
                std::optional<sf::IpAddress> senderIP;
                unsigned short senderPort = 0;

                while (udpSocket.receive(buffer, sizeof(buffer), receivedSize, senderIP, senderPort) == sf::Socket::Status::Done) {
                    if (senderIP.has_value()) {

                        std::vector<char> packetData(buffer, buffer + receivedSize);    //Creamos una copia local de todos los bits del paquete para poder tratar directamente con
                        sf::IpAddress clientIP = senderIP.value();                      //sus datos en los threads y no preocuparnos de si llega otro paquete que sobreescriba los datos

                        uint8_t bitmask = NORMAL_PACKET;
                        std::memcpy(&bitmask, packetData.data(), sizeof(bitmask));

                        switch (bitmask) {
                        case NORMAL_PACKET:
                            PM->AddTask([packetData, clientIP, senderPort, &udpSocket]() {
                                PM->HandleUDPClientPacket(
                                    packetData.data(),
                                    packetData.size(),
                                    clientIP,
                                    senderPort,
                                    udpSocket
                                );
                                });
                            break;

                        case URGENT_PACKET:
                        case CRITIC_PACKET:
                        case URGENT_PACKET | CRITIC_PACKET:
                            PM->AddUrgentCriticTask([packetData, clientIP, senderPort, &udpSocket]() {
                                PM->HandleUDPClientPacket(
                                    packetData.data(),
                                    packetData.size(),
                                    clientIP,
                                    senderPort,
                                    udpSocket
                                );
                                });
                            break;

                        default:
                            std::cerr << "Paquete UDP con flags desconocidas: " << static_cast<int>(bitmask) << std::endl;
                            break;
                        }
                    }
                }
            }
        }
    }
    //Hecho con IA para liberar los threads de forma segura
    PM->StopWorkers();
    
    for (std::thread& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    return 0;
}
