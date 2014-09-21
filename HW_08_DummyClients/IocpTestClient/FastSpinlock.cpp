#include "stdafx.h"
#include "Exception.h"
#include "FastSpinlock.h"

FastSpinlock::FastSpinlock( const int lockOrder ) : mLockFlag( 0 )
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	while ( true )
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while ( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		if ( ( InterlockedAdd( &mLockFlag, LF_WRITE_FLAG ) & LF_WRITE_MASK ) == LF_WRITE_FLAG )
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			while ( mLockFlag & LF_READ_MASK )
				YieldProcessor();

			return;
		}

		InterlockedAdd( &mLockFlag, -LF_WRITE_FLAG );
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd( &mLockFlag, -LF_WRITE_FLAG );
}

void FastSpinlock::EnterReadLock()
{
	while ( true )
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while ( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)
		//if ( !( InterlockedAdd( &mLockFlag, 1 ) & LF_WRITE_MASK ) )
		//	return;

		//InterlockedAdd( &mLockFlag, -1 );
		// WIP

		///# 위에도 맞긴 맞는데 걍 이렇게 하면 되지?
		if ( ( InterlockedIncrement( &mLockFlag ) & LF_WRITE_MASK ) == 0 )
			return;

		InterlockedDecrement( &mLockFlag );

	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag 처리 
	// InterlockedAdd( &mLockFlag, -1 );
	InterlockedDecrement( &mLockFlag );
	///# 마찬가지로 InterlockedDecrement 사용하면 보기 좋지
	// WIP
}