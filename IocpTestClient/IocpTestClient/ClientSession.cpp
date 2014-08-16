#include "stdafx.h"
#include "Exception.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include <string>


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

ClientSession::ClientSession() : mBuffer( BUFSIZE ), mConnected( 0 ), mRefCount( 0 )
{
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	//memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mBuffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}
	closesocket(mSocket);

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
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
			printf_s( "AcceptEx Error : %d\n", GetLastError() );

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


// 	bool resultOk = true;
// 
// 	if ( !resultOk )
// 	{
// 		DisconnectRequest( DR_ONCONNECT_ERROR );
// 		return;
// 	}AAAAA
// 	
	//printf_s( "[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa( mClientAddr.sin_addr ), ntohs( mClientAddr.sin_port ) );

	if ( false == PostSend() )
	{
		printf_s( "[DEBUG] PreRecv error: %d\n", GetLastError() );
	}
}





// bool ClientSession::PostAccept()
// {
// 	CRASH_ASSERT(LThreadType == THREAD_MAIN);
// 
// 	OverlappedConnectContext* acceptContext = new OverlappedConnectContext(this);
// 	DWORD bytes = 0;
// 	DWORD flags = 0;
// 	acceptContext->mWsaBuf.len = 0;
// 	acceptContext->mWsaBuf.buf = nullptr;
// 
// 
// 
// 	if (FALSE == AcceptEx(*GIocpManager->GetListenSocket(), mSocket, GIocpManager->mAcceptBuf, 0,
// 		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &bytes, (LPOVERLAPPED)acceptContext))
// 	{
// 		if (WSAGetLastError() != WSA_IO_PENDING)
// 		{
// 			DeleteIoContext(acceptContext);
// 			printf_s("AcceptEx Error : %d\n", GetLastError());
// 
// 			return false;
// 		}
// 	}
// 
// 	return true;
// }
// 
// void ClientSession::AcceptCompletion()
// {
// 	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);
// 	
// 	if (1 == InterlockedExchange(&mConnected, 1))
// 	{
// 		/// already exists?
// 		CRASH_ASSERT(false);
// 		return;
// 	}
// 
// 	bool resultOk = true;
// 	do 
// 	{
// 		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
// 		{
// 			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
// 			resultOk = false;
// 			break;
// 		}
// 
// 		int opt = 1;
// 		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
// 		{
// 			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
// 			resultOk = false;
// 			break;
// 		}
// 
// 		opt = 0;
// 		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
// 		{
// 			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
// 			resultOk = false;
// 			break;
// 		}
// 
// 		int addrlen = sizeof(SOCKADDR_IN);
// 		if (SOCKET_ERROR == getpeername(mSocket, (SOCKADDR*)&mClientAddr, &addrlen))
// 		{
// 			printf_s("[DEBUG] getpeername error: %d\n", GetLastError());
// 			resultOk = false;
// 			break;
// 		}
// 
// 		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
// 		if (handle != GIocpManager->GetComletionPort())
// 		{
// 			printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
// 			resultOk = false;
// 			break;
// 		}
// 
// 	} while (false);
// 
// 
// 	if (!resultOk)
// 	{
// 		DisconnectRequest(DR_ONCONNECT_ERROR);
// 		return;
// 	}
// 
// 	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
// 
// 	if (false == PreRecv())
// 	{
// 		printf_s("[DEBUG] PreRecv error: %d\n", GetLastError());
// 	}
// }

// 여기서 DISCONNECT관련 FUNCTION POINTER만들고 바로 사용.
void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	/// 이미 끊겼거나 끊기는 중이거나
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

	FastSpinlockGuard criticalSection(mBufferLock);

	if (0 == mBuffer.GetFreeSpaceSize())
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mBuffer.GetBuffer();
	

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
	FastSpinlockGuard criticalSection( mBufferLock );

	mBuffer.Remove( transferred );

	GIocpManager->IncreaseReadBytes( transferred );
// 	FastSpinlockGuard criticalSection(mBufferLock);
// 
// 	mBuffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	if (!IsConnected())
		return false;

	FastSpinlockGuard criticalSection(mBufferLock);
	
	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;

// 	sendContext->mWsaBuf.len = (ULONG) mBuffer.GetContiguiousBytes(); 
// 	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();
	char text[SEND_BUFF];
	memset( text, mClientId % 10 + 48, SEND_BUFF );	
	text[SEND_BUFF - 1] = '\0';
	sendContext->mWsaBuf.len = (ULONG)strlen(text);
	sendContext->mWsaBuf.buf = text;
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
	GIocpManager->IncreaseWriteBytes( transferred );
// 	FastSpinlockGuard criticalSection(mBufferLock);
// 
// 	mBuffer.Remove(transferred);
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

