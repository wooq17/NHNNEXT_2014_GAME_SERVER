#include "stdafx.h"
#include "Log.h"

#include <iostream>

void ThreadCallHistory::DumpOut(std::ostream& ost)
{
	//todo: 현재 스레드의 call history를 ost 스트림에 쓰기
	uint64_t count = mCounter < MAX_HISTORY ? mCounter : MAX_HISTORY;

	ost << "===== Call History [Thread:" << mThreadId << "]" << std::endl;

	for ( int i = 1; i <= count; ++i )
	{
		ost << "  FUNC:" << mHistory[( mCounter - i ) % MAX_HISTORY] << std::endl;
	}
	ost << "===== End of Call History" << std::endl << std::endl;
	// DONE
}
	

void ThreadCallElapsedRecord::DumpOut(std::ostream& ost)
{
	uint64_t count = mCounter < MAX_ELAPSED_RECORD ? mCounter : MAX_ELAPSED_RECORD;

	ost << "===== Recent Call Performance [Thread:" << mThreadId << "]" << std::endl;

	for (int i = 1; i <= count; ++i)
	{
		ost << "  FUNC:" << mElapsedFuncSig[(mCounter - i) % MAX_ELAPSED_RECORD] 
			<< "ELAPSED: " << mElapsedTime[(mCounter - i) % MAX_ELAPSED_RECORD] << std::endl;
	}
	ost << "===== End of Recent Call Performance" << std::endl << std::endl;

}


namespace LoggerUtil
{
	LogEvent gLogEvents[MAX_LOG_SIZE];
	__int64 gCurrentLogIndex = 0;

	void EventLogDumpOut(std::ostream& ost)
	{
		//todo: gLogEvents내용 ost 스트림에 쓰기
		// lock 안 잡아도 되나 - 다른 스레드들은 정지합니다?
		uint64_t count = gCurrentLogIndex < MAX_LOG_SIZE ? gCurrentLogIndex : MAX_LOG_SIZE;

		ost << "===== Event History" << std::endl;

		for ( int i = 1; i <= count; ++i )
		{
			ost << "[Thread:" << gLogEvents[( gCurrentLogIndex - i ) % MAX_LOG_SIZE].mThreadId << "]"
				<< "  MESSAGE :" << gLogEvents[( gCurrentLogIndex - i ) % MAX_LOG_SIZE].mMessage
				<< "  INFO :" << gLogEvents[( gCurrentLogIndex - i ) % MAX_LOG_SIZE].mAdditionalInfo << std::endl;
		}
		ost << "===== End of Event History" << std::endl << std::endl;
		// DONE
	}
}
