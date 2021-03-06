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
	mHealth = 100;
	mPos.m_X = mPos.m_Y = mPos.m_Z = 0.0f;	
}

void Player::Start(int id)
{
	mPlayerId = id;
}


void Player::SetName( wchar_t* name ) 
{
	FastSpinlockGuard criticalSection( mPlayerLock );

	wcscpy_s( mPlayerName, name ); 
}

void	Player::SetPosition( Float3D pos ) 
{
	FastSpinlockGuard criticalSection( mPlayerLock );

	mPos = pos; 
}

void Player::SetPosition( float x, float y, float z ) 
{
	FastSpinlockGuard criticalSection( mPlayerLock );

	mPos = Float3D( x, y, z ); 
}