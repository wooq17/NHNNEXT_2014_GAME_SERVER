﻿#pragma once
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

	// void RegisterLogedinSession( ClientSession* client );
	// void DeregisterLogedinSession( ClientSession* client );
	void NearbyBroadcast( const char* message, int len, int from );

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	// typedef xmap<SOCKET, ClientSession*>::type ClientMap;
	// ClientMap	mLogedinSessionList;

	FastSpinlock mLock;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;
};

extern ClientSessionManager* GClientSessionManager;
