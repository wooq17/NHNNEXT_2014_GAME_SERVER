#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"
#include "ClientSession.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(LO_ECONOMLY_CLASS), mCurrentIssueId(0)
{

}

int PlayerManager::RegisterPlayer(std::shared_ptr<Player> player)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap[++mCurrentIssueId] = player;

	return mCurrentIssueId;
}

void PlayerManager::UnregisterPlayer(int playerId)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap.erase(playerId);
}


int PlayerManager::GetCurrentPlayers(PlayerList& outList)
{
	// TODO: mLock을 read모드로 접근해서 outList에 현재 플레이어들의 정보를 담고 total에는 현재 플레이어의 총 수를 반환.
	FastSpinlockGuard share( mLock, false );

	for ( auto it : mPlayerMap )
		outList.push_back( it.second );

	int total = static_cast<int>( outList.size() );
	// WIP

	return total;
}

void PlayerManager::BroadcastChatting( const char* message, int len, int from )
{
	// clientSessionManager에게 방송 요청 
	// PlayerWideEvent로 구현? 그러면 메시지는 복사하고 사용?
	for ( auto it : mPlayerMap )
	{
		// 조심해!
		// 보낸 사람 표시할 것
		it.second->GetSession()->PostSend( message, len );
	}
}