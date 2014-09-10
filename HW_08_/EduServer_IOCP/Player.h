#pragma once

#include "ContentsConfig.h"
#include "SyncExecutable.h"
class ClientSession;


class Player : public SyncExecutable
{
public:
	Player(ClientSession* session);
	virtual ~Player();

	bool IsLoaded() { return mPlayerId > 0; }

	void RequestLoad( int pid );
	void ResponseLoad( int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment );

	void RequestUpdatePosition( float x, float y, float z );
	void ResponseUpdatePosition( float x, float y, float z );

	void RequestUpdateComment( const wchar_t* comment );
	void ResponseUpdateComment( const wchar_t* comment );

	void RequestUpdateValidation( bool isValid );
	void ResponseUpdateValidation( bool isValid );

	int GetPlayerId() { return mPlayerId; }
	bool IsAlive() { return mIsAlive;  }
	void Start(int heartbeat);

	void OnTick(); ///< �α����� 1�ʸ��� �Ҹ��� ���

	void PlayerReset();

	/// �÷��̾�� ������ �ɾ��ִ� �Լ���� ġ��.
	void AddBuff(int fromPlayerId, int buffId, int duration);

	/// �ֱ������� ���� �ð� ������Ʈ�ϴ� �Լ�
	void DecayTickBuff();

private:

	void TestCreatePlayerData( const wchar_t* newName );
	void TestDeletePlayerData( int playerId );

	FastSpinlock mPlayerLock;
	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	bool	mIsValid;
	wchar_t	mPlayerName[MAX_NAME_LEN];
	wchar_t	mComment[MAX_COMMENT_LEN];
	int		mHeartBeat;
	bool	mIsAlive;

	/// ���� ����Ʈ�� lock���� GCE�� �غ���
	std::map<int, int> mBuffList; ///< (id, time)

	ClientSession* const mSession;
	friend class ClientSession;
};