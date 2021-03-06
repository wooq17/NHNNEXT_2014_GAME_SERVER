﻿#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	// TODO: mTimerJobQueue에 TimerJobElement를 push..
	//LTickCount = GetTickCount64();
	//mTimerJobQueue.push( TimerJobElement( owner, task, LTickCount + after ) );

	///# 이게 의도..
	int64_t dueTimeTick = after + LTickCount;
	mTimerJobQueue.push(TimerJobElement(owner, task, dueTimeTick));

	// WIP
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 
		// mTimerJobQueue.pop();

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		timerJobElem.mOwner->EnterLock();
		
		timerJobElem.mTask();

		timerJobElem.mOwner->LeaveLock();

		mTimerJobQueue.pop();
	}


}

