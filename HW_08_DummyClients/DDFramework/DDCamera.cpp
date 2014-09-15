#include "DDCamera.h"
#include "DDRenderer.h"
#include "DDApplication.h"


DDCamera::DDCamera():
m_LookatPoint(0.0f, 0.0f, 10.0f)
{	
	SetPosition( 0.0f, 0.0f, 0.0f );
}

DDCamera::~DDCamera()
{
}

void DDCamera::RenderItSelf()
{
	D3DXVECTOR4 tempEye;
	D3DXVec3Transform( &tempEye, &m_Position, &m_Matrix );
	DDVECTOR3 vEyePt( tempEye.x, tempEye.y, tempEye.z );

	D3DXVECTOR4 tempLook;
	D3DXVec3Transform( &tempLook, &m_LookatPoint, &m_Matrix );
	DDVECTOR3 vLookatPt( tempLook.x, tempLook.y, tempLook.z );

	DDVECTOR3 vUpVec( m_Matrix._21, m_Matrix._22, m_Matrix._23 );

	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	DDRenderer::GetInstance()->GetDevice()->SetTransform( D3DTS_VIEW, &matView );

	D3DXMATRIXA16 matProj;

	float aspectRatio = static_cast<float>(DDApplication::GetInstance()->GetScreenWidth()) / static_cast<float>(DDApplication::GetInstance()->GetScreenHeight());
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 5, aspectRatio, 1.0f, 2000.0f );
	//D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 1000.0f );
	DDRenderer::GetInstance()->GetDevice()->SetTransform( D3DTS_PROJECTION, &matProj );
}

