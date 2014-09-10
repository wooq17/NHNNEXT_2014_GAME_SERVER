﻿#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "Timer.h"
#include "Log.h"
#include "IocpManager.h"
#include "LockOrderChecker.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IOThread.h"
#include "ClientSessionManager.h"
#include "DBContext.h"

// #define GQCS_TIMEOUT	20 // INFINITE // 주기적인 작업을 실행하기 위해 타임아웃 설정 

IocpManager* GIocpManager = nullptr;

LPFN_DISCONNECTEX IocpManager::mFnDisconnectEx = nullptr;
LPFN_ACCEPTEX IocpManager::mFnAcceptEx = nullptr;
LPFN_CONNECTEX IocpManager::mFnConnectEx = nullptr;

char IocpManager::mAcceptBuf[64] = { 0, };


BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved )
{
	return IocpManager::mFnDisconnectEx( hSocket, lpOverlapped, dwFlags, reserved );
}

BOOL AcceptEx( SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped )
{
	return IocpManager::mFnAcceptEx( sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
		dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped );
}

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr* name, int namelen, PVOID lpSendBuffer, 
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped )
{
	return IocpManager::mFnConnectEx( hSocket, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped );
}

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{	
	memset( mIoWorkerThread, 0, sizeof( mIoWorkerThread ) );
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	mIoThreadCount = si.dwNumberOfProcessors;
	
	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mCompletionPort == NULL)
		return false;

	/// create TCP socket
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
		return false;

	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mCompletionPort, 0, 0);
	if (handle != mCompletionPort)
	{
		printf_s("[DEBUG] listen socket IOCP register error: %d\n", GetLastError());
		return false;
	}

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;

	GUID guidDisconnectEx = WSAID_DISCONNECTEX ;
	DWORD bytes = 0 ;
	if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&guidDisconnectEx, sizeof(GUID), &mFnDisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, NULL, NULL) )
		return false;

	GUID guidAcceptEx = WSAID_ACCEPTEX ;
	if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(GUID), &mFnAcceptEx, sizeof(LPFN_ACCEPTEX), &bytes, NULL, NULL))
		return false;
	
	/// make session pool
	GClientSessionManager->PrepareClientSessions();

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// create I/O Thread
	for ( int i = 0; i < MAX_IO_THREAD; ++i )
	{
		DWORD dwThreadId;
		/// 스레드ID는 DB 스레드 이후에 IO 스레드로..
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, (LPVOID)( i + MAX_DB_THREAD ), CREATE_SUSPENDED, (unsigned int*)&dwThreadId );
		if ( hThread == NULL )
			return false;

		mIoWorkerThread[i] = new IOThread( hThread, mCompletionPort );
	}

	/// start!
	for ( int i = 0; i < MAX_IO_THREAD; ++i )
	{
		ResumeThread( mIoWorkerThread[i]->GetHandle() );
	}

	return true;
}


void IocpManager::StartAccept()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
	{
		printf_s("[DEBUG] listen error\n");
		return;
	}
		
	while ( GClientSessionManager->AcceptClientSessions() )
	{
		Sleep(100);
	}

}


void IocpManager::Finalize()
{
	for ( int i = 0; i < MAX_IO_THREAD; ++i )
	{
		CloseHandle( mIoWorkerThread[i]->GetHandle() );
	}

	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;
	LWorkerThreadId = reinterpret_cast<int>( lpParam );
	LTimer = new Timer;
	LLockOrderChecker = new LockOrderChecker( LWorkerThreadId );
	LSendRequestSessionList = new xdeque<Session*>::type;
	GThreadCallHistory[LWorkerThreadId] = LThreadCallHistory = new ThreadCallHistory( LWorkerThreadId );
	GThreadCallElapsedRecord[LWorkerThreadId] = LThreadCallElapsedRecord = new ThreadCallElapsedRecord( LWorkerThreadId );

	/// 반드시 DB 쓰레드를 먼저 띄운 후에 진입해야 한다.
	CRASH_ASSERT( LWorkerThreadId >= MAX_DB_THREAD );

	return GIocpManager->mIoWorkerThread[LWorkerThreadId - MAX_DB_THREAD]->Run();
}

void IocpManager::PostDatabaseResult( DatabaseJobContext* dbContext )
{
	if ( FALSE == PostQueuedCompletionStatus( mCompletionPort, 0, (ULONG_PTR)CK_DB_RESULT, (LPOVERLAPPED)dbContext ) )
	{
		printf_s( "IocpManager::PostDatabaseResult PostQueuedCompletionStatus Error: %d\n", GetLastError() );
		CRASH_ASSERT( false );
	}
}
