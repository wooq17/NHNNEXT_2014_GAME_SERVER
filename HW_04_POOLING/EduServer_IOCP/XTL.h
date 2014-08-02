#pragma once
#include "MemoryPool.h"
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <hash_set>
#include <hash_map>
#include <map>
#include <queue>

template <class T>
class STLAllocator
{
public:
	STLAllocator() = default;

	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	template <class U>
	STLAllocator(const STLAllocator<U>&)
	{}

	template <class U>
	struct rebind
	{
		typedef STLAllocator<U> other;
	};

	void construct(pointer p, const T& t)
	{
		new(p)T(t);
	}

	void destroy(pointer p)
	{
		p->~T();
	}

	T* allocate(size_t n)
	{
		//TODO: 메모리풀에서 할당해서 리턴
		return static_cast<T*>( GMemoryPool->Allocate( n * sizeof( T ) ) );
		// WIP
	}

	void deallocate(T* ptr, size_t n)
	{
		//TODO: 메모리풀에 반납
		// 현재 메모리 풀에서 allocate를 통해서 n개를 할당해도 n개를 포함하는 하나의 메모리 블럭을 할당
		// 해제는 해당 블럭만 해제하면 되므로 해당 블럭의 주소만 있으면 된다.
		// n은 extraInfo로 사용?
		GMemoryPool->Deallocate( ptr, n );
		// WIP
	}
};


template <class T>
struct xvector
{
	typedef std::vector<T, STLAllocator<T>> type;
};

template <class T>
struct xdeque
{
	//TODO: STL 할당자를 사용하는 deque를 type으로 선언
	typedef std::deque<T, STLAllocator<T>> type;
	// WIP
};

template <class T>
struct xlist
{
	//TODO: STL 할당자 사용
	typedef std::list<T, STLAllocator<T>> type;
	// WIP
};

template <class K, class T, class C = std::less<K> >
struct xmap
{
	//TODO: STL 할당자 사용하는 map을  type으로 선언
	typedef std::map<K, T, C, STLAllocator<std::pair<K, T>>> type;
	// WIP
};

template <class T, class C = std::less<T> >
struct xset
{
	//TODO: STL 할당자 사용하는 set을  type으로 선언
	typedef std::set<T, C, STLAllocator<T>> type;
	// WIP
};

template <class K, class T, class C = std::hash_compare<K, std::less<K>> >
struct xhash_map
{
	typedef std::hash_map<K, T, C, STLAllocator<std::pair<K, T>> > type;
};

template <class T, class C = std::hash_compare<T, std::less<T>> >
struct xhash_set
{
	typedef std::hash_set<T, C, STLAllocator<T> > type;
};

template <class T, class C = std::less<std::vector<T>::value_type> >
struct xpriority_queue
{
	//TODO: STL 할당자 사용하는 priority_queue을  type으로 선언
	typedef std::priority_queue<T, xvector<T>, C> type;
	// WIP
};

typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, STLAllocator<wchar_t>> xstring;

