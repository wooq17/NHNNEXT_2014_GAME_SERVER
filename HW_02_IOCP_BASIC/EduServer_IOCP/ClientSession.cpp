#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//TODO: �� ���� lock���� ��ȣ �� ��
	mLock.EnterLock();
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
		mLock.LeaveLock();
		return false;
	}
	
	//TODO: ���⿡�� CreateIoCompletionPort((HANDLE)mSocket, ...);����Ͽ� ������ ��
	//HANDLE handle = 0; 
	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), (DWORD)mSocket, 0 );
	//DONE

	if (handle != GIocpManager->GetComletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		mLock.LeaveLock();
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();

	mLock.LeaveLock();
	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: �� ���� lock���� ��ȣ�� ��
	mLock.EnterLock();
	if ( !IsConnected() )
	{
		mLock.LeaveLock();
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
	mLock.LeaveLock();
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//TODO: WSARecv ����Ͽ� ������ ��
	if ( !WSARecv( mSocket, &recvContext->mWsaBuf, 1, NULL, 0, &(recvContext->mOverlapped), NULL ) )
	{
		return false;
	}
	//DONE
	//flag�� ���� byte�� ���� ��Ͼ��ص��ǳ� �ͳ׿�. �ϴ� NULL

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);

	//TODO: WSASend ����Ͽ� ������ ��
	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	sendContext->mWsaBuf.len = len;
	if ( !WSASend( mSocket, &sendContext->mWsaBuf, 1, NULL, 0, &(sendContext->mOverlapped), NULL ) )
	{
		return false;
	}
	//DONE	
	//1. overlappedIoContext ����ü�� ��� �����ϴ°����Ф� 
	//overlapped����ü�� io�� ���������� ���� �޸𸮿��� ������� �ȵȴٰ���
	//GQCS�������� �� ����ü�� ���.. �ű⼭ sendCompletion���� sendȮ���ϰ� ����
	//2. ���� �־ �ϴ� ������ ����!!!
	//GQCS���� ����� �� �ֵ��� �ϴ°� �����ε�. ���߿� context������ �޾Ƽ� �� ���̴� 
	//context����ü ���ο� overlapped ������ ���� ���� ����Ǿ��־����

	return true;
}