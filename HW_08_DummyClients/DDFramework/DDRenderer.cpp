#include "DDRenderer.h"
#include "DDApplication.h"

DDRenderer::DDRenderer()
{
}

DDRenderer::~DDRenderer()
{
}

bool DDRenderer::Init( HWND hWnd )
{
	return Init( hWnd, DDApplication::GetInstance()->GetScreenWidth(), DDApplication::GetInstance()->GetScreenHeight() );
}

// overriding
bool DDRenderer::Init( HWND hWnd, int ScreenWidth, int ScreenHeight )
{
	HRESULT hr = 0;

	if ( NULL == ( m_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
	{
		return false;
	}

	ZeroMemory( &m_D3DPresentParameters, sizeof( m_D3DPresentParameters ) );

	D3DMULTISAMPLE_TYPE mst = D3DMULTISAMPLE_NONE;

	m_D3DPresentParameters.Windowed = TRUE;
	m_D3DPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_D3DPresentParameters.EnableAutoDepthStencil = TRUE;
	m_D3DPresentParameters.AutoDepthStencilFormat = D3DFMT_D16; // 16-bit z-buffer bit depth.
	m_D3DPresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_D3DPresentParameters.BackBufferWidth = ScreenWidth;
	m_D3DPresentParameters.BackBufferHeight = ScreenHeight;
	m_D3DPresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;;
	m_D3DPresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	m_D3DPresentParameters.hDeviceWindow = hWnd;

	hr = m_pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL, m_D3DPresentParameters.BackBufferFormat, true, mst, NULL );

	if ( FAILED( hr ) )
	{
		return false;
	}

	m_D3DPresentParameters.MultiSampleType = mst;

	hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3DPresentParameters, &m_pD3DDevice ); //D3DCREATE_SOFTWARE_VERTEXPROCESSING

	if ( FAILED( hr ) )
	{
		return false;
	}

	hr = D3DXCreateSprite( m_pD3DDevice, &m_pSprite );

	if ( FAILED( hr ) )
	{
		return false;
	}

	// 이거 없어도 되긴 하던데 ViewPort 해주는게 맞지 않나요?
	// 뭐지 이거 내가 아는 뷰포트가 아닌거 같은데 ㅋㅋㅋ
	D3DVIEWPORT9 pViewPort = { 0, 0, ScreenWidth, ScreenHeight, 0.0f, 1.0f };
	hr = m_pD3DDevice->SetViewport( &pViewPort );

	if ( FAILED( hr ) )
	{
		return false;
	}

	m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
	m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );

	//m_pD3DDevice->SetRenderState( D3DRS_ALPHAREF, 0x00000088 );
	//m_pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER );

	return true;
}

bool DDRenderer::Release()
{
	if ( m_pSprite != NULL )
		m_pSprite->Release();

	if ( m_pD3DDevice != NULL )
		m_pD3DDevice->Release();

	if ( m_pD3D != NULL )
		m_pD3D->Release();

	return true;
}

bool DDRenderer::Clear()
{
	if ( NULL == m_pD3DDevice )
		return false;

	HRESULT hr = NULL;

	hr = m_pD3DDevice->Clear(
		0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB( 100, 100, 100 ), 1.0f, 0 );

	if ( FAILED( hr ) )
		return false;

	return true;
}

bool DDRenderer::Begin()
{
	if ( NULL == m_pD3DDevice )
		return false;

	if ( FAILED( m_pD3DDevice->BeginScene() ) )
		return false;
	

	return true;
}

bool DDRenderer::End()
{
	if ( NULL == m_pD3DDevice )
		return false;

	HRESULT hr = 0;

	hr = m_pD3DDevice->EndScene();
	if ( FAILED( hr ) )
		return false;

	hr = m_pD3DDevice->Present( NULL, NULL, NULL, NULL );
	if ( FAILED( hr ) )
		return false;

	return true;
}
