#include "stdafx.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	//TODO: mIoThreadCount = ...;GetSystemInfo����ؼ� set num of I/O threads

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	//TODO: mCompletionPort = CreateIoCompletionPort(...)
	
	/// create TCP socket
	//TODO: mListenSocket = ...
	
	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	//TODO:  bind
	//if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
	//	return false;

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		//TODO: HANDLE hThread = (HANDLE)_beginthreadex...);
	}

	return true;
}


bool IocpManager::StartAcceptLoop()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		return false;


	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// ���� ���� ����ü �Ҵ�� �ʱ�ȭ
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// Ŭ�� ���� ó��
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}

	return true;
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
	HANDLE hComletionPort = GIocpManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		int ret = 0; ///<���⿡�� GetQueuedCompletionStatus(hComletionPort, ..., GQCS_TIMEOUT)�� ������ ������� ����

		/// check time out first 
		if (ret == 0 && GetLastError()==WAIT_TIMEOUT)
			continue;

		if (ret == 0 || dwTransferred == 0)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		// TODO
		// if (nullptr == context) �� ��� ó��
		//{
		//}

		bool completionOk = true;
		switch (context->mIoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}

	}

	return 0;
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	//TODO
	/// echo back ó�� client->PostSend()���.
	
	delete context;

	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	//TODO
	/// ���� �� �Ǿ����� Ȯ���ϴ� �� ó��..
	//if (context->mWsaBuf.len != dwTransferred) {...}
	
	delete context;
	return true;
}
