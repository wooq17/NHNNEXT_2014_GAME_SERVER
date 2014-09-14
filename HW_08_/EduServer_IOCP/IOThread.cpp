﻿#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "EduServer_IOCP.h"
#include "IOThread.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "IocpManager.h"
#include "DBContext.h"

IOThread::IOThread(HANDLE hThread, HANDLE hCompletionPort) : mThreadHandle(hThread), mCompletionPort(hCompletionPort)
{
}


IOThread::~IOThread()
{
	CloseHandle(mThreadHandle);
}

DWORD IOThread::Run()
{

	while (true)
	{
		LTimer->DoTimerJob();

		DoIocpJob();

		DoSendJob(); ///< aggregated sends

		//... ...
	}

	return 1;
}

void IOThread::DoIocpJob()
{
	DWORD dwTransferred = 0;
	LPOVERLAPPED overlapped = nullptr;
	
	ULONG_PTR completionKey = 0;

	int ret = GetQueuedCompletionStatus(mCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, &overlapped, GQCS_TIMEOUT);

	if (CK_DB_RESULT == completionKey)
	{
		//todo: DB 처리 결과가 담겨오는 경우 처리
		DatabaseJobContext* dbContext = reinterpret_cast<DatabaseJobContext*>(overlapped);
		dbContext->OnResult();

		delete dbContext;

		return;
		// DONE
	}

	/// 아래로는 일반적인 I/O 처리

	OverlappedIOContext* context = reinterpret_cast<OverlappedIOContext*>(overlapped);
	
	Session* remote = context ? context->mSessionObject : nullptr;

	if (ret == 0 || dwTransferred == 0)
	{
		/// check time out first 
		//if ( context == nullptr && GetLastError() == WAIT_TIMEOUT)
		//	return;
		int gle = GetLastError();

		/// check time out first 
		if ( gle == WAIT_TIMEOUT )
		{
			// 조심해!
			// 3주차 과제 피드백에서 sm9 가라사대
			///# timeout이라고 무조건 continue시키면 안된다, timeout인데 ret==true이고 context에 데이터가 넘어오는 경우는 어떻게?
			// 라고 하시니 그 경우에는 context를 확인할 수 있도록 한다 (안 그러면 GQCS_TIMEOUT가 INFINITE 아닐 경우 accept를 못 하는 상황 발생)
			if ( ret == 0 || context == nullptr )
				return;
		}
	
		if (context->mIoType == IO_RECV || context->mIoType == IO_SEND)
		{
			CRASH_ASSERT(nullptr != remote);

			/// In most cases in here: ERROR_NETNAME_DELETED(64)

			remote->DisconnectRequest(DR_COMPLETION_ERROR);

			DeleteIoContext(context);

			return;
		}
	}

	CRASH_ASSERT(nullptr != remote);

	bool completionOk = false;
	switch (context->mIoType)
	{
	case IO_CONNECT:
		dynamic_cast<ServerSession*>(remote)->ConnectCompletion();
		completionOk = true;
		break;

	case IO_DISCONNECT:
		remote->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
		completionOk = true;
		break;

	case IO_ACCEPT:
		dynamic_cast<ClientSession*>(remote)->AcceptCompletion();
		completionOk = true;
		break;

	case IO_RECV_ZERO:
		completionOk = remote->PostRecv();
		break;

	case IO_SEND:
		remote->SendCompletion(dwTransferred);

		if (context->mWsaBuf.len != dwTransferred)
			printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred);
		else
			completionOk = true;
		
		break;

	case IO_RECV:
		remote->RecvCompletion(dwTransferred);
	
		/// for test
		// 여기서 패킷 종류에 따라서 따로 처리
		// remote->EchoBack();
		// completionOk = remote->PreRecv();
		//static_cast<ClientSession*>(remote)->PacketHandling();
		
		while ( !static_cast<ClientSession*>( remote )->PacketHandling() )
		{

		}
		
		completionOk = remote->PreRecv();

		break;

	default:
		printf_s("Unknown I/O Type: %d\n", context->mIoType);
		CRASH_ASSERT(false);
		break;
	}

	if (!completionOk)
	{
		/// connection closing
		remote->DisconnectRequest(DR_IO_REQUEST_ERROR);
	}

	DeleteIoContext(context);
	
}


void IOThread::DoSendJob()
{
	while (!LSendRequestSessionList->empty())
	{
		auto& session = LSendRequestSessionList->front();
	
		if (session->FlushSend())
		{
			/// true 리턴 되면 빼버린다.
			LSendRequestSessionList->pop_front();
		}
	}
	
}

