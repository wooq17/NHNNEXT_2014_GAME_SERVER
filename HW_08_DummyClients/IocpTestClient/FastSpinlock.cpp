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
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while ( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		if ( ( InterlockedAdd( &mLockFlag, LF_WRITE_FLAG ) & LF_WRITE_MASK ) == LF_WRITE_FLAG )
		{
			/// �ٸ����� readlock Ǯ���ٶ����� ��ٸ���.
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
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while ( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		//TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���?)
		//if ( !( InterlockedAdd( &mLockFlag, 1 ) & LF_WRITE_MASK ) )
		//	return;

		//InterlockedAdd( &mLockFlag, -1 );
		// WIP

		///# ������ �±� �´µ� �� �̷��� �ϸ� ����?
		if ( ( InterlockedIncrement( &mLockFlag ) & LF_WRITE_MASK ) == 0 )
			return;

		InterlockedDecrement( &mLockFlag );

	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag ó�� 
	// InterlockedAdd( &mLockFlag, -1 );
	InterlockedDecrement( &mLockFlag );
	///# ���������� InterlockedDecrement ����ϸ� ���� ����
	// WIP
}