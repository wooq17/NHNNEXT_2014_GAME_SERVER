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
	bool	IsAlive() { return mHealth > 0; }
	int		GetPlayerId() { return mPlayerId; }	
	int		GetPlayerHealth() { return mHealth; }
	void	Start(int id);

	void	PlayerReset();	
	void	SetName( wchar_t* name ) { wcscpy_s( mPlayerName, name ); }
	const wchar_t* GetName() const { return mPlayerName; }
	void	SetPosition( Float3D pos ) { mPos = pos; }
	void	SetPosition( float x, float y, float z ) { mPos = Float3D( x, y, z ); }

	bool	SendLogin(wchar_t* name);
	bool	SendLogout();
	bool	SendMove( Float3D position );
	bool	SendChat( wchar_t* chatString );
		
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