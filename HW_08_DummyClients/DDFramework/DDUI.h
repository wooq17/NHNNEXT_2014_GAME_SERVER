#pragma once
#include "DDObject.h"
class DDUI :
	public DDObject
{
public:
	DDUI();
	virtual ~DDUI();

	CREATE_OBJECT( DDUI );

	bool InitUI( std::wstring path );
	bool Cleanup();

protected:	
	ID3DXSprite* m_pUIObject = nullptr;
	LPDIRECT3DTEXTURE9 m_pTexture = nullptr;

	// m_pCenter == NULL 은 스프라이트 왼쪽 끝에서 그리겠다는 겁니다
	//DDVECTOR3	m_Center{ .0f, .0f, .0f };
	// m_pRect == NULL 은 전체 텍스쳐를 다 쓰겠다는 뜻입니다
	//RECT		m_Rect;
	D3DCOLOR	m_Color = 0xFFFFFFFF;  // 본래 색깔

private :
	virtual void RenderItSelf();
};

