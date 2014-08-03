#pragma once


#include "Exception.h"
#include "FastSpinlock.h"


template <class TOBJECT, int ALLOC_COUNT = 100>
class ObjectPool
{
public:

	static void* operator new(size_t objSize)
	{
		//TODO: TOBJECT 타입 단위로 lock 잠금
		FastSpinlockGuard criticalSection( mObjectLock );
		// WIP

		if (!mFreeList)
		{
			mFreeList = new uint8_t[sizeof(TOBJECT)*ALLOC_COUNT];

			uint8_t* pNext = mFreeList;
			uint8_t** ppCurr = reinterpret_cast<uint8_t**>(mFreeList);

			for (int i = 0; i < ALLOC_COUNT - 1; ++i)
			{
				
				pNext += sizeof(TOBJECT);
				*ppCurr = pNext;
				ppCurr = reinterpret_cast<uint8_t**>(pNext);
			}

			// 조심해!
			// 처음 mFreeList가 nullptr일 때 만들고, 그 다음에 모자라서 추가로 더 만들려고 하면 마지막을 nullptr로 만들어놔야 
			*ppCurr = nullptr;
			mTotalAllocCount += ALLOC_COUNT;
		}

		uint8_t* pAvailable = mFreeList;
		mFreeList = *reinterpret_cast<uint8_t**>(pAvailable);
		++mCurrentUseCount;

		return pAvailable;
	}

	static void	operator delete(void* obj)
	{
		//TODO: TOBJECT 타입 단위로 lock 잠금
		FastSpinlockGuard criticalSection( mObjectLock );
		// WIP

		CRASH_ASSERT(mCurrentUseCount > 0);

		--mCurrentUseCount;

		*reinterpret_cast<uint8_t**>(obj) = mFreeList;
		mFreeList = static_cast<uint8_t*>(obj);
	}


private:
	static uint8_t*	mFreeList;
	static int		mTotalAllocCount; ///< for tracing
	static int		mCurrentUseCount; ///< for tracing

	static FastSpinlock	mObjectLock;
};


template <class TOBJECT, int ALLOC_COUNT>
uint8_t* ObjectPool<TOBJECT, ALLOC_COUNT>::mFreeList = nullptr;

template <class TOBJECT, int ALLOC_COUNT>
int ObjectPool<TOBJECT, ALLOC_COUNT>::mTotalAllocCount = 0;

template <class TOBJECT, int ALLOC_COUNT>
int ObjectPool<TOBJECT, ALLOC_COUNT>::mCurrentUseCount = 0;

template <class TOBJECT, int ALLOC_COUNT>
FastSpinlock ObjectPool<TOBJECT, ALLOC_COUNT>::mObjectLock;