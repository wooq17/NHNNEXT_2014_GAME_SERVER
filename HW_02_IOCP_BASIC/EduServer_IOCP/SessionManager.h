#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentConnectionCount(0)	{}

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);

	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }

	// ��� �������..
	//ClientSession* GetClientFromList(SOCKET sock) { return mClientList[sock]; }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	//TODO: mLock; ����
	FastSpinlock mLock;
	//done 
	//������� ����Ʈ�����Ͱ��� ��ü�� ���پ��� �Լ��� �־(fastspinlockguard)	
	//unlock �� ����� ���ɷ� ��ü�ϱ� ����..
	//classTypelock�̶�� �͵� �ִµ� �װ� ����ϳ�..

	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
