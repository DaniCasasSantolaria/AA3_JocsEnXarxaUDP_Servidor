#pragma once
class Match
{
private:
	unsigned short matchId = 0;
	short mode = 0;

	unsigned short player1Id = 0;
	unsigned short player2Id = 0;

public:
    Match() = default;

    Match(unsigned short newPlayer1Id, unsigned short newPlayer2Id)
        : player1Id(newPlayer1Id), player2Id(newPlayer2Id) {}

    inline unsigned int GetMatchId() const { return matchId; }
    inline short GetMode() const { return mode; }

    inline unsigned short GetPlayer1Id() const { return player1Id; }
    inline unsigned short GetPlayer2Id() const { return player2Id; }

    inline bool HasPlayer(unsigned short clientId) const {
        return clientId == player1Id || clientId == player2Id;
    }

    inline unsigned short GetOtherPlayerId(unsigned short clientId) const {
        if (clientId == player1Id) {
            return player2Id;
        }

        if (clientId == player2Id) {
            return player1Id;
        }

        return 0;
    }
};

