#pragma once
#include "DDObject.h"

class DDModel :
	public DDObject
{
public:
	DDModel();
	DDModel( std::wstring filePath );
	virtual ~DDModel();

	CREATE_OBJECT_WSTRING( DDModel, filePath );
	//static DDModel* Create( wchar_t* filePath );	

protected:
	virtual void RenderItSelf();
	bool InitModel( std::wstring path );
	bool SetNormalVector();
	bool Cleanup();

	// LPDIRECT3DDEVICE9	m_pD3DDevice = nullptr;

	LPD3DXMESH			m_pMesh = nullptr;
	LPDIRECT3DTEXTURE9* m_pMeshTexture = nullptr;
	D3DMATERIAL9*		m_pMeshMaterials = nullptr;
	DWORD				m_dwNumMaterials = 0L;

private:
};

