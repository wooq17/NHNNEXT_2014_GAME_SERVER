#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"

MemoryPool* GMemoryPool = nullptr;


SmallSizeMemoryPool::SmallSizeMemoryPool(DWORD allocSize) : mAllocSize(allocSize)
{
	CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
	InitializeSListHead(&mFreeList);
}

MemAllocInfo* SmallSizeMemoryPool::Pop()
{
	//TODO: InterlockedPopEntrySList를 이용하여 mFreeList에서 pop으로 메모리를 가져올 수 있는지 확인. 
	// 비어 있으면 NULL 반환
	MemAllocInfo* mem = static_cast<MemAllocInfo*>( InterlockedPopEntrySList( &mFreeList ) );
	// WIP

	if (NULL == mem)
	{
		// 할당 불가능하면 직접 할당.
		mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		CRASH_ASSERT(mem->mAllocSize == 0);
	}

	InterlockedIncrement(&mAllocCount);
	return mem;
}

void SmallSizeMemoryPool::Push(MemAllocInfo* ptr)
{
	//TODO: InterlockedPushEntrySList를 이용하여 메모리풀에 (재사용을 위해) 반납.
	// 할당 불가능해서 풀 내에서 할당하지 않고 직접 할당한 경우 반납되는 해당 블럭은 풀에 포함 시켜서 계속 활용
	// 이미 직접 할당했다는 것은 풀에 만들어 놓은 블럭들보다 사용량이 더 많다는 의미
	// 만약 여기서 해제하면 지속적으로 직접할 할당할 확률이 높음

	// 애초에 풀을 만들 때 한 번에 메모리를 할당받는 것이 아니라 부족한 블럭을 직접 할당하고 재활용하는 방식으로 작동 중
	
	MemAllocInfo* prevEntry = static_cast<MemAllocInfo*>( InterlockedPushEntrySList( &mFreeList, ptr ) );
	if ( NULL == prevEntry )
	{
		// 만약 비어있는 상태였다면 반환 값은 NULL
		// 맨 처음 반환 되면 여기로 올텐데... 뭘 해야 하나?
	}

	// WIP

	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	for (int i = 32; i < 1024; i += 32)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent+1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	for (int i = 1024; i < 2048; i += 128)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent + 1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	//TODO: [2048, 4096] 범위 내에서 256바이트 단위로 SmallSizeMemoryPool을 할당하고 
	//TODO: mSmallSizeMemoryPoolTable에 O(1) access가 가능하도록 SmallSizeMemoryPool의 주소 기록
	for ( int i = 2048; i < MAX_ALLOC_SIZE; i += 256 )
	{
		// recent보다 크고 i 이하인 요청 사이즈는 i 사이즈 풀에서 처리
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool( i );
		for ( int j = recent + 1; j <= i; ++j )
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}
	// WIP
}

void* MemoryPool::Allocate(int size)
{
	MemAllocInfo* header = nullptr;
	int realAllocSize = size + sizeof(MemAllocInfo);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		header = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(realAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//TODO: SmallSizeMemoryPool에서 할당
		//header = ...; 
		// size가 0인 경우에는 그냥 nullptr 반환하면 되려나
		// 안 그러면 그냥 헤더만 만들어서 저장하는 블럭 하나 사용하게 되는데...
		// 찾아보니까 c++에서 0 size new()의 경우 valid pointer를 반환한다고 함
		// 그러니까 그냥 두고 최소 블럭 크기 할당 후 반환
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop( );
		// WIP
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< 최근 할당에 관련된 정보 힌트
	
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< 두번 해제 체크 위해
	
	CRASH_ASSERT(realAllocSize > 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool에 (재사용을 위해) push..
		mSmallSizeMemoryPoolTable[realAllocSize]->Push( header );
		// WIP
	}
}