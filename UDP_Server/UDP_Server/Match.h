#pragma once
#include <string>

class Match
{
private:
	unsigned short matchId = 0;
	short mode = 0;

	unsigned short player1Id = 0;
    std::string player1Username = "";
	unsigned short player2Id = 0;
	std::string player2Username = "";

public:
    Match() = default;

    Match( unsigned short newMatchId, short newMode, unsigned short newPlayer1Id, const std::string& newPlayer1Username, unsigned short newPlayer2Id, const std::string& newPlayer2Username)
        : matchId(newMatchId), mode(newMode), player1Id(newPlayer1Id), player1Username(newPlayer1Username), player2Id(newPlayer2Id), player2Username(newPlayer2Username) {}

    inline unsigned int GetMatchId() const { return matchId; }
    inline short GetMode() const { return mode; }

    inline unsigned short GetPlayer1Id() const { return player1Id; }
    inline unsigned short GetPlayer2Id() const { return player2Id; }

    inline const std::string& GetPlayer1Username() const { return player1Username; }
    inline const std::string& GetPlayer2Username() const { return player2Username; }

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

    inline std::string GetUsernameById(unsigned short clientId) const {
        if (clientId == player1Id) {
            return player1Username;
        }

        if (clientId == player2Id) {
            return player2Username;
        }

        return "";
    }
};

