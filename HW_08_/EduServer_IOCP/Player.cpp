#include "stdafx.h"
#include "Timer.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerManager.h"
#include "PlayerWideEvent.h"
#include "GrandCentralExecuter.h"
#include "PlayerDBContext.h"
#include "DBManager.h"
#include "Packet.h"

#include "ClientSessionManager.h"
#include "MyPacket.pb.h"

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

	/// 플레이어 맵에서 제거
	// printf("player deleted\n");
	if ( mPlayerId != -1 )
		GPlayerManager->UnregisterPlayer(mPlayerId, mZoneIdx);

	memset( mPlayerName, 0, sizeof( mPlayerName ) );
	memset( mComment, 0, sizeof( mComment ) );
	mPlayerId = -1;
	mIsValid = false;
	mPos.m_X = mPos.m_Y = mPos.m_Z = 0.0f;
	mHeartBeat = -1;
	mIsAlive = false;
	mZoneIdx = -1;
}

void Player::Start(int id)
{
	mPlayerId = id;
	mIsAlive = true;

	UpdateZoneIdx();

	/// ID 발급 및 플레잉어 맵에 등록
	GPlayerManager->RegisterPlayer( GetSharedFromThis<Player>(), mPlayerId, mZoneIdx );

	/// 생명 불어넣기 ㄱㄱ
	// OnTick();
}

void Player::OnTick()
{
	if (!IsAlive())
		return;

	/// 랜덤으로 이벤트를 발생시켜보기 (예: 다른 모든 플레이어에게 버프 주기)
	if (rand() % 100 == 0) ///< 1% 확률
	{
		int buffId = mPlayerId * 100;
		int duration = (rand() % 3 + 2) * 1000;
	
		//GCE 예. (lock-order 귀찮고, 전역적으로 순서 보장 필요할 때)
		auto playerEvent = std::make_shared<AllPlayerBuffEvent>(buffId, duration);
		GCEDispatch(playerEvent, &AllPlayerBuffEvent::DoBuffToAllPlayers, mPlayerId);
	}
	
	// TODO: AllPlayerBuffDecay::CheckBuffTimeout를 GrandCentralExecuter를 통해 실행
	auto buffTimeoutEvent = std::make_shared<AllPlayerBuffDecay>();
	GCEDispatch( buffTimeoutEvent, &AllPlayerBuffDecay::CheckBuffTimeout );
	// WIP
	
	if (mHeartBeat > 0)
		DoSyncAfter(mHeartBeat, GetSharedFromThis<Player>(), &Player::OnTick);
}

void Player::AddBuff(int fromPlayerId, int buffId, int duration)
{
	printf_s("I am Buffed [%d]! from Player [%d]\n", buffId, fromPlayerId);

	/// 플레이어의 버프 리스트에 추가
	mBuffList.insert(std::make_pair(buffId, duration));
}

void Player::DecayTickBuff()
{
	/// 버프의 남은 시간을 주기적으로 수정하고, 시간이 종료되면 제거하기
	for (auto it = mBuffList.begin(); it != mBuffList.end();)
	{
		it->second -= mHeartBeat;

		if (it->second <= 0)
		{
			printf_s("Player [%d] BUFF [%d] expired\n", mPlayerId, it->first);
			mBuffList.erase(it++);
		}
		else
		{
			++it;
		}
	}
}


void Player::RequestLoad( int pid )
{
	LoadPlayerDataContext* context = new LoadPlayerDataContext( mSession, pid );
	GDatabaseManager->PostDatabsaseRequest( context );
}

void Player::ResponseLoad( int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment )
{
	FastSpinlockGuard criticalSection( mPlayerLock );

	mPlayerId = pid;
	mPos.m_X = x;
	mPos.m_Y = y;
	mPos.m_Z = z;
	mIsValid = valid;

	wcscpy_s( mPlayerName, name );
	wcscpy_s( mComment, comment );

	wprintf_s( L"PID[%d], X[%f] Y[%f] Z[%f] NAME[%s] COMMENT[%s]\n", mPlayerId, mPos.m_X, mPos.m_Y, mPos.m_Z, mPlayerName, mComment );
}

void Player::RequestUpdatePosition( float x, float y, float z )
{
	ResponseUpdatePosition( x, y, z );
	// 모든 움직임을 디비 업데이트는 좀 ㅋㅋㅋ
	/*
	//todo: DB에 플레이어 위치를 x,y,z로 업데이트 요청하기
	UpdatePlayerPositionContext* context = new UpdatePlayerPositionContext( mSession, mPlayerId );

	// 함수 하나 만들어서 쓸까
	context->mPosX = x;
	context->mPosY = y;
	context->mPosZ = z;

	// GDatabaseManager->PostDatabsaseRequest( context );
	// DONE
	*/
}

void Player::ResponseUpdatePosition( float x, float y, float z )
{
	FastSpinlockGuard criticalSection( mPlayerLock );
	mPos.m_X = x;
	mPos.m_Y = y;
	mPos.m_Z = z;

	// 존 변경 확인
	int prevZoneIdx = mZoneIdx;
	UpdateZoneIdx();

	if ( prevZoneIdx != mZoneIdx )
		GPlayerManager->ChangePlayerZone( prevZoneIdx, mZoneIdx, mPlayerId );

	// 전송해주기 
	// payload 생성
	MyPacket::MoveRequest moveResult;
	moveResult.set_playerid( mPlayerId );
	moveResult.mutable_playerpos()->set_x( x );
	moveResult.mutable_playerpos()->set_y( y );
	moveResult.mutable_playerpos()->set_z( z );


	// header 생성
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_SC_MOVE;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = moveResult.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// 조립	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = moveResult.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );

	// 전송
	mSession->PostSend( reinterpret_cast<char*>( encoded), packetHeader.mSize );
	delete[] encoded;

// 	MoveResponse sendPacket;
// 	sendPacket.mPlayerId = mPlayerId;
// 	sendPacket.mX = mPos.m_X;
// 	sendPacket.mY = mPos.m_Y;
// 	sendPacket.mZ = mPos.m_Z;

//	mSession->PostSend( reinterpret_cast<char*>( &sendPacket ), sendPacket.mSize );
}

void Player::RequestUpdateComment( const wchar_t* comment )
{
	UpdatePlayerCommentContext* context = new UpdatePlayerCommentContext( mSession, mPlayerId );
	context->SetNewComment( comment );
	GDatabaseManager->PostDatabsaseRequest( context );
}

void Player::ResponseUpdateComment( const wchar_t* comment )
{
	FastSpinlockGuard criticalSection( mPlayerLock );
	wcscpy_s( mComment, comment );
}

void Player::RequestUpdateValidation( bool isValid )
{
	UpdatePlayerValidContext* context = new UpdatePlayerValidContext( mSession, mPlayerId );
	context->mIsValid = isValid;
	GDatabaseManager->PostDatabsaseRequest( context );
}

void Player::ResponseUpdateValidation( bool isValid )
{
	FastSpinlockGuard criticalSection( mPlayerLock );
	mIsValid = isValid;
}


void Player::RequestRegisterPlayer( const wchar_t* newName )
{
	// for test without DB
	ResponseRegisterPlayer( GClientSessionManager->GetTempId() );
	wcscpy_s( mPlayerName, newName );

	//todo: DB스레드풀에 newName에 해당하는 플레이어 생성 작업을 수행시켜보기
	// CreatePlayerDataContext* context = new CreatePlayerDataContext( mSession );
	// wcscpy_s( context->mPlayerName, newName ); ///# 이런거는 accessor 하나 만들어서 깔끔하게..
	// GDatabaseManager->PostDatabsaseRequest( context );
	// DONE
}

void Player::ResponseRegisterPlayer( int id )
{
	Start( id );

	// payload 생성
	MyPacket::LoginResult loginResult;
	loginResult.set_playerid( mPlayerId );

	// header 생성
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_SC_LOGIN;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = loginResult.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// 조립	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = loginResult.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );
	
	// 전송
	mSession->PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize );
	
	delete[] encoded;
}

void Player::RequestDeregisterPlayer( int playerId )
{
	// for test without DB
	ResponseDeregisterPlayer( playerId );

	//todo: DB스레드풀에 playerId에 해당하는 플레이어 생성 삭제 작업을 수행시켜보기
	// DeletePlayerDataContext* context = new DeletePlayerDataContext( mSession, playerId );
	// GDatabaseManager->PostDatabsaseRequest( context );
	// DONE
}


void Player::ResponseDeregisterPlayer( int playerId )
{
	// payload 생성
	MyPacket::LogoutResult logoutResult;
	logoutResult.set_playerid( mPlayerId );

	// header 생성
	PacketHeader packetHeader;
	packetHeader.mType = MyPacket::PKT_SC_LOGOUT;
	size_t packetHeaderSize = sizeof( PacketHeader );
	size_t packetPayloadSize = logoutResult.ByteSize();
	packetHeader.mSize = packetHeaderSize + packetPayloadSize;

	// 조립	
	char* encoded = new char[packetHeader.mSize];
	memcpy( encoded, &packetHeader, packetHeaderSize );
	int encodingResult = logoutResult.SerializeToArray( encoded + BYTE( packetHeaderSize ), packetPayloadSize );

	// 전송
	mSession->PostSend( reinterpret_cast<const char*>( encoded ), packetHeader.mSize );

	delete[] encoded;
}

void Player::UpdateZoneIdx()
{
	// 십자가 모양으로 존의 크기가 다른 곳보다 크다 ㅋㅋㅋ
	int xIdx = static_cast<int>( mPos.m_X ) / ZONE_RANGE;
	xIdx = abs( xIdx );
	xIdx = min( xIdx, 4 );

	if ( mPos.m_X > 0 )
		xIdx = 5 + xIdx;
	else
		xIdx = 4 - xIdx;

	int yIdx = static_cast<int>( mPos.m_Y ) / ZONE_RANGE;
	yIdx = abs( yIdx );
	yIdx = min( yIdx, 4 );

	if ( mPos.m_Y > 0 )
		yIdx = 5 + yIdx;
	else
		yIdx = 4 - yIdx;

	mZoneIdx = yIdx * ZONE_NUMBER_EACH_AXIS + xIdx;
}