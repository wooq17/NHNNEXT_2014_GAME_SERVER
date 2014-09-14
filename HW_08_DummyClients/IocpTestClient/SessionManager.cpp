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

	for ( auto it : mClientList )
	{
		xdelete( it );
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	srand( (int)time(NULL) );
	for (int i = 0; i < mMaxConnection; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
		client->SetClientId( i );
			
		mFreeSessionList.push_back(client);
		mClientList.push_back( client );
	}
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
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
	for ( auto client : mClientList )
	{
		if ( !client->IsConnected() )
			continue;

		// player login		
		if ( !client->mPlayer->IsLoaded() )
		{
			wchar_t name[6];
			name[0] = static_cast<wchar_t>( rand() % 26 + 65 );
			name[1] = static_cast<wchar_t>( rand() % 26 + 65 );
			name[2] = static_cast<wchar_t>( rand() % 26 + 65 );
			name[3] = static_cast<wchar_t>( rand() % 26 + 65 );
			name[4] = static_cast<wchar_t>( rand() % 26 + 65 );
			name[5] = '\0';

			if ( client->mPlayer->SendLogin( name ) )
			{
				wprintf_s( L"[LOG] %s send login packet\n", name );				
				client->mPlayer->SetName( name );
			}
			continue;
		}
		
// 		if ( !client->mPlayer->IsAlive() )
// 		{
// 			client->mPlayer->SendLogout() ;
// 		}

		// 1 / 5ÀÇ È®·ü·Î send message
		if ( 0 == rand() % 5 )
		{
			wchar_t chatMessage[11];
			chatMessage[0] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[1] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[2] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[3] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[4] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[5] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[6] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[7] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[8] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[9] = static_cast<wchar_t>( rand() % 26 + 97 );
			chatMessage[10] = '\0';
			
			if ( client->mPlayer->SendChat( chatMessage ) )
			{
				wprintf_s( L"[LOG] %s send chat Message : %s\n", client->mPlayer->GetName(), chatMessage );
			}
			
		}

		// 1 / 3ÀÇ È®·ü·Î move
// 		if ( 0 == rand() % 3 )
// 		{
// 			Float3D pos{ 
// 				static_cast<float>( rand() % 2000 - 1000 ),
// 				static_cast<float>( rand() % 2000 - 1000 ),
// 				static_cast<float>( rand() % 2000 - 1000 )
// 			};
// 
// 			if ( client->mPlayer->SendMove( pos ) )
// 			{
// 				wprintf_s( L"[LOG] %s Send move to ( %f , %f , %f )\n", client->mPlayer->GetName(), pos.m_X, pos.m_Y, pos.m_Z );
// 			}
// 		}

	}

}
