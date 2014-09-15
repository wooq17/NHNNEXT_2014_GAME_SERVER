#include "stdafx.h"
#include "DDObject.h"
#include "DDRenderer.h"
#include "DDApplication.h"

DDObject::DDObject() 
{
	D3DXMatrixIdentity( &m_Matrix );
	D3DXMatrixIdentity( &m_MatrixTransform );
	D3DXMatrixIdentity( &m_MatrixRotation );
}

DDObject::~DDObject()
{
	// Release();
}

void DDObject::Release( )
{
}

// 작성자 : 김성환
// 작성일 : 14.04.06
// 내용 : object의 affine변환 및 자식 노드 render
void DDObject::Render()
{
	if ( m_VisibleFlag == false ) return;
		
	AffineTransfrom();
	RenderItSelf();
	RenderChildNodes();
}

void DDObject::Update( float dTime )
{
	if ( m_UpdatableFlag == false ) return;
	
	UpdateItSelf( dTime );
	UpdateChildNodes( dTime );
}

void DDObject::AffineTransfrom()
{
	D3DXQUATERNION	qRotation;	

	D3DXMatrixIdentity( &m_Matrix );

	// rotation에서 쿼터니언 생성, yaw ptich roll 은 y, x, z 순서임
	D3DXQuaternionRotationYawPitchRoll( &qRotation, D3DXToRadian( m_Rotation.y ), D3DXToRadian( m_Rotation.x ), D3DXToRadian( m_Rotation.z) );

	// matrix를 affine변환이 적용된 형태로 변환	
	D3DXMatrixTransformation( &m_Matrix, NULL, NULL, &m_Scale, NULL, &qRotation, &m_Position );

	// 부모의 좌표계에다 내 변환된 좌표계를 누적 시킨다!
	// 부모의 어파인 변환을 적용
	if ( nullptr != m_pParent )
	{
		D3DXMatrixMultiply( &m_Matrix, &m_Matrix, &m_pParent->GetMatrix() );
	}

	// 자신+부모의 어파인 변환을 월드좌표계에 적용
	if ( FAILED( DDRenderer::GetInstance()->GetDevice()->SetTransform( D3DTS_WORLD, &m_Matrix ) ) )
	{
		// error 
		return;
	}
}

DDVECTOR3 DDObject::GetViewDirection( ) const
{
	return DDVECTOR3( m_Matrix._31, m_Matrix._32, m_Matrix._33 );
}

DDVECTOR3 DDObject::GetAxisX()
{
	return DDVECTOR3( m_Matrix._11, m_Matrix._12, m_Matrix._13 );
}

void DDObject::RenderChildNodes()
{
	for ( const auto& child : m_ChildList )
	{
		child->Render();
	}
}

void DDObject::UpdateChildNodes( float dTime )
{
	for ( const auto& child : m_ChildList )
	{
		child->Update( dTime );
	}
}

void DDObject::AddChild( DDObject* object )
{
	auto deleter = DeleteAlignedClass;
	std::shared_ptr<DDObject> object_ptr( object, deleter );
	object->SetParent( this );
	m_ChildList.push_back( object_ptr );
}

void DDObject::RemoveChild( DDObject* object )
{
	for ( auto& iter = m_ChildList.begin(); iter != m_ChildList.end(); iter++ )
	{
		if ( ( *iter ).get() == object )
		{
			//( *iter )->Release();
			//SafeDelete( *iter );
			m_ChildList.erase( iter );
			break;
		}
	}
}

void DDObject::DeleteAlignedClass( DDObject* object )
{
	object->~DDObject();
	_aligned_free( object );
}
