#pragma once

#define MAX_NAME_LEN	32
#define MAX_COMMENT_LEN	256

#define ZONE_NUMBER_EACH_AXIS	10
#define ZONE_NUMBER				100
#define ZONE_RANGE				100

struct Float3D
{
	Float3D()
		: m_X( 0.0f ), m_Y( 0.0f ), m_Z( 0.0f )
	{}

	Float3D( float x, float y, float z )
		: m_X( x ), m_Y( y ), m_Z( z )
	{}

	Float3D& operator=( const Float3D& rhs )
	{
		m_X = rhs.m_X;
		m_Y = rhs.m_Y;
		m_Z = rhs.m_Z;

		return *this;
	}

	Float3D& operator+( const Float3D& rhs )
	{
		m_X += rhs.m_X;
		m_Y += rhs.m_Y;
		m_Z += rhs.m_Z;

		return *this;
	}

	float m_X = 0.0f;
	float m_Y = 0.0f;
	float m_Z = 0.0f;
};