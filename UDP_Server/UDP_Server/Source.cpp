#include <SFML/Network.hpp>
#include <iostream>
#include <string>

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
	}
}