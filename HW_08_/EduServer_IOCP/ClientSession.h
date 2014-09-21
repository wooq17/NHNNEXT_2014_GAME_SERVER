#pragma once
#include "ObjectPool.h"
#include "MemoryPool.h"
#include "CircularBuffer.h"
#include "SyncExecutable.h"
#include "Session.h"
#include "Player.h"

class ClientSessionManager;
class PacketHeader;

enum ClientState
{
	NOTHING = -1,
	SHARING_KEY,
	WAIT_FOR_LOGIN,
	LOGGED_IN,
	WAIT_FOR_LOGOUT,
};

class ClientSession : public Session, public PooledAllocatable
{
public:
	ClientSession();
	~ClientSession() {}

	void	SessionReset();

	bool	PostAccept();
	void	AcceptCompletion();

	void	SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket;  }

	bool	PacketHandling();
	bool	IsValidData( PacketHeader* start, ULONG len );

	virtual void OnDisconnect( DisconnectReason dr );
	virtual void OnRelease();

	bool SendBaseKey();
	void ResponseExportedKey( PacketHeader* recvPacket );
	void ResponseLogin( PacketHeader* recvPacket );
	void ResponseLogout( PacketHeader* recvPacket );
	void ResponseChat( PacketHeader* recvPacket );
	void ResponseMove( PacketHeader* recvPacket );

	ClientState GetState() { return mState; }

public:
	std::shared_ptr<Player> mPlayer;

private:
	
	// SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	// FastSpinlock	mBufferLock;

	// CircularBuffer	mBuffer;

	// volatile long	mRefCount;
	// volatile long	mConnected;
	ClientState			mState;

	friend class ClientSessionManager;
} ;



