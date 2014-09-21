#pragma once
#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"

class ClientSession;
struct PacketHeader;

class ClientSessionManager
{
public:
	ClientSessionManager() : mCurrentIssueCount( 0 ), mCurrentReturnCount( 0 ), mTempId( 0 )
	{}
	
	~ClientSessionManager();

	void PrepareClientSessions();
	bool AcceptClientSessions();

	void ReturnClientSession(ClientSession* client);

	// void RegisterLogedinSession( ClientSession* client );
	// void DeregisterLogedinSession( ClientSession* client );
	void NearbyBroadcast( const char* pkt, size_t pktSize, int from );

	uint64_t GetTempId();

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;

	FastSpinlock mLock;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;

	uint64_t mTempId;

};

extern ClientSessionManager* GClientSessionManager;
