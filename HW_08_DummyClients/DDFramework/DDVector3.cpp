#include "stdafx.h"
#include "DDVector3.h"

DDVector3& DDVector3::operator= ( const DDVector3& vec )
{
	this->SetVector( vec.GetX(), vec.GetY(), vec.GetZ() );
	return *this;
}

DDVector3& DDVector3::operator += ( const DDVector3& vec )
{
	this->SetVector( this->m_X + vec.GetX(), this->m_Y + vec.GetY(), this->m_Z + vec.GetZ() );
	return *this;
}

DDVector3& DDVector3::operator -= ( const DDVector3& vec )
{
	this->SetVector( this->m_X - vec.GetX(), this->m_Y - vec.GetY(), this->m_Z - vec.GetZ() );
	return *this;
}

DDVector3 DDVector3::operator+( const DDVector3& vec ) const
{
	return DDVector3( this->m_X + vec.GetX(), this->m_Y + vec.GetY(), this->m_Z + vec.GetZ() );
}

DDVector3 DDVector3::operator-( const DDVector3& vec ) const
{
	return DDVector3( this->m_X - vec.GetX(), this->m_Y - vec.GetY(), this->m_Z - vec.GetZ() );
}

DDVector3 DDVector3::operator-( ) const
{
	return DDVector3( -this->m_X, -this->m_Y, -this->m_Z );
}

DDVector3 DDVector3::operator*( float f ) const
{
	return DDVector3( this->m_X * f, this->m_Y * f, this->m_Z * f );
}

DDVector3 DDVector3::operator/( float f ) const
{
	return DDVector3( this->m_X / f, this->m_Y / f, this->m_Z / f );
}

