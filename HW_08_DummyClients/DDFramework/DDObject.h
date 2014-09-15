#pragma once
#include "DDConfig.h"
#include "DDRenderer.h"

// 전방선언
//class DDRenderer;

class DDObject
{
public:
	DDObject();
	virtual ~DDObject();

	CREATE_OBJECT(DDObject);

	static void DeleteAlignedClass( DDObject* object );

	virtual void Release( );
	
	const DDObject* GetParent() { return m_pParent; }
	const std::list<std::shared_ptr<DDObject>>& GetChildList() { return m_ChildList; }
	
	void AddChild( DDObject* object );
	void RemoveChild( DDObject* object );

	D3DXMATRIXA16 GetMatrix() const { return m_Matrix; }

	const DDVECTOR3 GetPosition() { return m_Position; }
	const float GetPositionX() const { return m_Position.x; }
	const float GetPositionY() const { return m_Position.y; }
	const float GetPositionZ() const { return m_Position.z; }
	
	const DDVECTOR3 GetScale() { return m_Scale; }
	const float GetScaleX() const { return m_Scale.x; }
	const float GetScaleY() const { return m_Scale.y; }
	const float GetScaleZ() const { return m_Scale.z; }
	
	const DDVECTOR3 GetRotation() const { return m_Rotation; }
	const float GetRotationX() const { return m_Rotation.x; }
	const float GetRotationY() const { return m_Rotation.y; }
	const float GetRotationZ() const { return m_Rotation.z; }
	
	void IncreasePosition(DDVECTOR3 position) { m_Position += position; }
	void IncreasePositionX(float x) { m_Position.x += x; }
	void IncreasePositionY(float y) { m_Position.y += y; }
	void IncreasePositionZ(float z) { m_Position.z += z; }

	void IncreaseScale(DDVECTOR3 scale) { m_Scale += scale; }
	void IncreaseScaleX(float x) { m_Scale.x += x; }
	void IncreaseScaleY(float y) { m_Scale.y += y; }
	void IncreaseScaleZ(float z) { m_Scale.z += z; }

	void IncreaseRotation(DDVECTOR3 rotation) { m_Rotation += rotation; }
	void IncreaseRotationX(float x) { m_Rotation.x += x; }
	void IncreaseRotationY(float y) { m_Rotation.y += y; }
	void IncreaseRotationZ(float z) { m_Rotation.z += z; }

	bool IsVisible() const { return m_VisibleFlag; }
	bool IsUpdatable() const { return m_UpdatableFlag; }

	void SetPosition( float x, float y, float z ) { m_Position = DDVECTOR3( x, y, z ); }
	void SetPosition( DDVECTOR3 point ) { m_Position = point; }

	void SetScale( float scaleX, float scaleY, float scaleZ ) {	m_Scale = DDVECTOR3( scaleX, scaleY, scaleZ );}
	void SetScale( DDVECTOR3 scale ) { m_Scale = scale; }

	void SetRotation( DDVECTOR3 rotation ) { m_Rotation = rotation; }
	void SetRotation( float rotationX, float rotationY, float rotationZ ) { m_Rotation = DDVECTOR3( rotationX, rotationY, rotationZ ); }
	
	void SetVisible( bool visible ) { m_VisibleFlag = visible; }
	void SetUpdatable( bool updatable ) { m_UpdatableFlag = updatable; }

	

	// z축 방향 벡터를 월드 좌표계 기준으로 반환
	DDVECTOR3 GetViewDirection() const;
	DDVECTOR3 GetAxisX();

	// NVI Wrapper (비가상 인터페이스)
	void Render();
	void Update( float dTime );


protected:
	void SetParent( DDObject* object ) { m_pParent = object; }

	// NVI virtual function
	virtual void RenderItSelf() {}
	virtual void UpdateItSelf( float dTime ) { UNREFERENCED_PARAMETER( dTime ); }

	DDObject*		m_pParent = nullptr;
	std::list<std::shared_ptr<DDObject>>	m_ChildList;
		
	D3DXMATRIXA16	m_Matrix;			// world coordinate
	D3DXMATRIXA16	m_MatrixTransform;	// local coordinate
	D3DXMATRIXA16	m_MatrixRotation;
	DDVECTOR3		m_Position{ .0f, .0f, .0f };	// c++11에서 나온 균일한 중괄호 초기화라함.
	DDVECTOR3		m_Rotation{ .0f, .0f, .0f };
	DDVECTOR3		m_Scale{ 1.0f, 1.0f, 1.0f };

	bool			m_VisibleFlag = true ;
	bool			m_UpdatableFlag = true;

private : 
	// NVI 함수 내용
	void AffineTransfrom();
	void RenderChildNodes();
	void UpdateChildNodes( float dTime );

	
};
// 
// typedef std::aligned_storage<sizeof( DDObject ), ALIGNMENT_SIZE>::type DDObjectA;