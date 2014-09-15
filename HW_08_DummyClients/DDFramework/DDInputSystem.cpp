#include "DDInputSystem.h"
#include "DDApplication.h"

DDInputSystem::DDInputSystem()
{
	ZeroMemory( m_PrevKeyState, sizeof( m_PrevKeyState ) );
	ZeroMemory( m_NowKeyState, sizeof( m_NowKeyState ) );
}

DDInputSystem::~DDInputSystem()
{
}

void DDInputSystem::UpdateKeyState()
{
	if ( GetFocus() == NULL ) return;

	for ( int i = 0; i<256; i++ )
	{
		m_PrevKeyState[i] = m_NowKeyState[i];

		if ( ::GetKeyState( i ) & 0x8000 )
		{
			m_NowKeyState[i] = true;
		}
		else
		{
			m_NowKeyState[i] = false;
		}
	}
}

KeyState DDInputSystem::GetKeyState( int key )
{
	// 직전에 안 눌려지던 키가 눌려지고 있다면
	if ( m_PrevKeyState[key] == false && m_NowKeyState[key] == true )
	{
		return KEY_DOWN;
	}
	// 직전에 눌려져있던 키가 지금도 눌려지고 있다고
	else if ( m_PrevKeyState[key] == true && m_NowKeyState[key] == true )
	{
		return KEY_PRESSED;
	}
	// 직전에 눌리고 있던 키가 지금은 안 눌려지고 있다면
	else if ( m_PrevKeyState[key] == true && m_NowKeyState[key] == false )
	{
		return KEY_UP;
	}
	// 마지막 경우는 직전에도 안 눌려지고 지금도 안 눌려진 키
	return KEY_NOTPRESSED;
}

DDPoint DDInputSystem::GetMousePosition()
{
	POINT pt;
	
	GetCursorPos( &pt );
	ScreenToClient( DDApplication::GetInstance()->GetHWND(), &pt );

	return DDPoint( (float)pt.x, (float)pt.y );
}

bool DDInputSystem::IsPressedAnyKey()
{
	for ( int i = 0; i < sizeof( m_NowKeyState ) / sizeof( bool ); ++i )
	{
		if ( m_NowKeyState[i] == true )
			return true;
	}
	return false;
}
