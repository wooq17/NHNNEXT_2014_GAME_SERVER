#pragma once
#include "FastSpinlock.h"

class ClientSession;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedConnectContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize( int time );
	void Finalize();

	bool StartIoThreads();
	void StartConnect();

	void IncreaseReadBytes( DWORD readBytes );
	void IncreaseWriteBytes( DWORD writtenBytes );

	long long GetTotalByteWritten() const { return mTotalByteWritten; }
	long long GetTotalByteRead() const { return mTotalByteRead; }

	HANDLE GetCompletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount;  }

	SOCKET* GetListenSocket()  { return &mListenSocket;  }

	static char mAcceptBuf[64];

private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:
	DWORD	mTime;
	DWORD	mProcessStartTime;
	long long	mTotalByteWritten;
	long long	mTotalByteRead;
	
	
	FastSpinlock mByteCounterLock;

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	SOCKET	mListenSocket;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;

