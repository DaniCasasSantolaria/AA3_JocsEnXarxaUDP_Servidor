#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <thread>
#include "PackageManager.h"

#define BIND_PORT 55000

void main()
{
	sf::UdpSocket socket;

	if (socket.bind(BIND_PORT) == sf::Socket::Status::Done)
	{
		std::cout << "Puerto bindeado correctamente " << BIND_PORT << std::endl;
	}
	else
	{
		std::cerr << "Error al bindear el puerto " << BIND_PORT << std::endl;
	}

	char buffer[1024];
	std::size_t receivedData;
	std::optional <sf::IpAddress> senderIP;
	unsigned short senderPort;

	//THREADS
	std::vector<std::thread> threads;

	for (int i = 0; i < NUM_MAX_THREADS; i++)
	{
		threads.push_back(std::thread(&PacketManager::Worker, PM));
	}

	while (true)
	{

		if (socket.receive(buffer, sizeof(buffer), receivedData, senderIP, senderPort) == sf::Socket::Status::Done)
		{
			//std::cout << "Mensaje recibido de " << senderIP.value() << ": " << senderPort << std::endl;
			std::size_t byToRead = 0;

			int messageSize = 0;
			std::memcpy(&messageSize, buffer, sizeof(messageSize));
			byToRead += sizeof(messageSize);

			std::string receivedString(buffer + byToRead, messageSize);
			byToRead += messageSize;

			int receivedData;
			std::memcpy(&receivedData, buffer + byToRead, sizeof(receivedData));
			byToRead += sizeof(receivedData);

			std::cout << "Datos recibidos: " << receivedString << ": " << receivedData << std::endl;
		}

		//if (socket.receive(buffer, sizeof(buffer), receivedData, senderIP, senderPort) == sf::Socket::Status::Done)
		//{
		//	std::string packetData(buffer, receivedData);

		//	PM->AddTask([packetData]() {
		//		std::size_t byToRead = 0;

		//		int messageSize = 0;
		//		std::memcpy(&messageSize, packetData.data(), sizeof(messageSize));
		//		byToRead += sizeof(messageSize);

		//		std::string receivedString(packetData.data() + byToRead, messageSize);
		//		byToRead += messageSize;

		//		int receivedNumber = 0;
		//		std::memcpy(&receivedNumber, packetData.data() + byToRead, sizeof(receivedNumber));

		//		std::cout << "Datos recibidos: " << receivedString << ": " << receivedNumber << std::endl;
		//		});
		//}
	}
}