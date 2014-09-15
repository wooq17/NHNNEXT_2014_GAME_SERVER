#include "stdafx.h"
#include "Exception.h"
#include "IocpManager.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	20 // INFINITE //20

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

char IocpManager::mAcceptBuf[64] = { 0, };

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{	
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize( int time )
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

	// second to millisecond
	mTime = time * 1000;
	mProcessStartTime = ::GetTickCount();
		
	/// make session pool
	GSessionManager->PrepareSessions();

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)	
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i+1), 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return false;
	}

	return true;
}

// 이걸 START CONNECT로.. 전부 커넥션 ex수행하도록 바꾸면 될듯.
void IocpManager::StartConnect()
{
	while (GSessionManager->ConnectSessions())
	{
		if ( ::GetTickCount() - mProcessStartTime > mTime )
		{
			DWORD a = ::GetTickCount();
			break;
		}
		GSessionManager->DoPeriodJob();

		Sleep(10);
	}
}


void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;

	LIoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = GIocpManager->GetCompletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ULONG_PTR completionKey = 0;

		int ret = GetQueuedCompletionStatus(hComletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

		ClientSession* theClient = context ? context->mSessionObject : nullptr ;
		
		if (ret == 0 || dwTransferred == 0)
		{
			
			/// check time out first 
			if ( context == nullptr && GetLastError() == WAIT_TIMEOUT )
				continue;


			if ( context->mIoType == IO_RECV || context->mIoType == IO_SEND )
			{
				CRASH_ASSERT( nullptr != theClient );

				/// In most cases in here: ERROR_NETNAME_DELETED(64)

				theClient->DisconnectRequest( DR_COMPLETION_ERROR );

				DeleteIoContext( context );

				continue;
			}

			/*
			DWORD gle = GetLastError();

			/// check time out first 
			if (gle == WAIT_TIMEOUT)
				continue;
			if ( gle == WSA_INVALID_HANDLE || gle == ERROR_ABANDONED_WAIT_0 )
				break;
			

			if (context->mIoType == IO_RECV || context->mIoType == IO_SEND )
			{
				CRASH_ASSERT(nullptr != theClient);

				/// In most cases in here: ERROR_NETNAME_DELETED(64)

				theClient->DisconnectRequest(DR_COMPLETION_ERROR);

				DeleteIoContext(context);

				continue;
			}
			*/
		}

		CRASH_ASSERT(nullptr != theClient);
	
		bool completionOk = false;
		switch (context->mIoType)
		{
		case IO_DISCONNECT:
			theClient->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
			completionOk = true;
			break;

		case IO_CONNECT:
			if ( ret )
				theClient->ConnectCompletion();
			else
				theClient->ReleaseRef();
			completionOk = true;
			break;

		case IO_RECV_ZERO:
			completionOk = PreReceiveCompletion(theClient, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;

		case IO_SEND:
			completionOk = SendCompletion(theClient, static_cast<OverlappedSendContext*>(context), dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(theClient, static_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			CRASH_ASSERT(false);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			theClient->DisconnectRequest(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

bool IocpManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	/// real receive...
	return client->PostRecv();
}

bool IocpManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);
// 
// 	if ( context->mWsaBuf.buf[0] != (char)( client->GetClientId() % 10 + 48 ) )
// 		printf_s( "What the hell!!\n" );
	/// echo back
	//return client->PostSend();
	return client->PreRecv();
}

bool IocpManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->mWsaBuf.len != dwTransferred)
	{
		printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred) ;
		return false;
	}		
	return true;
}


