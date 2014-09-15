#include "stdafx.h"
#include "DDUI.h"


DDUI::DDUI()
{
}


DDUI::~DDUI()
{
	Cleanup();
}


bool DDUI::InitUI( std::wstring path )
{
	if ( FAILED( D3DXCreateTextureFromFile( DDRenderer::GetInstance()->GetDevice(), path.c_str(), &m_pTexture ) ) )
	{
		return false;
	}
	
	if ( FAILED( D3DXCreateSprite( DDRenderer::GetInstance()->GetDevice(), &m_pUIObject ) ) )
	{
		return false;
	}
	return true;
}

void DDUI::RenderItSelf()
{	
	m_pUIObject->SetTransform( &m_Matrix );
	m_pUIObject->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
	//m_pUIObject->Draw( m_pTexture, &m_Rect, &m_Center, &m_Position, m_Color );
	m_pUIObject->Draw( m_pTexture, NULL, NULL, &m_Position, m_Color );
	m_pUIObject->End();
}

bool DDUI::Cleanup()
{
	if ( m_pUIObject != nullptr )
	{
		m_pUIObject->Release();
		m_pUIObject = nullptr;
	}

	if ( m_pTexture != nullptr )
	{
		m_pTexture->Release();
		m_pTexture = nullptr;
	}	
	return true;
}
