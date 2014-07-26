#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"


OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType) 
: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mBuffer(BUFSIZE), mConnected(0), mRefCount(0)
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

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

bool ClientSession::PostAccept()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);

	//TODO : AccpetEx를 이용한 구현.
	DWORD recvbytes = 0;

	BOOL bRetVal = AcceptEx( *GIocpManager->GetListenSocket(), mSocket, &mBuffer,
		0, sizeof (SOCKADDR_IN)+16, sizeof (SOCKADDR_IN)+16,
		&recvbytes, (LPWSAOVERLAPPED)acceptContext );

	if ( bRetVal == FALSE ) 
	{
		int err = WSAGetLastError();
		switch ( err )
		{
		case ERROR_IO_PENDING:
			// 일단 진행 중이다.
			break;
		case WSAECONNRESET:
			// 접속 생성하려는데 저쪽에서 끊어버렸다.
			// 소켓을 다시 재활용할 수 있도록 바꿔주자.
			// refCount 줄이면 0이 돼서 다시 등록되겠지?
			ReleaseRef();
			return true;
		default:
			CRASH_ASSERT( false );
			return false;
		}
	}

	// 소켓을 IOCP에 등록 - 안 하면 접속했는지 계속 확인해야 되잖아?
	HANDLE hCompPort = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
	
	if ( hCompPort != GIocpManager->GetComletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}
	// WIP

	return true;
}

void ClientSession::AcceptCompletion()
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);
	
	if (1 == InterlockedExchange(&mConnected, 1))
	{
		/// already exists?
		CRASH_ASSERT(false);
		return;
	}

	bool resultOk = true;
	do 
	{
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int opt = 1;
		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int addrlen = sizeof(SOCKADDR_IN);
		if (SOCKET_ERROR == getpeername(mSocket, (SOCKADDR*)&mClientAddr, &addrlen))
		{
			printf_s("[DEBUG] getpeername error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		//TODO: CreateIoCompletionPort를 이용한 소켓 연결
		/*
		HANDLE hCompPort = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
		
		if ( hCompPort == GIocpManager->GetComletionPort() )
		{
			printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
			// 연결 실패... 뭘 할까..
		}
		*/

		// AcceptEx하면서 이미 등록했는데 또 하나?
		// why do-while....
		// WIP

	} while (false);


	if (!resultOk)
	{
		DisconnectRequest(DR_ONCONNECT_ERROR);
		return;
	}

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	if (false == PreRecv())
	{
		printf_s("[DEBUG] PreRecv error: %d\n", GetLastError());
	}
}


void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	/// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0))
		return ;
	
	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	//TODO: DisconnectEx를 이용한 연결 끊기 요청
	// time_wait 문제를 해결해야 한다...
	// shutdown은 아닌 것 같은데....
	if ( shutdown( mSocket, SD_BOTH ) != 0 )
	{
		switch ( WSAGetLastError() )
		{
		case WSAECONNABORTED:
			// The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.
			// This error applies only to a connection - oriented socket.
			break;
		case WSAECONNRESET:
			// The virtual circuit was reset by the remote side executing a hard or abortive close.The application should close the socket as it is no longer usable.
			break;
		case WSAEINPROGRESS:
			// A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.
			break;
		case WSAEINVAL:
			// The how parameter is not valid, or is not consistent with the socket type.For example, SD_SEND is used with a UNI_RECV socket type.
			break;
		case WSAENETDOWN:
			// The network subsystem has failed.
			break;
		case WSAENOTCONN:
			// The socket is not connected. This error applies only to a connection-oriented socket.
			break;
		case WSAENOTSOCK:
			// The descriptor is not a socket.
			break;
		case WSANOTINITIALISED:
			// A successful WSAStartup call must occur before using this function.
			break;
		default:
			break;
		}
	}

	// DisconnectEx( mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0 );

	// WIP
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

	//TODO: zero-byte recv 구현
	if ( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "ClientSession::PostRecv Error : %d\n", GetLastError() );
			return false;
		}

	}
	// WIP

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
	FastSpinlockGuard criticalSection(mBufferLock);

	mBuffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	if (!IsConnected())
		return false;

	FastSpinlockGuard criticalSection(mBufferLock);

	if ( 0 == mBuffer.GetContiguiousBytes() )
		return true;

	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG) mBuffer.GetContiguiousBytes(); 
	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();

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
	FastSpinlockGuard criticalSection(mBufferLock);

	mBuffer.Remove(transferred);
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

	delete context;

	
}

