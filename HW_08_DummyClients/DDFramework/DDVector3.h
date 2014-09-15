#pragma once
class DDVector3
{
public:
	DDVector3() : m_X( 0.0f ), m_Y( 0.0f ), m_Z( 0.0f ) {}
	DDVector3(float x, float y, float z) : m_X( x ), m_Y( y ), m_Z(z ) {}
	DDVector3( DDVector3 &vec ) : m_X( vec.GetX() ), m_Y( vec.GetY() ), m_Z( vec.GetZ() ) {}
	~DDVector3() {}

	DDVector3& operator= ( const DDVector3& vec );
	DDVector3& operator+= ( const DDVector3& vec );
	DDVector3& operator-= ( const DDVector3& vec );
	DDVector3 operator+( const DDVector3& vec ) const;
	DDVector3 operator-( const DDVector3& vec ) const;
	DDVector3 operator-( ) const;
	DDVector3 operator*( float f ) const;
	DDVector3 operator/( float f ) const;

	inline float GetX() const { return m_X; }
	inline float GetY() const { return m_Y; }
	inline float GetZ() const { return m_Z; }

	void SetVector(float x, float y, float z) {
		m_X = x;
		m_Y = y;
		m_Z = z;
	}

	void SetVector( DDVector3& vec ) { *this = vec; }

private:
	float m_X, m_Y, m_Z;
};

