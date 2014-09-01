#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "DBContext.h"
#include "DBThread.h"
#include "IocpManager.h"

DBThread::DBThread(HANDLE hThread, HANDLE hCompletionPort)
: mDbThreadHandle(hThread), mDbCompletionPort(hCompletionPort)
{}

DBThread::~DBThread()
{
	CloseHandle(mDbThreadHandle);
}

DWORD DBThread::Run()
{
	while (true)
	{
		DoDatabaseJob();
	}

	return 1;
}

void DBThread::DoDatabaseJob()
{
	DWORD dwTransferred = 0;
	LPOVERLAPPED overlapped = nullptr;
	ULONG_PTR completionKey = 0;

	int ret = GetQueuedCompletionStatus(mDbCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, &overlapped, INFINITE);

	if (CK_DB_REQUEST != completionKey)
	{
		CRASH_ASSERT(false);
		return;
	}

	
	DatabaseJobContext* dbContext = reinterpret_cast<DatabaseJobContext*>(overlapped);

	//todo: dbContext의 SQL을 실행하고 그 결과를 IO thread풀로 보내기
	dbContext->mSuccess = dbContext->SQLExecute();
	ret = PostQueuedCompletionStatus( GIocpManager->GetComletionPort(), 0, (ULONG_PTR)CK_DB_RESULT, (LPOVERLAPPED)dbContext );

	if ( !ret )
	{
		printf( "DoDatabaseJob failed : %d\n", GetLastError( ) );

		delete dbContext;

		CRASH_ASSERT( false );
	}
	// DONE


	///# 이거 왜 안씀? 코드를 필요한 부분만 보지말고 전체적으로 보는 습관을
	/// GIocpManager->PostDatabaseResult(dbContext);
}
