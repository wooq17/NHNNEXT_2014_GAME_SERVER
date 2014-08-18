#pragma once

#include "TypeTraits.h"
#include "FastSpinlock.h"
#include "Timer.h"


class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable() : mLock(LO_BUSINESS_CLASS)
	{}
	virtual ~SyncExecutable() {}

	template <class R, class T, class... Args>
	R DoSync(R (T::*memfunc)(Args...), Args... args)
	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

		// TODO: mLock으로 보호한 상태에서, memfunc를 실행하고 결과값 R을 리턴
		FastSpinlockGuard exclusive( mLock );

		//return std::bind( memfunc, static_cast<T*>( this ), args... )();
		///# 바로 실행할건데 굳이 바인드로 묶을 이유 없다.
		return (static_cast<T*>(this)->*memfunc)(args...);
		// WIP
	}
	

	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }
	
 	template <class T>
	std::shared_ptr<T> GetSharedFromThis()
 	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");
 		
		//TODO: this 포인터를 std::shared_ptr<T>형태로 반환.
		//(HINT: 이 클래스는 std::enable_shared_from_this에서 상속받았다. 그리고 static_pointer_cast 사용)

		return std::static_pointer_cast<T>( this->shared_from_this() );
		// WIP
 	}

private:

	FastSpinlock mLock;
};


template <class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible<T, std::shared_ptr<SyncExecutable>>::value, "T should be shared_ptr SyncExecutable");

	// TODO: instance의 memfunc를 bind로 묶어서 LTimer->PushTimerJob() 수행
	///# std::forward<Args>(args)...사용하면 약간 더 효율적
	LTimer->PushTimerJob( instance, std::bind( memfunc, instance, args... ), after ); 

	///# 이게 정석: LTimer->PushTimerJob(std::static_pointer_cast<SyncExecutable>(instance), bind, after);
	// WIP
}