#pragma once
#include "DDConfig.h"

class DDRenderer : public Singleton<DDRenderer>
{
public:
	DDRenderer();
	~DDRenderer();

	bool Init( HWND hWnd );
	bool Init( HWND hWnd, int ScreenWidth, int ScreenHeight );
	bool Release();

	bool Clear();
	bool Begin();
	bool End();

	LPDIRECT3D9				GetD3D() { return m_pD3D; }
	LPDIRECT3DDEVICE9		GetDevice() { return m_pD3DDevice; }
	LPD3DXSPRITE			GetSprite() { return m_pSprite; }

private:

	LPDIRECT3D9				m_pD3D = nullptr;
	LPDIRECT3DDEVICE9		m_pD3DDevice = nullptr;
	LPD3DXSPRITE			m_pSprite = nullptr;
	D3DPRESENT_PARAMETERS	m_D3DPresentParameters;
};