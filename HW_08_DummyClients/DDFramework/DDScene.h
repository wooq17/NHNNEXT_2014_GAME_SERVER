#pragma once

#include "DDObject.h"
#include "DDPoint.h"
#include "DDInputSystem.h"
#include <vector>
#include "DDUIControl.h"

class DDScene :
	public DDObject
{
public:
	DDScene();
	DDScene( std::wstring sceneName );
	virtual ~DDScene();
	
	CREATE_OBJECT_WSTRING( DDScene, sceneName );	
	
	std::wstring GetSceneName() const { return m_SceneName; }
	void SetSceneName( std::wstring val ) { m_SceneName = val; }

	// override
	//void Render();

protected:	
	// 작성자 : 최경욱 4.8
	// input : int값의 키
	// output : 인자로 넘긴 키의 현재 상태를 받아오는 함수
	KeyState	GetKeyState( int key );

	// input : 없음
	// output : 현재 마우스 좌표 받아오는 함수
	DDPoint		GetMousePosition();
	

	// 여기서 나중에 skybox 만들면 될 거 같음(4.6 성환)
	//LPD3DXSPRITE	m_pSprite;

	// scene을 구별하기 위한 이름
	std::wstring m_SceneName;

	// UI를 갖고 있는 멤버
	//std::vector<DDUIControl*> m_UICollection;
};

