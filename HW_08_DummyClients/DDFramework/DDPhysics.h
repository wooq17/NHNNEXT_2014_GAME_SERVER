#pragma once

/*
	작성자 : 최경욱
	작성일 : 2014. 4. 6
	내용 : 물리적 계산 수행(물체 운동, 회전, 충돌 등)
*/

#include "DDConfig.h"
#include "DDPoint.h"

class DDPhysics
{
public:
	DDPhysics();
	~DDPhysics();

	/*
		input : 현재 위치, 속도, 시간 변화량
		output : 업데이트된 현재 위치
		주의 : 등속운동인 경우 사용, 가속도에 의한 계산이 필요하면 오버로딩된 함수 사용할 것
	*/
	static void CalcCurrentPosition( _Inout_ DDVECTOR3* pos, const DDVECTOR3 &velocity, float dt );

	/*
		input : 현재 위치, 현재 속도, 가속도, 시간 변화량
		output : 업데이트된 현재 위치, 업데이트된 현재 속도
		주의 : 가속도에 의한 운동인 경우 사용, 등속운동의 경우 오버로딩된 함수 사용할 것
	*/
	static void CalcCurrentPosition( _Inout_ DDVECTOR3* pos, _Inout_ DDVECTOR3* velocity, const DDVECTOR3 &acceleration, float dt );

	/*
		input : 원본 벡터와 결과가 저장될 벡터 주소
		output : 원본 벡터의 노멀 벡터
	*/
	static void GetNormalVector( _In_ DDVECTOR3* srcVec, _Out_ DDVECTOR3* normalVec );
};

