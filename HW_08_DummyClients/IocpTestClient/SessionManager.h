#pragma once
//#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"
#include <array>

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
	void DoPeriodJob();	
	sockaddr_in* GetServerAddr() { return &serverAddr; }

	void RegisterLogedinSession( ClientSession* client );
	void DeregisterLogedinSession( ClientSession* client );

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	typedef xmap<SOCKET, ClientSession*>::type ClientMap;
	ClientMap	mLogedinSessionList;

	FastSpinlock mLock;

	sockaddr_in serverAddr;
	int mMaxConnection;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;

	
};

extern SessionManager* GSessionManager;
