#pragma once
//#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0)
	{}
	
	~SessionManager();

	void Initialize(wchar_t* hostIP, unsigned short port, int maxConnection);

	void PrepareSessions();
	bool ConnectSessions();

	void ReturnClientSession(ClientSession* client);
		
	void SetMaxConnection( int val ) { mMaxConnection = val; }	
	sockaddr_in* GetServerAddr() { return &serverAddr; }

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	FastSpinlock mLock;

	sockaddr_in serverAddr;
	int mMaxConnection;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;

	
};

extern SessionManager* GSessionManager;
