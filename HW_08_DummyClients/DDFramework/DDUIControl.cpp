#include "DDUIControl.h"
#include "DDRenderer.h"
// UI의 기본이 되는 녀석

DDUIControl::DDUIControl()
{
}


DDUIControl::~DDUIControl()
{
	if ( m_pUIObject != nullptr )
	{
		m_pUIObject->Release();
	}

	if ( m_pPosition != nullptr )
	{
		delete m_pPosition;
		m_pPosition = nullptr;
	}
}

void DDUIControl::Init( const std::wstring& filePath, const float LeftTopX, const float LeftTopY)
{
	// Create UI Texture
	// 밑에 함수는 복붙한건데 안됨^^;; 옵션을 잘 보고 가져왔어야지!!

// 	D3DXCreateTextureFromFileEx( DDRenderer::GetInstance()->GetDevice(), filePath.c_str(), D3DX_DEFAULT, D3DX_DEFAULT,
// 		D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
// 		D3DX_DEFAULT, D3DX_DEFAULT, 0,
// 		NULL, NULL, &m_pTexture );


	D3DXCreateTextureFromFile( DDRenderer::GetInstance()->GetDevice(), filePath.c_str(), &m_pTexture );

	// m_pRect == NULL 은 전체 텍스쳐를 다 쓰겠다는 뜻입니다

	// m_pCenter == NULL 은 스프라이트 왼쪽 끝에서 그리겠다는 겁니다
	
	// Create Position
	m_pPosition = new D3DXVECTOR3(LeftTopX, LeftTopY, 0.f);

	// Create D3DXSprite
	D3DXCreateSprite( DDRenderer::GetInstance()->GetDevice(), &m_pUIObject );
}

void DDUIControl::Render()
{
	m_pUIObject->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
	m_pUIObject->Draw( m_pTexture, m_pRect, m_pCenter, m_pPosition, m_Color );
	m_pUIObject->End();
}

void DDUIControl::SetTransform( float x, float y, float z )
{
	D3DXMATRIX* size = new D3DXMATRIX();
	D3DXMatrixScaling( size, x, y, z );
	m_pUIObject->SetTransform( size );
}

