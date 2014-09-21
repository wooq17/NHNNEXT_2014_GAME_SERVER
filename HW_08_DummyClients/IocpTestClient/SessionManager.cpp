#include "stdafx.h"
#include "FastSpinlock.h"
#include "MemoryPool.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"



SessionManager* GSessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList)
	{
		xdelete(it);
	}

	for ( auto it : mActiveSessionList )
	{
		xdelete( it.second );
	}
	mActiveSessionList.clear();
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	//srand( (int)time(NULL) );
	for ( int i = 0; i < mInputMaxConnection; ++i )
	{
		ClientSession* client = xnew<ClientSession>();
		client->SessionReset();
		client->SetClientId( i );
			
		mFreeSessionList.push_back(client);
	}

	for ( auto it : mActiveSessionList )
	{
		xdelete( it.second );
	}
	mActiveSessionList.clear();
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	DeregisterActiveSession( client );
	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;

	// printf( "return session\n" );
}

bool SessionManager::ConnectSessions()
{
	FastSpinlockGuard guard(mLock);

	if ( mMaxConnection < mInputMaxConnection )
		mMaxConnection += 1;

	while (mCurrentIssueCount - mCurrentReturnCount < mMaxConnection)
	{
		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();

		++mCurrentIssueCount;

		newClient->AddRef(); ///< refcount +1 for issuing 
		
		if (false == newClient->PostConnect())
			return false;
	}


	return true;
}

void SessionManager::Initialize( wchar_t* hostIP, unsigned short port, int maxConnection)
{
	char* pStr;	
	int strSize = WideCharToMultiByte( CP_ACP, 0, hostIP, -1, NULL, 0, NULL, NULL );
	pStr = new char[strSize];
	WideCharToMultiByte( CP_ACP, 0, hostIP, -1, pStr, strSize, 0, 0 );
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons( port );
	serverAddr.sin_addr.s_addr = inet_addr( pStr );

	mInputMaxConnection = maxConnection;
}

void SessionManager::DoPeriodJob()
{
	// 보내는 동안 건드리지 마라
	FastSpinlockGuard guard( mLock );

	for ( auto client : mActiveSessionList )
	{
		if ( client.second->GetCurrentState() != LOGGED_IN )
			continue;

		// 1 / 5의 확률로 move / chat
 		if ( 0 == rand() % 5 )
		{
			client.second->RequestMove();
			client.second->RequestChat();
 		}
	}
}


void SessionManager::DoSendJob()
{
	// 보내는 동안 건드리지 마라
	FastSpinlockGuard guard( mLock );

	for ( auto client : mActiveSessionList )
	{
		client.second->FlushSend();
	}
}

void SessionManager::RegisterActiveSession( ClientSession* client )
{
	FastSpinlockGuard guard( mLock );

	auto it = mActiveSessionList.find( client->GetSocket() );

	printf( "****client register : %d\n", client->GetSocket( ) );
	CRASH_ASSERT( it == mActiveSessionList.end() ); // 같은 게 이미 있으면 이상하다
	mActiveSessionList.insert( xmap<SOCKET, ClientSession*>::type::value_type( client->GetSocket(), client ) );
}

void SessionManager::DeregisterActiveSession( ClientSession* client )
{
	auto it = mActiveSessionList.find( client->GetSocket() );

	printf( "****client deregister : %d\n", client->GetSocket( ) );
	// CRASH_ASSERT( it != mActiveSessionList.end() );
	if ( it != mActiveSessionList.end() )
		mActiveSessionList.erase( it );
}