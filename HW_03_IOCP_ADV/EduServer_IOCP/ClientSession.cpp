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
	acceptContext->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize( ); ///# 0으로
	acceptContext->mWsaBuf.buf = mBuffer.GetBuffer( );  ///# nullptr

	BOOL bRetVal = IocpManager::AcceptEx( *GIocpManager->GetListenSocket( ), mSocket, acceptContext->mWsaBuf.buf, ///# accept용 임시 buf하나 만들어서 사용.
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
			// 원격 호스트가 이상하다. 그런데 여기로 들어올 수 있나...
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
	///# 이건 여기서 하는것이 아니다 -.-;

	HANDLE hCompPort = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
	
	if ( hCompPort != GIocpManager->GetComletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}
	// DONE

	return true;
}

void ClientSession::AcceptCompletion()
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	// 조심해!
	// 밑에서 버퍼 초기화 하니까 락 잡고 시작
	FastSpinlockGuard criticalSection( mBufferLock );
	
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

		///# 여기에 IOCP와 소켓 연결을 애야 한다. 연결이 성사 되었을 때, 해당 소켓을 감시하겠다고 IOCP여 연결 요청하는 개념.

		//TODO: CreateIoCompletionPort를 이용한 소켓 연결
		/*
		HANDLE hCompPort = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
		
		if ( hCompPort != GIocpManager->GetComletionPort() )
		{
			printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
			// 연결 실패... 뭘 할까..
		}
		*/
		// AcceptEx하면서 이미 등록했는데 또 하나요?
		
		// why do-while....
		// WIP

	} while (false);


	if (!resultOk)
	{
		DisconnectRequest(DR_ONCONNECT_ERROR);
		return;
	}

	// 조심해!
	// acceptEx()하면서 mBuffer에 데이터(로컬 및 리모트 주소)는 썼는데
	// 이건 이제 안 쓰는 데이터니까 버퍼를 다시 초기화 해줘야 될 것 같은데...
	
	///# 그러니까 acceptex용 전용 버퍼 하나 만들어 놓으면 됨. 
	mBuffer.BufferReset();

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
	
	// time_wait 문제를 해결해야 한다...?
	// 이미 LINGER 설정으로 TIME_OUT 안 되지 않나...

	///# 에러처리 안하는가?
	IocpManager::DisconnectEx( mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0 );

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
			printf_s( "ClientSession::PreRecv Error : %d\n", GetLastError() );
			return false;
		}

	}
	// DONE

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
	recvContext->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize( );
	recvContext->mWsaBuf.buf = mBuffer.GetBuffer();
	

	/// start real recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		int err = WSAGetLastError();
		if ( err != WSA_IO_PENDING )
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

