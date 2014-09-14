#pragma once
#include "ObjectPool.h"
#include "MemoryPool.h"
#include "CircularBuffer.h"
#include "SyncExecutable.h"
#include "Session.h"
#include "Player.h"
// #include "RSA.h"
#include "Crypt.h"

class ClientSessionManager;

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

	void	PacketHandling();

	virtual void OnDisconnect( DisconnectReason dr );
	virtual void OnRelease();

public:
	std::shared_ptr<Player> mPlayer;

private:
	
	// SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
	Crypt			mCrypt;
		
	// FastSpinlock	mBufferLock;

	// CircularBuffer	mBuffer;

	// volatile long	mRefCount;
	// volatile long	mConnected;

	friend class ClientSessionManager;
} ;



