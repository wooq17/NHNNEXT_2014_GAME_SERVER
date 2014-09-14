#include "stdafx.h"
#include "Exception.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include <string>
#include "RSA.h"

LPFN_DISCONNECTEX ClientSession::mFnDisconnectEx = nullptr;
LPFN_CONNECTEX ClientSession::mFnConnectEx = nullptr;

BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved )
{
	return ClientSession::mFnDisconnectEx( hSocket, lpOverlapped, dwFlags, reserved );
}

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr* name, int nameLen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped )
{
	return ClientSession::mFnConnectEx( hSocket, name, nameLen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped );
}



OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType) 
: mSessionObject(owner), mIoType(ioType)
{
	
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mRecvBuffer( BUFSIZE ), mSendBuffer( BUFSIZE ), mConnected( 0 ), mRefCount( 0 ), mPlayer( new Player( this ) ), mIsKeyShared( false )
{
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mSendBuffer.BufferReset();
	mRecvBuffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}
	closesocket(mSocket);

	mSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );

	mCrypt.ReleaseResources();
	mIsKeyShared = false;
}

bool ClientSession::PostConnect()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	DWORD bytes = 0;
	if ( SOCKET_ERROR == WSAIoctl( mSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof( GUID ), &mFnDisconnectEx, sizeof( LPFN_DISCONNECTEX ), &bytes, NULL, NULL ) )
		return false;
		
	GUID guidConnectEx = WSAID_CONNECTEX;
	if ( SOCKET_ERROR == WSAIoctl( mSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof( GUID ), &mFnConnectEx, sizeof( LPFN_CONNECTEX ), &bytes, NULL, NULL ) )
		return false;

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = 0;
	client.sin_addr.s_addr = INADDR_ANY;
	bind( mSocket, (const sockaddr*)&client, sizeof( client ) );

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetCompletionPort(), ( ULONG_PTR )this, 0 );
	if ( handle != GIocpManager->GetCompletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
	}

	sockaddr_in* serverAddr = GSessionManager->GetServerAddr();
	
	OverlappedConnectContext* connectContext = new OverlappedConnectContext( this );
	connectContext->mWsaBuf.len = 0;
	connectContext->mWsaBuf.buf = nullptr;

	if ( FALSE == ConnectEx( mSocket, (const sockaddr*)serverAddr, sizeof(*serverAddr), 0,0,0,(LPOVERLAPPED)connectContext ))
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext(connectContext);
			printf_s( "ConnectEx Error : %d\n", GetLastError() );

			return false;
		}
	}

	return true;
}

void ClientSession::ConnectCompletion()
{
	CRASH_ASSERT( LThreadType == THREAD_IO_WORKER );

	if ( 1 == InterlockedExchange( &mConnected, 1 ) )
	{
		/// already exists?
		CRASH_ASSERT( false );
		return;
	}

	if ( false == PreRecv() )
	{
		printf_s( "[DEBUG] PreRecv error: %d\n", GetLastError() );
	}
}

// ���⼭ DISCONNECT���� FUNCTION POINTER����� �ٷ� ���.
void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	/// �̹� ����ų� ����� ���̰ų�
	if (0 == InterlockedExchange(&mConnected, 0))
		return ;
	
	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	if (FALSE == DisconnectEx(mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(context);
			printf_s("ClientSession::DisconnectRequest Error : %d\n", GetLastError());
		}
	}
}

void ClientSession::DisconnectCompletion(DisconnectReason dr)
{
	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	/// release refcount when added at issuing a session
	ReleaseRef();
}


bool ClientSession::PreRecv()
{
	if (!IsConnected())
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PreRecv Error : %d\n", GetLastError());
			return false;
		}
	}

	return true;
}

bool ClientSession::PostRecv()
{
	if (!IsConnected())
		return false;

	//FastSpinlockGuard criticalSection(mBufferLock);

	if (0 == mRecvBuffer.GetFreeSpaceSize())
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mRecvBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mRecvBuffer.GetBuffer();
	

	/// start real recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PostRecv Error : %d\n", GetLastError());
			return false;
		}
			
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{	
	// ���ۿ� ���� ���̸�ŭ ����
 	mRecvBuffer.Commit(transferred);

	PacketHandler();
// 	while ( !PacketHandler() )
// 	{
// 	}
}



bool ClientSession::PostSend()
{
	if (!IsConnected())
		return false;

	//FastSpinlockGuard criticalSection(mBufferLock);
	
	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;

	sendContext->mWsaBuf.len = (ULONG) mSendBuffer.GetContiguiousBytes(); 
	sendContext->mWsaBuf.buf = mSendBuffer.GetBufferStart();

	/// start async send
	if (SOCKET_ERROR == WSASend(mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(sendContext);
			printf_s("ClientSession::PostSend Error : %d\n", GetLastError());

			return false;
		}
			
	}

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	//FastSpinlockGuard criticalSection( mBufferLock );
	
	// ���� �ʱ�ȭ
	mSendBuffer.Remove(transferred);
}


bool ClientSession::WritePacket( PacketHeader* packet )
{
	//FastSpinlockGuard criticalSection( mBufferLock );

	assert( packet->mType != 0 );

	if ( mSendBuffer.GetFreeSpaceSize() < packet->mSize )
		return false;

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, packet, packet->mSize );

	if ( mIsKeyShared )
	{
		if ( !mCrypt.Encrypt( (PBYTE)destData + sizeof( PacketHeader ), ( (PacketHeader*)destData )->mSize ) )
			printf( "[DH] Decrypt failed\n" );
	}

	mSendBuffer.Commit( packet->mSize );

	return true;
}

bool ClientSession::WritePacket( const char* packet, DWORD len )
{
	//FastSpinlockGuard criticalSection( mBufferLock );

	if ( mSendBuffer.GetFreeSpaceSize() < len )
		return false;

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, packet, len );

	mSendBuffer.Commit( len );

	return true;
}

bool ClientSession::PacketHandler()
{
	size_t len = mRecvBuffer.GetContiguiousBytes();

	if ( len == 0 )
		return true;

	PacketHeader* recvPacket = reinterpret_cast<PacketHeader*>( mRecvBuffer.GetBufferStart() );

	if ( mIsKeyShared )
	{
		if ( !mCrypt.Decrypt( (PBYTE)recvPacket + sizeof( PacketHeader ), recvPacket->mSize ) )
			printf( "[DH] Decrypt failed\n" );
	}

	switch ( recvPacket->mType )
	{
		case PKT_SC_BASE_PUBLIC_KEY:
		{
			RSA::Init();

			PBYTE data = (PBYTE)recvPacket + sizeof( PacketHeader );
			PBYTE base = nullptr;
			if ( !RSA::Decrypt( data, P_LENGTH + G_LENGTH, &base ) )
				printf( "fail\n" );

			DATA_BLOB clientG;
			DATA_BLOB clientP;
			PBYTE pos = base;

			clientG.cbData = *pos;
			clientG.pbData = pos + 4;
			pos += 68;
			mCrypt.SetG( clientG );

			clientP.cbData = *pos;
			clientP.pbData = pos + 4;
			pos += 68;
			mCrypt.SetP( clientP );

			// printf( "%d / %d", clientG.cbData, clientP.cbData );

			// send client public key
			if ( !mCrypt.GeneratePrivateKey() )
				break;

			DWORD exportedLen = 0;
			PBYTE exportedData = mCrypt.ExportPublicKey( &exportedLen );

			unsigned short pktLen = sizeof(PacketHeader)+sizeof(DWORD)+exportedLen;
			PBYTE pkt = (PBYTE)malloc( pktLen );
			PBYTE currentPos = pkt;

			( (PacketHeader*)currentPos )->mSize = pktLen;
			( (PacketHeader*)currentPos )->mType = PKT_CS_EXPORT_PUBLIC_KEY;
			currentPos += sizeof( PacketHeader );

			memcpy( currentPos, &exportedLen, sizeof( DWORD ) );
			currentPos += sizeof( DWORD );

			memcpy( currentPos, exportedData, exportedLen );

			if ( !WritePacket( (char*)pkt, pktLen ) )
			{
				printf( "[RSA] post key fail %d\n", GetLastError() );
				DisconnectRequest( DR_ONCONNECT_ERROR );
			}
			PostSend();

			delete pkt;
		}
			break;
		case PKT_SC_EXPORT_PUBLIC_KEY:
		{
			DWORD exportedLen = 0;
			memcpy( &exportedLen, recvPacket + sizeof( PacketHeader ), sizeof( DWORD ) );
			PBYTE exportedData = PBYTE(recvPacket) + sizeof(PacketHeader)+sizeof( DWORD );

			if ( !mCrypt.GenerateSessionKey( exportedData, exportedLen ) )
				break;

			mIsKeyShared = true;
		}
			break;
		case PKT_SC_LOGIN:
		{
			LoginResponse* clientPacket = reinterpret_cast<LoginResponse*>( recvPacket );
			mPlayer->Start( clientPacket->mPlayerId );
			wprintf_s( L"[LOG] %s <<<< login packet \n", mPlayer->GetName() );

			// move (0,0,0) to random position
			GSessionManager->CreateMovePacket( this );
		}
			break;
		case PKT_SC_LOGOUT:
		{
			LogoutResponse* clientPacket = reinterpret_cast<LogoutResponse*>( recvPacket );
			mPlayer->PlayerReset();
			wprintf_s( L"[LOG] %s <<<< logout packet\n", mPlayer->GetName() );
			DisconnectRequest( DR_NONE );
		}
			break;
		case PKT_SC_CHAT:
		{
			if ( !mPlayer->IsLoaded() )
			{
				wprintf_s( L"[DEBUG] LOGOUT PLAYER GOT A MESSAGE" );
				break;
			}
				
			ChatBroadcastResponse* clientPacket = reinterpret_cast<ChatBroadcastResponse*>( recvPacket );
			if ( mPlayer->GetPlayerId() != clientPacket->mPlayerId )
				break;
			
			--mPlayer->mHealth;
			wprintf_s( L"[LOG] %s : %s Health : %d\n", clientPacket->mName, clientPacket->mChat, mPlayer->GetPlayerHealth() );

			if ( !mPlayer->IsAlive() )
			{
				wprintf_s( L"[LOG] %s player dead\n", mPlayer->GetName() );
				if ( mPlayer->SendLogout() )
				{
					wprintf_s( L"[LOG] %s >>>> logout packet\n", mPlayer->GetName() );
				}
			}
		}
			break;
		case PKT_SC_MOVE:
		{
			if ( !mPlayer->IsLoaded() )
			{
				wprintf_s( L"[DEBUG] LOGOUT PLAYER MOVES" );
				break;
			}

			MoveRequest* clientPacket = reinterpret_cast<MoveRequest*>( recvPacket );
			if ( mPlayer->GetPlayerId() != clientPacket->mPlayerId )
				break;

			mPlayer->SetPosition( clientPacket->mX, clientPacket->mY, clientPacket->mZ );
			wprintf_s( L"[LOG] %s <<<< move packet ( %f , %f , %f )\n", mPlayer->GetName(), clientPacket->mX, clientPacket->mY, clientPacket->mZ );
		}
			break;
		default:
			printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
			break;
	}

	// ��Ŷ ó���������� ó���� ��Ŷ ũ�⸸ŭ ����
	DWORD retVal = recvPacket->mSize;
	mRecvBuffer.Remove( recvPacket->mSize );

	return retVal == len;
}

void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		GSessionManager->ReturnClientSession(this);
	}
}


void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context)
		return;

	context->mSessionObject->ReleaseRef();

	/// ObjectPool's operate delete dispatch
	switch (context->mIoType)
	{
	case IO_SEND:
		delete static_cast<OverlappedSendContext*>(context);
		break;

	case IO_RECV_ZERO:
		delete static_cast<OverlappedPreRecvContext*>(context);
		break;

	case IO_RECV:
		delete static_cast<OverlappedRecvContext*>(context);
		break;

	case IO_DISCONNECT:
		delete static_cast<OverlappedDisconnectContext*>(context);
		break;

	case IO_CONNECT:
		delete static_cast<OverlappedConnectContext*>(context);
		break;

	default:
		CRASH_ASSERT(false);
	}

	
}

