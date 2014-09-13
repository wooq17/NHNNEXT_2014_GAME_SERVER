#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "ThreadLocal.h"
#include "EduServer_IOCP.h"
#include "OverlappedIOContext.h"
#include "Player.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "ClientSessionManager.h"
#include "LockOrderChecker.h"
#include "Packet.h"

#define CLIENT_BUFSIZE	65536

ClientSession::ClientSession() : Session( CLIENT_BUFSIZE, CLIENT_BUFSIZE ), mPlayer( new Player( this ) )
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
}


void ClientSession::SessionReset()
{
	TRACE_THIS;

	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mRecvBuffer.BufferReset();

	mSendBufferLock.EnterWriteLock();
	mSendBuffer.BufferReset();
	mSendBufferLock.LeaveWriteLock();

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

	/// 플레이어 리셋
	mPlayer->DoSync(&Player::PlayerReset);
}

bool ClientSession::PostAccept()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);
	DWORD bytes = 0;
	// 조심해!
	// DWORD flags = 0; // 사용 안 함
	acceptContext->mWsaBuf.len = 0;
	acceptContext->mWsaBuf.buf = nullptr;

	if (FALSE == AcceptEx(*GIocpManager->GetListenSocket(), mSocket, GIocpManager->mAcceptBuf, 0,
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &bytes, (LPOVERLAPPED)acceptContext))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(acceptContext);
			printf_s("AcceptEx Error : %d\n", GetLastError());

			return false;
		}
	}

	return true;
}

void ClientSession::AcceptCompletion()
{
	TRACE_THIS;

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

		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
		if (handle != GIocpManager->GetComletionPort())
		{
			printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

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

	/// 타이머 테스트를 위해 10ms 후에 player 가동 ㄱㄱ
	// 로그인 성공하면 수행하도록 변경 
	// DoSyncAfter(10, mPlayer, &Player::Start, 1000);

	// 요놈의 위치는 원래 C_LOGIN 핸들링 할 때 해야하는거지만 지금은 접속 완료 시점에서 테스트 ㄱㄱ
	// static int id = 101;
	// mPlayer.TestCreatePlayerData( L"testName" );
	// mPlayer.RequestLoad( id++ );
	// mPlayer.TestDeletePlayerData( id );
	//
}

void ClientSession::OnDisconnect( DisconnectReason dr )
{
	TRACE_THIS;

	printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa( mClientAddr.sin_addr ), ntohs( mClientAddr.sin_port ) );
	// log and dump test
	// CRASH_ASSERT( false );
}

void ClientSession::OnRelease()
{
	TRACE_THIS;

	GClientSessionManager->ReturnClientSession( this );
}

void ClientSession::PacketHandling()
{
	TRACE_THIS;

	size_t len = mRecvBuffer.GetContiguiousBytes();

	if ( len == 0 )
		return;

	PacketHeader* recvPacket = reinterpret_cast<PacketHeader*>( mRecvBuffer.GetBufferStart() );

	// 맙소사 ㅋㅋㅋ
	// 일단 switch-case로 ㅋㅋㅋ
	switch ( recvPacket->mType )
	{
	case PKT_CS_LOGIN:
		// db에 로그인하고 
		{
			LoginRequest* clientPacket = reinterpret_cast<LoginRequest*>( recvPacket );
			mPlayer->RequestRegisterPlayer( clientPacket->mName );
			// 응답 보내기는 나중에 비동기로 수행
		}
		break;
	case PKT_CS_LOGOUT:
		// db에 로그아웃하고 
		{
			LogoutRequest* clientPacket = reinterpret_cast<LogoutRequest*>( recvPacket );
			if ( mPlayer->GetPlayerId() != clientPacket->mPlayerId )
				break;

			mPlayer->RequestDeregisterPlayer( clientPacket->mPlayerId );
		}
		break;
	case PKT_CS_CHAT:
		{
			ChatBroadcastRequest* clientPacket = reinterpret_cast<ChatBroadcastRequest*>( recvPacket );
			if ( mPlayer->GetPlayerId() != clientPacket->mPlayerId )
				break;

			ChatBroadcastResponse packet;

			packet.mPlayerId = clientPacket->mPlayerId;
			memcpy( packet.mName, mPlayer->GetName(), sizeof( packet.mName ) );
			memcpy( packet.mChat, clientPacket->mChat, sizeof( packet.mChat ) );

			GClientSessionManager->NearbyBroadcast( reinterpret_cast<char*>( &packet ), packet.mSize, mPlayer->GetZoneIdx() );
		}
		break;
	case PKT_CS_MOVE:
		{
			MoveRequest* clientPacket = reinterpret_cast<MoveRequest*>( recvPacket );
			if ( mPlayer->GetPlayerId() != clientPacket->mPlayerId )
				break;

			mPlayer->RequestUpdatePosition( clientPacket->mX, clientPacket->mY, clientPacket->mZ );
		}
		break;
	default:
		if ( false == PostSend( mRecvBuffer.GetBufferStart(), len ) )
			return;
		break;
	}

	mRecvBuffer.Remove( len );
}
