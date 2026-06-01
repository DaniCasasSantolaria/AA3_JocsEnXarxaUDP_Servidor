#pragma once
class Match
{
private:
	unsigned short player1Id = 0;
	unsigned short player2Id = 0;

public:
    Match() = default;

    Match(unsigned short newPlayer1Id, unsigned short newPlayer2Id)
        : player1Id(newPlayer1Id), player2Id(newPlayer2Id) {}

    inline bool HasPlayer(unsigned short clientId) const {
        return clientId == player1Id || clientId == player2Id;
    }
};

