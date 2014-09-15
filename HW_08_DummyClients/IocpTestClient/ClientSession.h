#pragma once
#include "ObjectPool.h"
#include "MemoryPool.h"
#include "CircularBuffer.h"
#include "ContentsConfig.h"
#include "Player.h"
#include "Crypt.h"

#define BUFSIZE	65536
#define SEND_BUFF 4096

class ClientSession ;
class SessionManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_CONNECT,
	IO_DISCONNECT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType);

	OVERLAPPED		mOverlapped ;
	ClientSession*	mSessionObject ;
	IOType			mIoType ;
	WSABUF			mWsaBuf;
	
} ;

//TODO: 아래의 OverlappedXXXXContext는 ObjectPool<>을 사용하도록 수정

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IO_SEND)
	{
	}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV)
	{
	}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO)
	{
	}
};

struct OverlappedDisconnectContext : public OverlappedIOContext, public ObjectPool<OverlappedDisconnectContext>
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr) 
	: OverlappedIOContext(owner, IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

struct OverlappedConnectContext : public OverlappedIOContext, public ObjectPool<OverlappedConnectContext>
{
	OverlappedConnectContext(ClientSession* owner) : OverlappedIOContext(owner, IO_CONNECT)
	{}
};
// WIP

void DeleteIoContext(OverlappedIOContext* context) ;

enum ClientState
{
	NOTHING = -1,
	SHARING_KEY,
	WAIT_FOR_LOGIN,
	LOGGED_IN,
	WAIT_FOR_LOGOUT,
};

//TODO: 아래의 ClientSession은 xnew/xdelete사용 가능하도록 클래스 정의 부분 수정
class ClientSession : public PooledAllocatable
{
public:
	ClientSession();
	~ClientSession() {}

	void	SessionReset();

	bool	IsConnected() const { return !!mConnected; }

	bool	PostConnect();
	void	ConnectCompletion();
	
	void	SetClientId( int val ) { mClientId = val; }
	const int	GetClientId() { return mClientId; }

	bool	PreRecv() ; ///< zero byte recv

	bool	PostRecv();
	void	RecvCompletion(DWORD transferred);

	bool	PostSend();
	void	SendCompletion(DWORD transferred);

	bool	WritePacket( PacketHeader* packet );
	bool	WritePacket( const char* packet, DWORD len );
	bool	PacketHandler();

	void	DisconnectRequest(DisconnectReason dr);
	void	DisconnectCompletion(DisconnectReason dr);
	
	void	AddRef();
	void	ReleaseRef();

	void	SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket;  }

	ClientState GetCurrentState() { return mState; }

	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_ACCEPTEX mFnAcceptEx;
	static LPFN_CONNECTEX mFnConnectEx;

	// 조심해!
	// 아래 전송 요청은 결국 세션이 하는 일인데, 내부적으로 플레이어를 거쳐서 다시 세션에서 처리함
	// 바로 세션에서 처리하도록 수정할 것
	void RequestLogin();
	void RequestMove();
	void RequestChat();

private:
	int				mClientId;
	SOCKET			mSocket ;

	SOCKADDR_IN		mClientAddr ;
		
	CircularBuffer	mSendBuffer;
	CircularBuffer	mRecvBuffer;

	volatile long	mRefCount;
	volatile long	mConnected;

	std::shared_ptr<Player> mPlayer;

	Crypt			mCrypt;
	ClientState		mState;
	
	friend class SessionManager;
} ;
// WIP



BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved );

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr* name, int nameLen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );