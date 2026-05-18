#include "PackageManager.h"

// Procesa los paquetes recibidos del cliente
void PacketManager::HandlePacket(Client& client, sf::Packet& packet, DataBase& db, LobbyManager& lobbyManager, std::unordered_map<std::string, std::vector<std::vector<std::string>>>& gameResults) {
	int intType = 0;
	packet >> intType;

	packetType type = static_cast<packetType>(intType);

	bool sendResponse = true;
	sf::Packet response;

	switch (type) {
	case HANDSHAKE:
	{
		std::string message = "Handshake done with client";
		response << HANDSHAKE << message;
		break;
	}
	case LOGIN:
	{
		std::string userName;
		std::string password;

		packet >> userName >> password;

		int authResultInt = db.Login(userName, password);

		if (authResultInt == LOGIN_OK) {
			client.SetUsername(userName);
		}
		response << LOGIN << userName << password << authResultInt << db.GetScore(userName);
		std::cout << "Score: " << db.GetScore(userName) << std::endl;
		break;
	}
	case REGISTER:
	{
		std::string userName;
		std::string password;

		packet >> userName >> password;

		int authResultInt = db.CreateUser(userName, password);

		response << REGISTER << userName << password << authResultInt;
		break;
	}
	case RANKING:
	{
		std::string clientUsername;

		packet >> clientUsername;

		std::vector<PlayerScore> ranking = db.GetRanking(clientUsername);

		response << type;

		for (int i = 0; i < ranking.size(); i++) {
			response << ranking[i].name << ranking[i].score << ranking[i].position;
		}
		break;
	}
	case CREATE_LOBBY:
	{
		std::string idLobby;

		packet >> idLobby;

		int lobbyResultInt = lobbyManager.CreateLobby(idLobby, &client);

		response << CREATE_LOBBY << idLobby << lobbyResultInt;
		break;
	}
	case JOIN_LOBBY:
	{
		std::string idLobby;

		packet >> idLobby;

		lobbyResult lobbyResult;
		Lobby* lobby = lobbyManager.GetLobby(idLobby);

		if (lobby == nullptr) {
			lobbyResult = LOBBY_NOT_FOUND;
		}
		else if (lobby->IsFull()) {
			lobbyResult = LOBBY_FULL;
		}
		else {
			lobbyManager.AddClientToLobby(idLobby, &client);
			lobbyResult = LOBBY_JOINED_OK;

			lobby = lobbyManager.GetLobby(idLobby);
			// Si la sala está completa, avisa a todos los clientes
			if (lobby->IsFull()) {
				//Notificar a los clientes que empieza el juego
			}
		}

		response << JOIN_LOBBY << idLobby << lobbyResult;
		break;
	}
	case GAME_RESULT:
	{
		std::string lobbyId;
		int numPlayers;
		packet >> lobbyId >> numPlayers;

		std::vector<std::string> ranking;
		for (int i = 0; i < numPlayers; i++) {
			std::string username;
			packet >> username;
			ranking.push_back(username);
		}

		std::cout << "GAME_RESULT received: lobby=" << lobbyId << " numPlayers=" << numPlayers << std::endl;
		for (int i = 0; i < (int)ranking.size(); i++)
			std::cout << "  [" << i << "] " << ranking[i] << std::endl;

		gameResults[lobbyId].push_back(ranking);
		std::cout << "  submissions so far: " << gameResults[lobbyId].size() << "/" << numPlayers << std::endl;

		if ((int)gameResults[lobbyId].size() == numPlayers) {
			bool valid = true;
			for (int i = 1; i < numPlayers; i++) {
				if (gameResults[lobbyId][i] != gameResults[lobbyId][0]) {
					valid = false;
					std::cout << "  MISMATCH at submission " << i << std::endl;
					break;
				}
			}

			if (valid) {
				std::cout << "Game result validated for lobby " << lobbyId << std::endl;
				for (int i = 0; i < numPlayers; i++) {
					int points = 3 - i;
					std::cout << "  Awarding " << points << " pts to " << gameResults[lobbyId][0][i] << std::endl;
					db.UpdateScore(gameResults[lobbyId][0][i], points);
				}
			}
			else {
				std::cout << "Game result mismatch for lobby " << lobbyId << ", no points awarded" << std::endl;
			}

			gameResults.erase(lobbyId);
		}

		sendResponse = false;
		break;
	}
	default:
		std::cout << "Packet type does not exist" << std::endl;
		break;
	}

	if (sendResponse) {
		SendData(client.GetSocket(), response);
	}
}

void PacketManager::DisconnectClient(Client* client, LobbyManager& lobbyManager, sf::SocketSelector& selector) {
	lobbyManager.RemoveClientFromAllLobbies(client);
	selector.remove(client->GetSocket());
	client->GetSocket().disconnect();
}

void PacketManager::SendData(sf::TcpSocket& client, sf::Packet& packet) {
	if (client.send(packet) == sf::Socket::Status::Done) {
		std::cout << "Mensaje enviado" << std::endl;
		packet.clear();
	}
	else {
		std::cerr << "Error al enviar el mensaje al cliente" << std::endl;
	}
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
		else
		{
			closeThread = true;
		}

		taskQueue_mutex.unlock();

		if (task)
		{
			task();
		}
	}
}

void PacketManager::AddTask(std::function<void()> task)
{
	taskQueue_mutex.lock();
	taskQueue.push(task);
	taskQueue_mutex.unlock();
}