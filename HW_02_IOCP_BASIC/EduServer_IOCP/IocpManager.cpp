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
	//TODO: mIoThreadCount = ...;GetSystemInfo사용해서 set num of I/O threads	
	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );
	mIoThreadCount = systemInfo.dwNumberOfProcessors * 2;	
	//DONE
	//쓰레드는 2배수로 만듦

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	//TODO: mCompletionPort = CreateIoCompletionPort(...)
	mCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	//DONE
	
	/// create TCP socket
	//TODO: mListenSocket = ...	
	mListenSocket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	//DONE

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	//TODO:  bind
	//if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
	//	return false;
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );
	serveraddr.sin_port = htons( atoi( "9001" ) );
	
	if ( SOCKET_ERROR == bind( mListenSocket, (SOCKADDR*)&serveraddr, sizeof( serveraddr ) ) )
	{
		return false;
	}
	//DONE


	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		//TODO: HANDLE hThread = (HANDLE)_beginthreadex...);
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, mCompletionPort, 0, (unsigned*)&dwThreadId );
		CloseHandle( hThread );
		//DONE
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

		/// 소켓 정보 구조체 할당과 초기화
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// 클라 접속 처리
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

		//TODO
		//int ret = 0; ///<여기에는 GetQueuedCompletionStatus(hComletionPort, ..., GQCS_TIMEOUT)를 수행한 결과값을 대입
		DWORD sock;
		//int ret = GetQueuedCompletionStatus( hComletionPort, &dwTransferred, (LPDWORD)&asCompletionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT );
		int ret = GetQueuedCompletionStatus( hComletionPort, &dwTransferred, &sock, (LPOVERLAPPED*)&context, INFINITE );
		asCompletionKey = GSessionManager->GetClientFromList( (SOCKET)sock );
		//DONE

		/// check time out first 
		if ( ret == 0 && GetLastError() == WAIT_TIMEOUT )
			continue;
		else
			printf( "lala" );

		if (ret == 0 || dwTransferred == 0)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		// TODO
		// if (nullptr == context) 인 경우 처리
		//{
		//}
		if ( nullptr == context )
		{
			asCompletionKey->Disconnect( DR_NONE ); //머라 처리해야할지..
			GSessionManager->DeleteClientSession( asCompletionKey );
			continue;
		}
		//DONE

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
	/// echo back 처리 client->PostSend()사용.
	client->PostSend( context->mWsaBuf.buf, dwTransferred );
	//DONE
	//여기서 context->mWsaBuf.len은 비어있을 거임.
	//받는걸 buf로 받겠다고는 했지만 받겠다고 선언한 시점에 얼마나 받았는지는 모르니
	//send와 같은 구조체를 사용하지않는 이유는.. 정확히는 모르겠으나 같으면 안된다고함..
	//(동시에 일어날 경우를 대비해서 그러는 건지..)
	//http://gpgstudy.com/forum/viewtopic.php?p=124336
	
	delete context;

	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	//TODO
	/// 전송 다 되었는지 확인하는 것 처리..
	//if (context->mWsaBuf.len != dwTransferred) {...}
	if ( context->mWsaBuf.len != dwTransferred )
	{
		return false;
	}
	//DONE
	
	delete context;
	return true;
}
