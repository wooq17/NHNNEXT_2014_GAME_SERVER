#pragma once
#include "DDObject.h"
#include <d3dx9core.h>

// UI는... 기반 작업 다시 해야되네요 쿨럭
// UI는 얘를 상속받아서 지지고 볶으면 됩니다
// 4. 22 문진상

class DDUIControl
{
public:
	DDUIControl();
	virtual ~DDUIControl();

	// 비가상입니다 주는대로쓰세요
	void Render();
	void Init( const std::wstring& filePath,
		const float LeftTopX, const float LeftTopY);

	// 0x 시리즈로 넣어주십쇼
	void inline SetSpriteColor( D3DCOLOR Color ) { m_Color = Color; }
	// 크기조절
	void SetTransform( float x, float y, float z );

protected:
	// UI 오브젝트
	ID3DXSprite* m_pUIObject = nullptr;

	// UI 드로잉에 필요한 값들을 멤버 변수로
	LPDIRECT3DTEXTURE9 m_pTexture = NULL;
	RECT* m_pRect = nullptr;
	D3DXVECTOR3* m_pCenter = nullptr;
	D3DXVECTOR3* m_pPosition = nullptr;
	D3DCOLOR m_Color = 0xFFFFFFFF; // 본래 색깔
};

