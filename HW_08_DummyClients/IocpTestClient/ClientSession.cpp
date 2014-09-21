#include "stdafx.h"
#include "Exception.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include <string>
#include "RSA.h"
#include "MyPacket.pb.h"

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

ClientSession::ClientSession() 
: mRecvBuffer( BUFSIZE ), mSendBuffer( BUFSIZE ), mConnected( 0 ), mRefCount( 0 ), mPlayer( new Player( this ) ), mState( NOTHING ), mSendPendingCount( 0 ), mIsKeyShared( false )
{
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mSendBufferLock.EnterWriteLock();
	mSendBuffer.BufferReset();
	mSendBufferLock.LeaveWriteLock();

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
	mState = NOTHING;
	mIsKeyShared = false;
	mSendPendingCount = 0;
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

	mState = SHARING_KEY;
	GSessionManager->RegisterActiveSession( this );
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

	if ( mIsKeyShared )
	{
		if ( !mCrypt.Decrypt( (PBYTE)mRecvBuffer.GetBufferStart(), transferred ) )
			printf( "[DH] Decrypt failed\n" );
	}

 	while ( !PacketHandler() )
 	{
 	}
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection( mSendBufferLock );
	
	// ���� �ʱ�ȭ
	mSendBuffer.Remove(transferred);

	--mSendPendingCount;
}

bool ClientSession::PostSend( const char* packet, DWORD len )
{
	FastSpinlockGuard criticalSection( mSendBufferLock );

	if ( !IsConnected() )
		return false;

	if ( mSendBuffer.GetFreeSpaceSize() < len )
		return false;

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, packet, len );

	mSendBuffer.Commit( len );

	return true;
}

bool ClientSession::FlushSend()
{
	if ( !IsConnected() )
	{
		return true;
	}

	FastSpinlockGuard criticalSection( mSendBufferLock );

	/// ���� �����Ͱ� ���� ���
	if ( 0 == mSendBuffer.GetContiguiousBytes() )
	{
		/// ���� �����͵� ���� ���
		if ( 0 == mSendPendingCount )
			return true;

		return false;
	}

	/// ������ send�� �Ϸ� �ȵ� ���
	if ( mSendPendingCount > 0 )
		return false;

	// for debug (use before encrypt)
	// CRASH_ASSERT( IsValidData( (PacketHeader*)sendContext->mWsaBuf.buf, sendContext->mWsaBuf.len ) );

	char* start = mSendBuffer.GetBufferStart();
	ULONG len = mSendBuffer.GetContiguiousBytes();\

	if ( mIsKeyShared )
	{
		if ( !mCrypt.Encrypt( (PBYTE)( start ), len ) )
			printf( "[DH] Decrypt failed\n" );
	}

	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)len;
	sendContext->mWsaBuf.buf = start;

	/// start async send
	if ( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );

			return true;
		}

	}

	mSendPendingCount++;

	return mSendPendingCount == 1;
}

bool ClientSession::PacketHandler()
{
	size_t len = mRecvBuffer.GetContiguiousBytes();

	if ( len == 0 )
		return true;

	PacketHeader* recvPacket = reinterpret_cast<PacketHeader*>( mRecvBuffer.GetBufferStart() );

	switch ( recvPacket->mType )
	{
		case MyPacket::PKT_SC_BASE_PUBLIC_KEY:
			ResponseBaseKey( recvPacket );
			break;
		case MyPacket::PKT_SC_EXPORT_PUBLIC_KEY:
			ResponseExportedKey( recvPacket );
			break;
		case MyPacket::PKT_SC_LOGIN:
			ResponseLogin( recvPacket );
			break;
		case MyPacket::PKT_SC_LOGOUT:
			ResponseLogout( recvPacket );
			break;
		case MyPacket::PKT_SC_CHAT:
			ResponseChat( recvPacket );
			break;
		case MyPacket::PKT_SC_MOVE:
			ResponseMove( recvPacket );
			break;
		default:
			// printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
			break;
	}

	// ��Ŷ ó���������� ó���� ��Ŷ ũ�⸸ŭ ����
	DWORD processedLen = recvPacket->mSize;
	mRecvBuffer.Remove( processedLen );

	return processedLen == len;
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

void ClientSession::RequestLogin()
{
	// contents ����
	wchar_t name[6];
	name[0] = static_cast<wchar_t>( rand() % 26 + 65 );
	name[1] = static_cast<wchar_t>( rand() % 26 + 65 );
	name[2] = static_cast<wchar_t>( rand() % 26 + 65 );
	name[3] = static_cast<wchar_t>( rand() % 26 + 65 );
	name[4] = static_cast<wchar_t>( rand() % 26 + 65 );
	name[5] = '\0';

	mPlayer->SetName( name );

	char playerName[6];
	WideCharToMultiByte( CP_ACP, 0, name, -1, playerName, 6, NULL, NULL );
	
	// payload����
	MyPacket::LoginRequest loginRequest;
	loginRequest.set_playername( playerName );

	// header����
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_CS_LOGIN;

	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = loginRequest.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// ����	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = loginRequest.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );

	if ( !PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize ) )
	{
		// disconnect
		DisconnectRequest( DR_IO_REQUEST_ERROR );
	}
	delete[] encoded;

 //	LoginRequest loginRequest;
// 
// 	wcscpy_s( loginRequest.mName, name );
// 
// 	if ( !PostSend( reinterpret_cast<const char*>( &loginRequest ), loginRequest.mSize ) )
// 	{
// 		// disconnect
// 		DisconnectRequest( DR_IO_REQUEST_ERROR );
// 	}

	// wprintf_s( L"[LOG] %s >>>> login packet\n", name );
}

void ClientSession::RequestMove()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	// contents ����
	float x = static_cast<float>( rand() % 2000 - 1000 );
	float y = static_cast<float>( rand() % 2000 - 1000 );
	float z = static_cast<float>( rand() % 2000 - 1000 );
	//Float3D pos{ x, y, z };
	
	// payload����
	MyPacket::MoveRequest moveReqeust;	
	moveReqeust.set_playerid( mPlayer->GetPlayerId() );
	moveReqeust.mutable_playerpos()->set_x( x );
	moveReqeust.mutable_playerpos()->set_y( y );
	moveReqeust.mutable_playerpos()->set_z( z );

	// header����
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_CS_MOVE;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = moveReqeust.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// ����	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = moveReqeust.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );
	
	if ( !PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize ) )
	{
		// disconnect
		DisconnectRequest( DR_IO_REQUEST_ERROR );
	}
	delete[] encoded;

// 	MoveRequest moveRequest;
// 	moveRequest.mPlayerId = mPlayer->GetPlayerId();
// 	moveRequest.mX = pos.m_X;
// 	moveRequest.mY = pos.m_Y;
// 	moveRequest.mZ = pos.m_Z;
// 
// 	if ( !PostSend( reinterpret_cast<const char*>( &moveRequest ), moveRequest.mSize ) )
// 	{
// 		// disconnect
// 		DisconnectRequest( DR_IO_REQUEST_ERROR );
// 	}

	// wprintf_s( L"[LOG] %s >>>> move to ( %f , %f , %f )\n", mPlayer->GetName(), pos.m_X, pos.m_Y, pos.m_Z );
}

void ClientSession::RequestChat()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	// contents ����
	wchar_t chat[11];
	chat[0] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[1] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[2] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[3] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[4] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[5] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[6] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[7] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[8] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[9] = static_cast<wchar_t>( rand() % 26 + 97 );
	chat[10] = '\0';

	char chatMessage[11];
	WideCharToMultiByte( CP_ACP, 0, chat, -1, chatMessage, 6, NULL, NULL );

	// payload ����
	MyPacket::ChatRequest chatReqeust;
	chatReqeust.set_playerid( mPlayer->GetPlayerId() );
	chatReqeust.set_playermessage( chatMessage );

	// header ����
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_CS_CHAT;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = chatReqeust.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// ����	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = chatReqeust.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );

	if ( !PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize ) )
	{
		// disconnect
		DisconnectRequest( DR_IO_REQUEST_ERROR );
	}
	delete[] encoded;


// 	ChatBroadcastRequest chatRequest;
// 	chatRequest.mPlayerId = mPlayer->GetPlayerId();
// 	wcscpy_s( chatRequest.mChat, chat );
// 
// 	if ( !PostSend( reinterpret_cast<const char*>( &chatRequest ), chatRequest.mSize ) )
// 	{
// 		// disconnect
// 		DisconnectRequest( DR_IO_REQUEST_ERROR );
// 	}
// 
// 	wprintf_s( L"[LOG] %s >>>> chat Message : %s\n", mPlayer->GetName(), chat );
}

void ClientSession::RequestLogout()
{
	if ( mState != LOGGED_IN )
		return;

	mState = WAIT_FOR_LOGOUT;

	// payload ����
	MyPacket::MoveRequest logoutReqeust;
	logoutReqeust.set_playerid( mPlayer->GetPlayerId() );	

	// header����
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_CS_LOGOUT;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = logoutReqeust.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// ����	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = logoutReqeust.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );

	if ( !PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize ) )
	{
		// disconnect
		DisconnectRequest( DR_IO_REQUEST_ERROR );
	}
	delete[] encoded;

// 	LogoutRequest logoutRequest;
// 	logoutRequest.mPlayerId = mPlayer->GetPlayerId();
// 
// 	if ( !PostSend( reinterpret_cast<const char*>( &logoutRequest ), logoutRequest.mSize ) )
// 	{
// 		// disconnect
// 		DisconnectRequest( DR_IO_REQUEST_ERROR );
// 	}

	// wprintf_s( L"[LOG] %s >>>> logout packet\n", mPlayer->GetName() );
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

bool ClientSession::IsValidData( PacketHeader* start, ULONG len )
{
	ULONG remainLen = len;
	char* currentPos = (char*)start;
	PacketHeader* pktHeader = (PacketHeader*)currentPos;

	// printf( "header : %d / len : %d\n", pktHeader->mType, pktHeader->mSize );

	if ( len % 284 == 0 && len > 284)
		printf("Here!!\n");

	while ( pktHeader->mSize != remainLen )
	{
		if ( pktHeader->mSize == 0 )
			return false;

		// ���� ��ġ�� ���� ��ȿ�� ��Ŷ �������ΰ�
		if ( pktHeader->mSize > remainLen )
			return false;

		remainLen -= pktHeader->mSize;
		currentPos += pktHeader->mSize;
		pktHeader = (PacketHeader*)currentPos;

		printf( "header : %d / len : %d\n", pktHeader->mType, pktHeader->mSize );
	}

	return true;
}

void ClientSession::ResponseBaseKey( PacketHeader* recvPacket )
{
	// RSA ��� ����
	/*
	printf( "PKT_SC_BASE_PUBLIC_KEY\n" );

	PBYTE data = (PBYTE)recvPacket + sizeof( PacketHeader );
	PBYTE base = nullptr;
	if ( !GRSA->Decrypt( data, P_LENGTH + G_LENGTH, &base ) )
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
	{
		DisconnectRequest( DR_IO_REQUEST_ERROR );
		return;
	}

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

	if ( !PostSend( (char*)pkt, pktLen ) )
	{
		printf( "[RSA] post key fail %d\n", GetLastError() );
		DisconnectRequest( DR_ONCONNECT_ERROR );
	}
	
	GSessionManager->RegisterSendRequest( this );

	delete pkt;
	*/
	if ( mState != SHARING_KEY )
		return;

	DATA_BLOB clientG;
	DATA_BLOB clientP;
	PBYTE pos = (PBYTE)recvPacket + sizeof( PacketHeader );

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
	{
		DisconnectRequest( DR_IO_REQUEST_ERROR );
		return;
	}

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

	if ( !PostSend( (char*)pkt, pktLen ) )
	{
		// printf( "[DH] post key fail %d\n", GetLastError() );
		DisconnectRequest( DR_ONCONNECT_ERROR );
	}

	delete pkt;
}

void ClientSession::ResponseExportedKey( PacketHeader* recvPacket )
{
	if ( mState != SHARING_KEY )
		return;

	DWORD exportedLen = 0;
	memcpy( &exportedLen, recvPacket + sizeof( PacketHeader ), sizeof( DWORD ) );
	PBYTE exportedData = PBYTE( recvPacket ) + sizeof(PacketHeader)+sizeof( DWORD );

	if ( !mCrypt.GenerateSessionKey( exportedData, exportedLen ) )
	{
		printf( "[DH] GenerateSessionKey fail %d\n", GetLastError() );
		DisconnectRequest( DR_ONCONNECT_ERROR );
	}

	mIsKeyShared = true;

	mState = WAIT_FOR_LOGIN;
	RequestLogin();
}

void ClientSession::ResponseLogin( PacketHeader* recvPacket )
{
	
	if ( mState != WAIT_FOR_LOGIN )
		return;

	size_t packetHeaderSize = sizeof( PacketHeader );
	MyPacket::LoginResult loginResult;
	loginResult.ParseFromArray( recvPacket + 1, recvPacket->mSize );
	mPlayer->Start( loginResult.playerid() );

// 	LoginResponse* clientPacket = reinterpret_cast<LoginResponse*>( recvPacket );
// 	mPlayer->Start( clientPacket->mPlayerId );
	// wprintf_s( L"[LOG] %s <<<< login packet \n", mPlayer->GetName() );

	mState = LOGGED_IN;
}

void ClientSession::ResponseLogout( PacketHeader* recvPacket )
{
	if ( mState != WAIT_FOR_LOGOUT )
		return;

	size_t packetHeaderSize = sizeof( PacketHeader );
	MyPacket::LogoutResult logoutResult;
	logoutResult.ParseFromArray( recvPacket + 1, recvPacket->mSize );

	if ( mPlayer->mPlayerId != logoutResult.playerid() )
		printf_s( "\nWrong ID\n\n" );
	else
	{
		mPlayer->PlayerReset();
		// wprintf_s( L"[LOG] %s <<<< logout packet\n", mPlayer->GetName() );
		DisconnectRequest( DR_LOGOUT );
	}



// 	LogoutResponse* clientPacket = reinterpret_cast<LogoutResponse*>( recvPacket );
// 	mPlayer->PlayerReset();
// 	// wprintf_s( L"[LOG] %s <<<< logout packet\n", mPlayer->GetName() );
// 	DisconnectRequest( DR_NONE );

	mState = WAIT_FOR_LOGOUT;
}

void ClientSession::ResponseChat( PacketHeader* recvPacket )
{
	if ( mState != LOGGED_IN )
		return;

	size_t packetHeaderSize = sizeof( PacketHeader );
	MyPacket::ChatResult chatResult;
	chatResult.ParseFromArray( recvPacket + 1, recvPacket->mSize );

	//ChatBroadcastResponse* clientPacket = reinterpret_cast<ChatBroadcastResponse*>( recvPacket );

	// if ( mPlayer->GetPlayerId() == clientPacket->mPlayerId )
	if ( true )
	{
		mPlayer->DecrementHealth();
		// wprintf_s( L"[LOG] %s : Health = %d\n", clientPacket->mName, mPlayer->GetPlayerHealth() );
	}

	// [LOG] %s >>>> chat Message : %s\n
	// wprintf_s( L"[LOG] %s <<<< chat Message : %s\n", clientPacket->mName, clientPacket->mChat );

	if ( !mPlayer->IsAlive() )
	{
		// wprintf_s( L"[LOG] %s player dead\n", mPlayer->GetName() );
		RequestLogout();
	}
}

void ClientSession::ResponseMove( PacketHeader* recvPacket )
{
	if ( mState != LOGGED_IN )
		return;

	size_t packetHeaderSize = sizeof( PacketHeader );
	MyPacket::MoveResult moveResult;
	moveResult.ParseFromArray( recvPacket + 1, recvPacket->mSize );
	MyPacket::Position pos = moveResult.playerpos();
	mPlayer->SetPosition( pos.x(), pos.y(), pos.z() );

	//MoveRequest* clientPacket = reinterpret_cast<MoveRequest*>( recvPacket );

	//mPlayer->SetPosition( clientPacket->mX, clientPacket->mY, clientPacket->mZ );
	// wprintf_s( L"[LOG] %s <<<< move packet ( %f , %f , %f )\n", mPlayer->GetName(), clientPacket->mX, clientPacket->mY, clientPacket->mZ );
}