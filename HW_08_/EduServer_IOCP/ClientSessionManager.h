#pragma once
#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"

class ClientSession;

class ClientSessionManager
{
public:
	ClientSessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0)
	{}
	
	~ClientSessionManager();

	void PrepareClientSessions();
	bool AcceptClientSessions();

	void ReturnClientSession(ClientSession* client);

	

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	FastSpinlock mLock;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;
};

extern ClientSessionManager* GClientSessionManager;
