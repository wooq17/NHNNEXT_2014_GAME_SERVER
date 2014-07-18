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

	// 없어서 만들었음..
	//ClientSession* GetClientFromList(SOCKET sock) { return mClientList[sock]; }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	//TODO: mLock; 선언
	FastSpinlock mLock;
	//done 
	//형말대로 스마트포인터같이 객체로 갖다쓰는 함수가 있어서(fastspinlockguard)	
	//unlock 다 지우고 저걸로 대체하긴 했음..
	//classTypelock이라는 것도 있는데 그걸 써야하나..

	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
