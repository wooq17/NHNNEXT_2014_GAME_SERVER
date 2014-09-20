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

	for ( auto it : mLogedinSessionList )
	{
		xdelete( it.second );
	}
	mLogedinSessionList.clear();
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	//srand( (int)time(NULL) );
	for (int i = 0; i < mMaxConnection; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
		client->SessionReset();
		client->SetClientId( i );
			
		mFreeSessionList.push_back(client);
	}

	for ( auto it : mLogedinSessionList )
	{
		xdelete( it.second );
	}
	mLogedinSessionList.clear( );
	mSendRequestSessionList.clear();
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	if ( client->GetCurrentState() == LOGGED_IN || client->GetCurrentState() == WAIT_FOR_LOGOUT )
		DeregisterLogedinSession( client );
	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;

	// printf( "return session\n" );
}

bool SessionManager::ConnectSessions()
{
	FastSpinlockGuard guard(mLock);

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

	mMaxConnection = maxConnection;
}

void SessionManager::DoPeriodJob()
{
	for ( auto client : mLogedinSessionList )
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

	while ( !mSendRequestSessionList.empty() )
	{
		auto& session = mSendRequestSessionList.front();

		if ( session->FlushSend() )
		{
			/// true 리턴 되면 빼버린다.
			mSendRequestSessionList.pop_front();
		}
	}
}

void SessionManager::RegisterLogedinSession( ClientSession* client )
{
	FastSpinlockGuard guard( mLock );

	auto it = mLogedinSessionList.find( client->GetSocket( ) );

	printf( "****client register : %d\n", client->GetSocket( ) );
	CRASH_ASSERT( it == mLogedinSessionList.end( ) ); // 같은 게 이미 있으면 이상하다
	mLogedinSessionList.insert( xmap<SOCKET, ClientSession*>::type::value_type( client->GetSocket( ), client ) );
}

void SessionManager::DeregisterLogedinSession( ClientSession* client )
{
	auto it = mLogedinSessionList.find( client->GetSocket( ) );

	printf( "****client deregister : %d\n", client->GetSocket( ) );
	CRASH_ASSERT( it != mLogedinSessionList.end( ) );
	mLogedinSessionList.erase( it );
}

void SessionManager::RegisterSendRequest( ClientSession* client )
{
	FastSpinlockGuard guard( mLock );

	mSendRequestSessionList.push_back( client );
}