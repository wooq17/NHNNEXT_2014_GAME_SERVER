//#define MAX_CONNECTION	10000
#define LISTEN_PORT 9001

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER
};

extern __declspec( thread ) int LThreadType;