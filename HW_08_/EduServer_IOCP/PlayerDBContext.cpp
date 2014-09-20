﻿#include "stdafx.h"
#include "SQLStatement.h"
#include "Log.h"
#include "PlayerDBContext.h"
#include "DBHelper.h"
#include "ClientSession.h"


//todo: CreatePlayerDataContext 구현
bool CreatePlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;

	/*
	dbHelper.BindParamText( mPlayerName );

	dbHelper.BindResultColumnInt( &mPlayerId );

	if ( dbHelper.Execute( SQL_CreatePlayer ) )
	{
		if ( dbHelper.FetchRow() )
		{
			return true;
		}
	}
	*/

	dbHelper.BindParamText(mPlayerName);
	dbHelper.BindResultColumnInt( &mPlayerId );

	if (dbHelper.Execute(SQL_CreatePlayer))
	{
		if (dbHelper.FetchRow())
		{
			/// 적용받은 행이 하나도 없다면, 실패라고 간주하자
			return mPlayerId != -1;
		}
	}

	return false;
}

void CreatePlayerDataContext::OnSuccess()
{
	// 뭐할까
	// id update 해줘야 되는데
	// 함수가 없네
	// db 작업 되는지만 확인하면 되니까 그냥 static 변수값 사용
	mSessionObject->mPlayer->ResponseRegisterPlayer( mPlayerId );
	// printf("create player\n");
}

void CreatePlayerDataContext::OnFail()
{
	EVENT_LOG( "CreatePlayerDataContext fail", mPlayerId );
}
// DONE

//todo: DeletePlayerDataContext 구현
bool DeletePlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;
	/*
	dbHelper.BindParamInt( &mPlayerId );

	if ( dbHelper.Execute( SQL_DeletePlayer ) )
	{
		if ( dbHelper.FetchRow() )
		{
			return true;
		}
	}
	*/

	int result = 0;

	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_DeletePlayer))
	{
		if (dbHelper.FetchRow())
		{
			/// 적용받은 행이 하나도 없다면, 실패라고 간주하자
			return result != 0;
		}
	}


	return false;
}

void DeletePlayerDataContext::OnSuccess()
{
	mSessionObject->mPlayer->ResponseDeregisterPlayer( mPlayerId );
	// printf( "delete player\n" );
}

void DeletePlayerDataContext::OnFail()
{
	EVENT_LOG( "DeletePlayerDataContext fail", mPlayerId );
}
// DONE

bool LoadPlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;

	dbHelper.BindParamInt(&mPlayerId);

	dbHelper.BindResultColumnText(mPlayerName, MAX_NAME_LEN);
	dbHelper.BindResultColumnFloat(&mPosX);
	dbHelper.BindResultColumnFloat(&mPosY);
	dbHelper.BindResultColumnFloat(&mPosZ);
	dbHelper.BindResultColumnBool(&mIsValid);
	dbHelper.BindResultColumnText(mComment, MAX_COMMENT_LEN);

	if (dbHelper.Execute(SQL_LoadPlayer))
	{
		if (dbHelper.FetchRow())
		{
			return true;
		}
	}

	return false;
}

void LoadPlayerDataContext::OnSuccess()
{
	//todo: 플레이어 로드 성공시 처리하기
	mSessionObject->mPlayer->ResponseLoad( mPlayerId, mPosX, mPosY, mPosZ, mIsValid, mPlayerName, mComment );
	// DONE
}

void LoadPlayerDataContext::OnFail()
{
	EVENT_LOG("LoadPlayerDataContext fail", mPlayerId);
}


bool UpdatePlayerPositionContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;

	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamFloat(&mPosX);
	dbHelper.BindParamFloat(&mPosY);
	dbHelper.BindParamFloat(&mPosZ);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerPosition))
	{
		if (dbHelper.FetchRow())
		{
			return result != 0;
		}
	}

	return false;
}

void UpdatePlayerPositionContext::OnSuccess()
{
	mSessionObject->mPlayer->ResponseUpdatePosition(mPosX, mPosY, mPosZ);
}




bool UpdatePlayerCommentContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;
	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamText(mComment);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerComment))
	{
		if (dbHelper.FetchRow())
		{
			return result != 0;
		}
	}

	return false;
}

void UpdatePlayerCommentContext::SetNewComment(const wchar_t* comment)
{
	wcscpy_s(mComment, comment);
}

void UpdatePlayerCommentContext::OnSuccess()
{
	mSessionObject->mPlayer->ResponseUpdateComment(mComment);
}



bool UpdatePlayerValidContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;

	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamBool(&mIsValid);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerValid))
	{
		if (dbHelper.FetchRow())
		{
			return result !=0 ;
		}
	}

	return false;
}

void UpdatePlayerValidContext::OnSuccess()
{
	mSessionObject->mPlayer->ResponseUpdateValidation(mIsValid);
}


