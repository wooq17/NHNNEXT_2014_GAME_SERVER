#pragma once


/*
	작성자 : 최경욱
	작성일 : 2014. 4. 6
	내용 : 평면상의 좌표를 표현하고 연산하는 데이터 타입 정의 (NNGameFramework와 동일)
*/

#include "DDConfig.h"
// agebreak : 자주 쓰이고, 변하지 않는 헤더는 PreCompiled Header에 넣는게 좋다.
// pre-compiled header로 이동

class DDPoint
{
public:
	DDPoint() : m_X( 0.f ), m_Y( 0.f ) {}
	DDPoint( float x, float y ) : m_X( x ), m_Y( y ) {}
	DDPoint( const DDPoint& point ) : m_X( point.GetX() ), m_Y( point.GetY() ) {} // agebreak : 컴파일하면 비표준 확장이라면서 워닝이 발생함. 이것을 잡아라. 교수실로 들고오면 알려줌. 
	~DDPoint() {}

	// agebreak : 아래와 같이 간단하고 자주 쓰는 함수는, inline으로 선언한다
	// inline으로 변경 완료
	DDPoint& operator=( const DDPoint& point ) { this->SetPoint( point.GetX(), point.GetY() ); return *this; }
	DDPoint operator+( const DDPoint& point ) const { return DDPoint( this->m_X + point.GetX(), this->m_Y + point.GetY() );}
	DDPoint operator-( const DDPoint& point ) const { return DDPoint( this->m_X - point.GetX(), this->m_Y - point.GetY() );}
	DDPoint operator-( ) const { return DDPoint( -this->m_X, -this->m_Y ); }
	DDPoint operator*( float n ) const { return DDPoint( this->m_X * n, this->m_Y * n ); }
	DDPoint operator/( float n ) const { return DDPoint( this->m_X / n, this->m_Y / n ); }

	/*
		input : DDPoint 2개(시작점, 끝점), float 가중치
		output : 인자로 받은 두 점을 잇는 선분 위의 점들 중 가중치를 적용한 linear interpolation 지점의 좌표
		주의 : 가중치 t는 0과 1 사이의 값을 가진다.
	*/
	DDPoint DDPoint::Lerp( const DDPoint& startPoint, const DDPoint& endPoint, float t )
	{
		return DDPoint( t * startPoint.GetX() + ( 1 - t ) * endPoint.GetX()
			, t * startPoint.GetY() + ( 1 - t ) * endPoint.GetY() );
	}

	/*
		input : 거리를 알고 싶은 DDPoint
		output : 주어진 점과 떨어진 거리
	*/
	inline float GetDistance( DDPoint& point ) const {
		// agebreak : 두개의 오버로딩한 함수는 아래와 같이 하나로 통일해서 사용하는 것이 좋다
		//return sqrtf( pow( m_X - point.GetX(), 2 ) + pow( m_Y - point.GetY(), 2 ) );
		return GetDistance(point.GetX(), point.GetY());
	}

	/*
		input : 거리를 알고 싶은 점의 x좌표와 y좌표
		output : 주어진 점과 떨어진 거리
	*/
	inline float GetDistance( float x, float y ) const {
		return sqrtf( pow( m_X - x, 2 ) + pow( m_Y - y, 2 ) );
	}

	inline float GetX() const { return m_X; }
	inline float GetY() const { return m_Y; }

	void SetPoint( float x, float y ) {
		m_X = x;
		m_Y = y;
	}

	void SetPoint( DDPoint& point ) { *this = point; }

	void SetX( float x ) { m_X = x; }

	void SetY( float y ) { m_Y = y; }


private:
	float m_X, m_Y;
};

