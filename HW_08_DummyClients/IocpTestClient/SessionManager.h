#pragma once
//#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"
#include <array>

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentIssueCount( 0 ), mCurrentReturnCount( 0 ), mMaxConnection( 0 )
	{}
	
	~SessionManager();

	void Initialize(wchar_t* hostIP, unsigned short port, int maxConnection);

	void PrepareSessions();
	bool ConnectSessions();

	void ReturnClientSession(ClientSession* client);
		
	void SetMaxConnection( int val ) { mInputMaxConnection = val; }
	void DoPeriodJob();	
	void DoSendJob();
	sockaddr_in* GetServerAddr() { return &serverAddr; }

	void RegisterActiveSession( ClientSession* client );
	void DeregisterActiveSession( ClientSession* client );

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	typedef xmap<SOCKET, ClientSession*>::type ClientMap;
	ClientMap	mActiveSessionList;

	FastSpinlock mLock;

	sockaddr_in serverAddr;
	int mMaxConnection;
	int mInputMaxConnection;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;

	
};

extern SessionManager* GSessionManager;
