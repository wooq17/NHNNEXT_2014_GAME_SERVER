#include "stdafx.h"
#include "FastSpinlock.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


SessionManager* GSessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList)
	{
		delete it;
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = new ClientSession();
			
		mFreeSessionList.push_back( client );

		// 조심해!
		// mFreeSessionList에 넣으면 AcceptSessions()에서 등록한다
		// 결국 나중에 연결 되었다가 반환되었을 때와 같은 상태로 만드는 과정이므로 
		// mCurrentReturnCount 수를 증가
		++mCurrentReturnCount;
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

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	// 조심해!
	// 현재 등록된 소켓 숫자가 최대보다 적으면 아직 등록안 되어 있으니 새로 등록
	// while ( mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION )
	while ( mCurrentIssueCount < MAX_CONNECTION )
	{
		//TODO mFreeSessionList에서 ClientSession* 꺼내서 PostAccept() 해주기.. (위의 ReturnClientSession와 뭔가 반대로 하면 될 듯?)
		ClientSession* newClient = mFreeSessionList.front();
		mFreeSessionList.pop_front();

		CRASH_ASSERT( newClient );

		// 반환 된 것에서 하나 꺼냈으니까 mCurrentReturnCount 감소
		--mCurrentReturnCount;

		// 실패시 false
		if (false == newClient->PostAccept())
			return false;

		// AddRef()도 당연히 해줘야 하고...
		// refCount가 0이 되면 해당 소켓을 리셋
		// IO context가 생겨나고 줄어들면서 이 숫자도 같이 변함
		// 접속을 끊을 때 이 숫자를 하나 감소
		// 그러면 접속 종료 요청하면서 1 줄여서 0이 되어야 되므로 처음에 1 증가
		newClient->AddRef();

		// 하나 새로 등록했으니까 mCurrentIssueCount 증가
		++mCurrentIssueCount;
		// WIP 
	}


	return true;
}