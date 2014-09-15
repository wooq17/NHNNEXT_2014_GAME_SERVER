#include "stdafx.h"
#include "DDPhysics.h"


DDPhysics::DDPhysics()
{
}

DDPhysics::~DDPhysics()
{
}

void DDPhysics::CalcCurrentPosition( _Inout_ DDVECTOR3* pos, _In_ const DDVECTOR3 &velocity, _In_ float dt )
{
	// s' = s + v * t	
	
	pos->x = pos->x + ( velocity.x * dt );
	pos->y = pos->y + ( velocity.y * dt );
	pos->z = pos->z + ( velocity.z * dt );
}

void DDPhysics::CalcCurrentPosition( _Inout_ DDVECTOR3* pos, _Inout_ DDVECTOR3* velocity, _In_ const DDVECTOR3 &acceleration, _In_ float dt )
{
	// s' = s + v * t + 1/2 * a * t^2

	// v * t
	CalcCurrentPosition( pos, *velocity, dt );

	// v' = v + a * t
	CalcCurrentPosition( velocity, acceleration, dt );

	// 1/2 * a * t^2
	
	pos->x = pos->x + ( 0.5f * acceleration.x * dt * dt );
	pos->y = pos->y + ( 0.5f * acceleration.y * dt * dt );
	pos->z = pos->z + ( 0.5f * acceleration.z * dt * dt );
}

void DDPhysics::GetNormalVector( _In_ DDVECTOR3* srcVec, _Out_ DDVECTOR3* normalVec )
{
	float length = sqrtf( srcVec->x * srcVec->x + srcVec->y * srcVec->y + srcVec->z * srcVec->z );

	normalVec->x = srcVec->x / length;
	normalVec->y = srcVec->y / length;
	normalVec->z = srcVec->z / length;
}