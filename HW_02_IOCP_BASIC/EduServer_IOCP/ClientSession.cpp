#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//TODO: 이 영역 lock으로 보호 할 것
	FastSpinlockGuard lck( mLock );
	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}
	
	//TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것
	//HANDLE handle = 0; 
	//HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetCompletionPort(), (DWORD)mSocket, 0 );
	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetCompletionPort(), (DWORD)this, 0 );
	//DONE

	if (handle != GIocpManager->GetCompletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();

	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: 이 영역 lock으로 보호할 것
	FastSpinlockGuard lck( mLock );
	if ( !IsConnected() )
	{
		return;
	}
	
	LINGER lingerOption ;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) )
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	GSessionManager->DecreaseConnectionCount();

	closesocket(mSocket) ;

	mConnected = false ;
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//TODO: WSARecv 사용하여 구현할 것
	recvContext->mWsaBuf.buf = recvContext->mBuffer;
	recvContext->mWsaBuf.len = BUFSIZE;
	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	if ( WSARecv( mSocket, &(recvContext->mWsaBuf), 1, &dwBytes, &dwFlags, &(recvContext->mOverlapped), NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			printf( "%d\n", WSAGetLastError() );
			return false;
		}		
	}
	//DONE
	//flag랑 받은 byte는 따로 기록안해도되나 싶네요. 일단 NULL
	//↑ 반드시 있어야함..ㅠㅠ
	//내부에  wsabuf도 반드시 buf를 가리키게 할 것.
	//wsabuf.len이 0이면 recv할 때 아무것도 못받음 dwTransferred가 0이 됨
	//context를 초기화할때, buf도 mBuffer를 알아서 가리키도록 하는 것이 나을듯(len도 buffersize만큼 초기화해놓고..)

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);

	//TODO: WSASend 사용하여 구현할 것
	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	sendContext->mWsaBuf.len = len;
	if ( WSASend( mSocket, &sendContext->mWsaBuf, 1, NULL, 0, &(sendContext->mOverlapped), NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			printf( "%d\n", WSAGetLastError() );
			return false;
		}
	}
	//DONE	
	//1. overlappedIoContext 구조체는 어디서 해제하는거지ㅠㅠ 
	//overlapped구조체는 io가 끝나기전에 절대 메모리에서 사라지면 안된다고함
	//GQCS했을때도 이 구조체를 사용.. 거기서 sendCompletion으로 send확인하고 지움
	//2. 여기 넣어서 하는 역할은 뭐지!!!
	//GQCS에서 사용할 수 있도록 하는게 역할인듯. 나중에 context변수로 받아서 쓸 것이니 
	//context구조체 내부에 overlapped 변수가 가장 먼저 선언되어있어야함

	return true;
}