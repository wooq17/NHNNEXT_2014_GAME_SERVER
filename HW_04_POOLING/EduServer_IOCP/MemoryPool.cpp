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
	//TODO: InterlockedPopEntrySList�� �̿��Ͽ� mFreeList���� pop���� �޸𸮸� ������ �� �ִ��� Ȯ��. 
	// ��� ������ NULL ��ȯ
	MemAllocInfo* mem = static_cast<MemAllocInfo*>( InterlockedPopEntrySList( &mFreeList ) );
	// WIP

	if (NULL == mem)
	{
		// �Ҵ� �Ұ����ϸ� ���� �Ҵ�.
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
	//TODO: InterlockedPushEntrySList�� �̿��Ͽ� �޸�Ǯ�� (������ ����) �ݳ�.
	// �Ҵ� �Ұ����ؼ� Ǯ ������ �Ҵ����� �ʰ� ���� �Ҵ��� ��� �ݳ��Ǵ� �ش� ���� Ǯ�� ���� ���Ѽ� ��� Ȱ��
	// �̹� ���� �Ҵ��ߴٴ� ���� Ǯ�� ����� ���� ���麸�� ��뷮�� �� ���ٴ� �ǹ�
	// ���� ���⼭ �����ϸ� ���������� ������ �Ҵ��� Ȯ���� ����

	// ���ʿ� Ǯ�� ���� �� �� ���� �޸𸮸� �Ҵ�޴� ���� �ƴ϶� ������ ���� ���� �Ҵ��ϰ� ��Ȱ���ϴ� ������� �۵� ��
	
	MemAllocInfo* prevEntry = static_cast<MemAllocInfo*>( InterlockedPushEntrySList( &mFreeList, ptr ) );
	if ( NULL == prevEntry )
	{
		// ���� ����ִ� ���¿��ٸ� ��ȯ ���� NULL
		// �� ó�� ��ȯ �Ǹ� ����� ���ٵ�... �� �ؾ� �ϳ�?
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

	//TODO: [2048, 4096] ���� ������ 256����Ʈ ������ SmallSizeMemoryPool�� �Ҵ��ϰ� 
	//TODO: mSmallSizeMemoryPoolTable�� O(1) access�� �����ϵ��� SmallSizeMemoryPool�� �ּ� ���
	for ( int i = 2048; i < MAX_ALLOC_SIZE; i += 256 )
	{
		// recent���� ũ�� i ������ ��û ������� i ������ Ǯ���� ó��
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
		//TODO: SmallSizeMemoryPool���� �Ҵ�
		//header = ...; 
		// size�� 0�� ��쿡�� �׳� nullptr ��ȯ�ϸ� �Ƿ���
		// �� �׷��� �׳� ����� ���� �����ϴ� �� �ϳ� ����ϰ� �Ǵµ�...
		// ã�ƺ��ϱ� c++���� 0 size new()�� ��� valid pointer�� ��ȯ�Ѵٰ� ��
		// �׷��ϱ� �׳� �ΰ� �ּ� �� ũ�� �Ҵ� �� ��ȯ
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop( );
		// WIP
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< �ֱ� �Ҵ翡 ���õ� ���� ��Ʈ
	
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< �ι� ���� üũ ����
	
	CRASH_ASSERT(realAllocSize > 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool�� (������ ����) push..
		mSmallSizeMemoryPoolTable[realAllocSize]->Push( header );
		// WIP
	}
}