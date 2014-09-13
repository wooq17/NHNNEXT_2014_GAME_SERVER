/* 테스트용 임시 사용 */

#pragma once
#include "stdafx.h"

#define MAX_CHAT_LEN	1024
#define MAX_NAME_LEN	32

enum PacketTypes
{
	PKT_NONE = 0,

	// 로그인 및 기타 초반 접속에 관련된 패킷
	PKT_CS_LOGIN = 1,
	PKT_SC_LOGIN = 2,
	PKT_CS_LOGOUT = 3,
	PKT_SC_LOGOUT = 4,

	PKT_CS_CHAT = 10,
	PKT_SC_CHAT = 11,

	PKT_CS_MOVE = 20,
	PKT_SC_MOVE = 21,
};

#pragma pack(push, 1)

struct PacketHeader
{
	PacketHeader() : mSize( 0 ), mType( PKT_NONE ) 	{}
	unsigned short mSize;
	short mType;
};


// 로그인 요청
struct LoginRequest : public PacketHeader
{
	LoginRequest()
	{
		mSize = sizeof( LoginRequest );
		mType = PKT_CS_LOGIN;

		memset( mName, 0, MAX_NAME_LEN );
	}

	wchar_t mName[MAX_NAME_LEN];
};

struct LoginResponse : public PacketHeader
{
	LoginResponse()
	{
		mSize = sizeof( LoginResponse );
		mType = PKT_SC_LOGIN;
		mPlayerId = -1;
	}

	int mPlayerId;
};

struct LogoutRequest : public PacketHeader
{
	LogoutRequest()
	{
		mSize = sizeof( LogoutRequest );
		mType = PKT_CS_LOGOUT;
		mPlayerId = -1;
	}

	int mPlayerId;
};

struct LogoutResponse : public PacketHeader
{
	LogoutResponse()
	{
		mSize = sizeof( LogoutResponse );
		mType = PKT_SC_LOGOUT;
	}
};

struct ChatBroadcastRequest : public PacketHeader
{
	ChatBroadcastRequest()
	{
		mSize = sizeof( ChatBroadcastRequest );
		mType = PKT_CS_CHAT;
		mPlayerId = -1;

		memset( mChat, 0, MAX_CHAT_LEN );
	}

	int	mPlayerId;
	wchar_t mChat[MAX_CHAT_LEN];
};

struct ChatBroadcastResponse : public PacketHeader
{
	ChatBroadcastResponse()
	{
		mSize = sizeof( ChatBroadcastResponse );
		mType = PKT_SC_CHAT;
		mPlayerId = -1;

		memset( mName, 0, MAX_NAME_LEN );
		memset( mChat, 0, MAX_CHAT_LEN );
	}

	int	mPlayerId;
	wchar_t mName[MAX_NAME_LEN];
	wchar_t mChat[MAX_CHAT_LEN];
};

struct MoveRequest : public PacketHeader
{
	MoveRequest()
	{
		mSize = sizeof( MoveRequest );
		mType = PKT_CS_MOVE;
		mPlayerId = -1;

		mX = mY = mZ = 0.0f;
	}

	int mPlayerId;
	float mX;
	float mY;
	float mZ;
};

struct MoveResponse : public PacketHeader
{
	MoveResponse()
	{
		mSize = sizeof( MoveResponse );
		mType = PKT_SC_MOVE;
		mPlayerId = -1;

		mX = mY = mZ = 0.0f;
	}

	int mPlayerId;
	float mX;
	float mY;
	float mZ;
};