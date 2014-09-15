﻿#pragma once

// 일단 easy game server 코드 그대로 옮김
// 2014. 4. 15
// 최경욱

class DDCircularBuffer
{
public:
	DDCircularBuffer( size_t capacity );
	~DDCircularBuffer( );

	size_t GetCurrentSize( ) const
	{
		return mCurrentSize;
	}

	size_t GetCapacity( ) const
	{
		return mCapacity;
	}

	bool Write( const char* data, size_t bytes );

	bool Read( char* data, size_t bytes );

	/// 데이터 훔쳐보기 (제거하지 않고)
	void Peek( char* data );
	bool Peek( char* data, size_t bytes );

	/// 데이터 제거
	bool Consume( size_t bytes );

private:
	size_t mBeginIndex;
	size_t mEndIndex;
	size_t mCurrentSize;
	size_t mCapacity;

	char* mData;
};
