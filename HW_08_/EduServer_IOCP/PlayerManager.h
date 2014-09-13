#pragma once

#include "FastSpinlock.h"
#include "XTL.h"
#include <array>

class Player;
typedef xvector<std::shared_ptr<Player>>::type PlayerList;
typedef xmap<int, std::shared_ptr<Player>>::type PlayerMap;

class PlayerManager
{
public:
	PlayerManager();

	/// 플레이어를 등록하고 ID를 발급
	int RegisterPlayer(std::shared_ptr<Player> player, int playerId, int zoneIdx );

	/// ID에 해당하는 플레이어 제거
	void UnregisterPlayer( int playerId, int zoneIdx );

	int GetCurrentPlayers(PlayerList& outList);
	int GetClosePlayers( PlayerList& outList, int from );

	void ChangePlayerZone( int from, int to, int playerId );
	
private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	PlayerMap mPlayerMap;
	std::array<PlayerMap, ZONE_NUMBER> mPlayerZone;
};

extern PlayerManager* GPlayerManager;
