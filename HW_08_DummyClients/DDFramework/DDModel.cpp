#include "DDConfig.h"
#include "DDModel.h"
#include "DDRenderer.h"
#include "DDApplication.h"

DDModel::DDModel()
{
}

DDModel::DDModel( std::wstring path )
{
	InitModel( path );
}

DDModel::~DDModel()
{
	Cleanup();
}

bool DDModel::InitModel( std::wstring path )
{
	LPD3DXBUFFER		pD3DXMtrlBuffer;	
	LPDIRECT3DDEVICE9	pD3DDevice = DDRenderer::GetInstance()->GetDevice();

	std::wstring xfilePath = L".\\Resources\\3DModel\\";
	xfilePath.append(path);

	if ( FAILED( D3DXLoadMeshFromX( xfilePath.c_str(), D3DXMESH_SYSTEMMEM, pD3DDevice, NULL, &pD3DXMtrlBuffer, NULL, &m_dwNumMaterials, &m_pMesh ) ) )
	{
		// x file loading error
		printf( "No Model\n" );
		return false;
	}

	D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
	m_pMeshMaterials = new D3DMATERIAL9[m_dwNumMaterials];
	if ( m_pMeshMaterials == NULL )
	{
		// out of memory
		return false;
	}

	m_pMeshTexture = new LPDIRECT3DTEXTURE9[m_dwNumMaterials];
	if ( m_pMeshTexture == NULL )
	{
		// out of memory
		return false;
	}

	// texture 불러오는 부분, bmp파일 경로를 통해 매터리얼 개수만큼 불러옴
	for ( DWORD i = 0; i < m_dwNumMaterials; i++ )
	{
		m_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;
		m_pMeshMaterials[i].Ambient = m_pMeshMaterials[i].Diffuse;

		m_pMeshTexture[i] = NULL;
		if ( d3dxMaterials[i].pTextureFilename != NULL && lstrlenA( d3dxMaterials[i].pTextureFilename ) > 0 )
		{
 			std::string bmpPath = ".\\Resources\\3DModel\\";
			bmpPath.append(d3dxMaterials[i].pTextureFilename );

			if ( FAILED( D3DXCreateTextureFromFileA( pD3DDevice, bmpPath.c_str(), &m_pMeshTexture[i] ) ) )
			{
				//MessageBox( NULL, L"no texture map", L"Meshes.exe", MB_OK );
				// no texture error
				return false;
			}
		}
	}

	pD3DXMtrlBuffer->Release();


	if ( SetNormalVector() ) {
		// failed compute normal vector 
		return false;
	}

	return true;
}


bool DDModel::SetNormalVector()
{
	if ( !( m_pMesh->GetFVF() & D3DFVF_NORMAL ) )
	{
		//가지고 있지 않다면 메쉬를 복제하고 D3DFVF_NORMAL을 추가한다.
		ID3DXMesh* pTempMesh = 0;
		m_pMesh->CloneMeshFVF(
			D3DXMESH_MANAGED,
			m_pMesh->GetFVF() | D3DFVF_NORMAL,  //이곳에 추가
			DDRenderer::GetInstance()->GetDevice(),
			&pTempMesh );

		// 법선을 계산한다.
		if ( FAILED( D3DXComputeNormals( pTempMesh, 0 ) ) )
		{
			return false;
		}

		m_pMesh->Release(); // 기존메쉬를 제거한다
		m_pMesh = pTempMesh; // 기존메쉬를 법선이 계산된 메쉬로 지정한다.
	}

	return true;
}

bool DDModel::Cleanup()
{
	// texture release
	if ( m_pMeshTexture )
	{
		for ( DWORD i = 0; i < m_dwNumMaterials; ++i )
		{
			if ( m_pMeshTexture[i] )
			{
				m_pMeshTexture[i]->Release();
			}
		}
		delete[] m_pMeshTexture;
		m_pMeshTexture = nullptr;
	}

	// mesh release
	if ( m_pMeshMaterials != NULL )
	{
		delete[] m_pMeshMaterials;
		m_pMeshMaterials = nullptr;
	}
	if ( m_pMesh != nullptr )
	{
		m_pMesh->Release();
		m_pMesh = nullptr;
	}

	return true;
}


void DDModel::RenderItSelf()
{
	LPDIRECT3DDEVICE9 pD3DDevice = DDRenderer::GetInstance()->GetDevice();
	for ( DWORD i = 0; i < m_dwNumMaterials; ++i )
	{
		pD3DDevice->SetMaterial( &m_pMeshMaterials[i] );
		pD3DDevice->SetTexture( 0, m_pMeshTexture[i] );
		m_pMesh->DrawSubset( i );
	}
}
