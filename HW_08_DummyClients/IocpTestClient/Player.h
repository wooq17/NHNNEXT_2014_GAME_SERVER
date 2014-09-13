#pragma once

#include "stdafx.h"
#include "ContentsConfig.h"
#include "Packet.h"
class ClientSession;
struct PacketHeader;

class Player
{
public:
	Player(ClientSession* session);
	virtual ~Player();

	bool	IsLoaded() { return mPlayerId > 0; }
	int		GetPlayerId() { return mPlayerId; }	
	void	Start(int id);

	void	PlayerReset();	
		
	ClientSession* GetSession() { return mSession; }
	
private:
	FastSpinlock	mPlayerLock;
	int				mPlayerId;

	Float3D			mPos;
	int				mHealth;
	wchar_t			mPlayerName[MAX_NAME_LEN];
	wchar_t			mComment[MAX_COMMENT_LEN];

	ClientSession* const mSession;
	friend class ClientSession;
};