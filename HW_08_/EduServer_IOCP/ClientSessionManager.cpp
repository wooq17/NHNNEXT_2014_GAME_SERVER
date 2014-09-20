﻿#include "stdafx.h"
#include "FastSpinlock.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "PlayerManager.h"
#include "IocpManager.h"
#include "MemoryPool.h"
#include "Packet.h"

ClientSessionManager* GClientSessionManager = nullptr;

ClientSessionManager::~ClientSessionManager()
{
	for (auto it : mFreeSessionList)
	{
		xdelete( it );
	}

	/*
	for ( auto it : mLogedinSessionList )
	{
		xdelete( it.second );
	}
	mLogedinSessionList.clear();
	*/
}

void ClientSessionManager::PrepareClientSessions()
{

	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
			
		mFreeSessionList.push_back(client);
	}

	// mLogedinSessionList.clear();
}





void ClientSessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	// 활성화 된 리스트에서 제거하고 초기화
	// 디비에서 로그아웃 처리 완료되면 거기서 바로 진행
	// DeregisterLogedinSession( client );

	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
}

bool ClientSessionManager::AcceptClientSessions()
{
	FastSpinlockGuard guard(mLock);

	while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
	{
		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();

		++mCurrentIssueCount;

		newClient->AddRef(); ///< refcount +1 for issuing 
		
		if (false == newClient->PostAccept())
			return false;
	}


	return true;
}
/*
void ClientSessionManager::RegisterLogedinSession( ClientSession* client )
{
	FastSpinlockGuard guard( mLock );

	auto it = mLogedinSessionList.find( client->GetSocket() );

	printf( "****client register : %d\n", client->GetSocket() );
	CRASH_ASSERT( it == mLogedinSessionList.end() ); // 같은 게 이미 있으면 이상하다
	mLogedinSessionList.insert( xmap<SOCKET, ClientSession*>::type::value_type( client->GetSocket(), client ) );
}

void ClientSessionManager::DeregisterLogedinSession( ClientSession* client )
{
	FastSpinlockGuard guard( mLock );

	auto it = mLogedinSessionList.find( client->GetSocket() );

	printf( "****client deregister : %d\n", client->GetSocket() );
	CRASH_ASSERT( it != mLogedinSessionList.end() );
	mLogedinSessionList.erase( it );
}
*/
void ClientSessionManager::NearbyBroadcast( PacketHeader* pkt, int from )
{
	PlayerList targetList;
	
	// 방송할 애들을 playerManager로부터 뽑아와서
	int targetNumber = GPlayerManager->GetClosePlayers( targetList, from );

	CRASH_ASSERT( targetNumber > 0 );

	// 순회하면서 방송
	for ( auto it : targetList )
	{
		it->GetSession()->PostSend( reinterpret_cast<const char*>( pkt ), pkt->mSize );
	}
}

uint64_t ClientSessionManager::GetTempId()
{
	return InterlockedIncrement( &mTempId );
}