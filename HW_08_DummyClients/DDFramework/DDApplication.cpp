#include "DDApplication.h"
#include "DDInputSystem.h"
#include "DDRenderer.h"
#include "DDSceneDirector.h"
#include "DDNetwork.h"

#pragma comment(lib,"ws2_32.lib")

DDApplication::DDApplication()
{
}

DDApplication::~DDApplication()
{

}


bool DDApplication::Init( std::wstring title, int width, int height )
{
	m_hInstance = GetModuleHandle( 0 );

	//m_pTitle = title;	// agebreak : 1. 이 코드는 과연 안전한 코드일까? 2. 이 멤버 변수는 굳이 필요할까?
	// 변수 삭제, std::wstring으로 변경
	m_ScreenWidth = width;
	m_ScreenHeight = height;

	// 윈도우와 렌더러 생성
	//if ( !_CreateWindow( m_pTitle, m_ScreenWidth, m_ScreenHeight ) ) 
	if ( !_CreateWindow( title, m_ScreenWidth, m_ScreenHeight ) )
	{
		// 윈도우 생성 실패
		return false;
	}

// 	if ( !_CreateRenderer() )
// 	{
// 		// 렌더러 생성 실패
// 		return false;
// 	}

	if ( !DDRenderer::GetInstance()->Init( m_Hwnd ) )
	{
		// 렌더러 초기화 실패
		return false;
	}

	
	if ( !DDSceneDirector::GetInstance()->Init() )
	{
		// 씬 디렉터 초기화 실패
		return false;
	}

	// random seed 생성
	srand( (unsigned int)time( NULL ) );

	DDNetwork::GetInstance()->Init();

	return true;
}


bool DDApplication::_CreateWindow( std::wstring title, int width, int height )
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = NULL;
	wcex.cbWndExtra = NULL;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Debris Defragmentation";
	wcex.hIconSm = NULL;
	wcex.hIcon = NULL;

	RegisterClassEx( &wcex );

	DWORD style = WS_OVERLAPPEDWINDOW;

	RECT wr = { 0, 0, width, height };
	AdjustWindowRect( &wr, WS_OVERLAPPEDWINDOW, FALSE );

	m_Hwnd = CreateWindow( L"Debris Defragmentation", title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
						   wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, m_hInstance, NULL );

	ShowWindow( m_Hwnd, SW_SHOWNORMAL );

	return true;
}

// bool DDApplication::_CreateRenderer()
// {
// 	// m_pRenderer생성
// 	DDRenderer::GetInstance();	
// 
// 	return true;
// }



bool DDApplication::Release()
{
// 	if ( m_DestroyWindow ) {
// 		ReleaseInstance();
// 		return true;
// 	}

	DDNetwork::ReleaseInstance();

	// agebreak : 싱글톤 = 멤버 변수라니, 이상하지 않은가?
	// singleton을 직접 호출하는 방식으로 변경
	DDSceneDirector::GetInstance()->Release();
	DDSceneDirector::ReleaseInstance();

	DDRenderer::GetInstance()->Release();
	DDRenderer::ReleaseInstance();

	DDInputSystem::ReleaseInstance();	

	ReleaseInstance();

	return true;
}

int DDApplication::Run()
{
	MSG msg;
	ZeroMemory( &msg, sizeof( msg ) );

	while ( true )
	{
		// 메세지가 있으면 메세지 처리 루틴으로
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				return true;
			}
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			// agebreak : FPS 구하는 내용이 여기에 구현되어 있을 필요가 있을까? 함수나 클래스로 따로 구현해서 사용하는게 좋음.
			// FPS 구하기
			ComputeFPS();

			DDInputSystem::GetInstance()->UpdateKeyState();

			// update scene
			DDSceneDirector::GetInstance()->UpdateScene( m_DeltaTime );

			// display
			if ( true ) // 프레임 수를 조절하려면 여기서 시간 제약을 둬야
			{
				DDRenderer::GetInstance()->Clear();
				DDRenderer::GetInstance()->Begin();			
				DDSceneDirector::GetInstance()->RenderScene();
				DDRenderer::GetInstance()->End();
			}

			if ( DDInputSystem::GetInstance()->GetKeyState( VK_ESCAPE ) == KEY_DOWN )
			{
				PostQuitMessage( 0 );
			}
		}
	}

	return true;
}


LRESULT CALLBACK DDApplication::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
		case WM_CREATE:
		{
			break;
		}

		case WM_DESTROY:
		{
			//DDApplication::GetInstance()->Release();
			//DDApplication::GetInstance()->m_DestroyWindow = true;
			PostQuitMessage( 0 );
			break;
		}

		case WM_PAINT:
		{
			break;
		}

		case WM_SOCKET:
		{
			if ( WSAGETSELECTERROR( lParam ) )
			{
				MessageBox( hWnd, L"WSAGETSELECTERROR", L"Error", MB_OK | MB_ICONERROR );
				SendMessage( hWnd, WM_DESTROY, NULL, NULL );
				break;
			}

			switch ( WSAGETSELECTEVENT( lParam ) )
			{
				case FD_CONNECT:
				{
					DDNetwork::GetInstance()->NagleOff();
					break;
				}

				case FD_READ:
				{
					if ( DDNetwork::GetInstance()->Read() )
					{
						DDNetwork::GetInstance()->ProcessPacket();
					}					
					break;
				}

				case FD_WRITE:
				{
					/// 실제로 버퍼에 있는것들 꺼내서 보내기
					DDNetwork::GetInstance()->Send();
					break;
				}

				case FD_CLOSE:
				{
					MessageBox( hWnd, L"Server closed connection", L"Connection closed!", MB_ICONINFORMATION | MB_OK );
					DDNetwork::GetInstance()->Disconnect();
					SendMessage( hWnd, WM_DESTROY, NULL, NULL );
					break;
				}
			}
			break; // WM_SOCKET end;
		}
		break; // switch end
	}

	return( DefWindowProc( hWnd, message, wParam, lParam ) );
}

void DDApplication::ComputeFPS()
{
	m_FrameCount++;
	m_NowTime = timeGetTime();

	if ( m_PrevTime == 0.f )
	{
		m_PrevTime = m_NowTime;
	}

	m_DeltaTime = ( static_cast<float>( m_NowTime - m_PrevTime ) ) / 1000.f;
	m_ElapsedTime += m_DeltaTime;
	m_FpsTimer += m_DeltaTime;

	if ( m_FpsTimer > 0.1f )
	{
		m_Fps = ( (float)m_FrameCount ) / m_FpsTimer;
		m_FrameCount = 0;
		m_FpsTimer = 0.f;
	}
	m_PrevTime = m_NowTime;
}

