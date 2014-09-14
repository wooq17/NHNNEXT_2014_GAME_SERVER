#include "stdafx.h"
#include "ClientSession.h"
#include "Player.h"
#include "Packet.h"

Player::Player(ClientSession* session) : mSession(session)
{
	PlayerReset();
}

Player::~Player()
{
	CRASH_ASSERT(false);
}

void Player::PlayerReset()
{
	FastSpinlockGuard criticalSection( mPlayerLock );
	
	memset( mPlayerName, 0, sizeof( mPlayerName ) );	
	memset( mComment, 0, sizeof( mComment ) );
	mPlayerId = -1;
	mHealth = 10;
	mPos.m_X = mPos.m_Y = mPos.m_Z = 0.0f;	
}

void Player::Start(int id)
{
	mPlayerId = id;
}

bool Player::SendLogin( wchar_t* name )
{
	LoginRequest loginRequest;
	wcscpy_s( loginRequest.mName, name );

	if ( !mSession->WritePacket( &loginRequest ) )
	{
		return false;
	}
	mSession->PostSend();
	return true;
}

void Player::SendLogout()
{
	LogoutRequest logoutRequest;
	logoutRequest.mPlayerId = mPlayerId;

	if ( mSession->WritePacket( &logoutRequest ) )
	{
		mSession->PostSend();
	}
}

void Player::SendMove( Float3D position )
{
	MoveRequest moveRequest;
	moveRequest.mPlayerId = mPlayerId;
	moveRequest.mX = position.m_X;
	moveRequest.mY = position.m_Y;
	moveRequest.mZ = position.m_Z;

	if ( mSession->WritePacket( &moveRequest ) )
	{
		mSession->PostSend();
	}
}

void Player::SendChat( wchar_t* chatString )
{
	ChatBroadcastRequest chatRequest;
	chatRequest.mPlayerId = mPlayerId;
	wcscpy_s( chatRequest.mChat, chatString );

	if ( mSession->WritePacket( &chatRequest ) )
	{
		mSession->PostSend();
	}
}
