#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"
#include "ClientSession.h"
#include "ContentsConfig.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(LO_ECONOMLY_CLASS), mCurrentIssueId(0)
{

}

int PlayerManager::RegisterPlayer( std::shared_ptr<Player> player, int playerId, int zoneIdx )
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap[playerId] = player;
	++mCurrentIssueId;

	mPlayerZone[zoneIdx][playerId] = player;

	return mCurrentIssueId;
}

void PlayerManager::UnregisterPlayer( int playerId, int zoneIdx )
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerZone[zoneIdx].erase( playerId );

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

int PlayerManager::GetClosePlayers( PlayerList& outList, int from )
{
	// 찾는 방식은
	
	// 존을 미리 만들어 두고 플레이어들이 이동할 때 마다 해당 존으로 새로 배치해서
	// 인근 존의 플레이어들만 뽑는 방법
	// n개의 캐릭터가 각자 한 번씩 이동하고 한 번씩 근처 캐릭터 검색하면
	// 이동할 때 logn, 검색할 때는 constant = nlogn
	// 존을 map으로 만들어서 따로 관리해야 되는데...귀찮...

	// 플레이어마다 자신의 존을 기록만 하고 있고
	// 전체 순회하면서 인근 존에 있는 애들만 뽑는 방법
	// 당장 찾을 때는 전체 순회가 필요하지만 캐릭터 이동할 때 별다른 자료 구조에 작업을 필요 없음 
	// n개의 캐릭터가 각자 한 번씩 이동하고 한 번씩 근처 캐릭터 검색하면
	// 이동할 때 constant, 검색할 때는 n = n^2

	// 게다가 캐릭터의 이동 속도에 비해 존이 충분히 크다면 
	// 이동에 의한 작업량보다 검색의 비용이 더 증가하므로
	// 존을 나누어서 관리하는 것이 더 유리
	FastSpinlockGuard share( mLock, false );

	int fromX = from % ZONE_NUMBER_EACH_AXIS;
	int fromY = from / ZONE_NUMBER_EACH_AXIS;

	int startX = max( 0, fromX - 1 );
	int startY = max( 0, fromY - 1 );

	int endX = min( ZONE_NUMBER_EACH_AXIS - 1, fromX + 1 );
	int endY = min( ZONE_NUMBER_EACH_AXIS - 1, fromY + 1 );

	for ( int j = startY; j <= endY; ++j )
	{
		for ( int i = startX; i <= endX; ++i )
		{
			int zoneIdx = j * ZONE_NUMBER_EACH_AXIS + i;

			for ( auto it : mPlayerZone[zoneIdx] )
				outList.push_back( it.second );
		}
	}

	return static_cast<int>( outList.size() );
}

void PlayerManager::ChangePlayerZone( int from, int to, int playerId )
{
	FastSpinlockGuard exclusive( mLock );

	mPlayerZone[to][playerId] = mPlayerZone[from][playerId];
	mPlayerZone[from].erase( playerId );
}