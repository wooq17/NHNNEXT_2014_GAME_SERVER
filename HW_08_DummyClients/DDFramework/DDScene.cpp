#include "stdafx.h"
#include "DDScene.h"


DDScene::DDScene():
m_SceneName( L"DefaultSceneName" )
{
}

DDScene::DDScene( std::wstring sceneName ):
m_SceneName(sceneName)
{

}


DDScene::~DDScene()
{
// 	for ( auto iter = m_UICollection.begin(); iter != m_UICollection.end(); ++iter )
// 	{
// 		auto pWillBeDie = ( *iter );
// 		delete pWillBeDie;
// 	}
}


KeyState DDScene::GetKeyState( int key )
{
	return DDInputSystem::GetInstance()->GetKeyState( key );
}

DDPoint DDScene::GetMousePosition()
{
	return DDInputSystem::GetInstance()->GetMousePosition();
}

// void DDScene::Render()
// {
// 	DDObject::Render();
// 
// 	for ( auto iter = m_UICollection.begin(); iter != m_UICollection.end(); ++iter )
// 	{
// 		(*iter)->Render();
// 	}
// }
