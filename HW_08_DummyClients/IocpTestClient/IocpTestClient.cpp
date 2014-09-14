// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"
#include "IocpTestClient.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"
#include "RSA.h"

__declspec( thread ) int LThreadType = -1;


int _tmain( int argc, _TCHAR* argv[] )
{
	LThreadType = THREAD_MAIN;

	if ( argc != 5 )
	{
		fprintf_s( stderr, "Usage : %s <IP> <PORT> <NUMBER OF CONNECTION> <TIME>" );
		exit( 1 );
	}

	/// for dump on crash
	SetUnhandledExceptionFilter( ExceptionFilter );

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;
	RSA::Init();

	GSessionManager->Initialize( argv[1], _wtoi( argv[2] ), _wtoi( argv[3] ) );

	if ( false == GIocpManager->Initialize( _wtoi( argv[4] ) ) )
		return -1;

	if ( false == GIocpManager->StartIoThreads() )
		return -1;


	wprintf_s( L"Start Client\n" );
	wprintf_s( L"Host\t\t: %s\n", argv[1] );
	wprintf_s( L"Port\t\t: %s\n", argv[2] );
	wprintf_s( L"Sessions\t: %s\n", argv[3] );
	wprintf_s( L"BufferSize\t: %d\n", SEND_BUFF );

	GIocpManager->StartConnect(); ///< block here...

	GIocpManager->Finalize();
	RSA::ExceptionHandling();

	wprintf_s( L"Total bytes written\t\t: %lld\n", GIocpManager->GetTotalByteWritten());
	wprintf_s( L"Total bytes read\t\t: %lld\n", GIocpManager->GetTotalByteRead() );

	wprintf_s( L"End Clients\n" );
	getchar();

	delete GIocpManager;
	delete GSessionManager;
	delete GMemoryPool;

	return 0;
}

