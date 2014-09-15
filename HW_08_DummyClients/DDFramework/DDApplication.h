#pragma once

/*
	작성자 : 최경욱
	작성일 : 2014. 4. 4
	내용 : 프레임웍 기반이 되는 application (NNGameFramework와 거의 동일)
*/

#include "DDConfig.h"

// class DDRenderer;
// class DDSceneDirector;

class DDApplication : public Singleton<DDApplication>
{
public:
	// agebreak : 자주 쓰는 싱글톤 패턴을 템플릿화 시켜서 간단히 쓸 수 있는 테크닉이 있음. 은자림팀의 코드를 참고 하도록
	// 싱글톤 템플릿을 public 상속받는방법으로 수정함
	DDApplication();
	~DDApplication();

	bool Init( std::wstring title, int width, int height );
	bool Release();
	int Run();

	int GetScreenWidth() const	{ return m_ScreenWidth; }
	int GetScreenHeight() const	{ return m_ScreenHeight; }
	HWND GetHWND() const		{ return m_Hwnd; }

private:

	static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	bool _CreateWindow( std::wstring title, int width, int height );
	//bool _CreateRenderer();
	void ComputeFPS();

	// agebreak : 싱글톤으로 만들어 놓고, 왜 굳이 멤버 변수가 필요한가?
	// 싱글톤 객체에 직접 접근하는 방식으로 변경(getinstance비용은 있겠으나 직관적이도록...)
// 	std::shared_ptr<DDRenderer>			m_pRenderer;
// 	std::shared_ptr<DDSceneDirector>	m_pSceneDirector;

	// 프로그램 윈도우 핸들
	HWND m_Hwnd;

	// 프로그램 코드 메모리의 핸들
	HINSTANCE m_hInstance;

	// agebreak : VS 2013부터 아래와 같이 초기화 가능 (C++0x)
	//wchar_t* m_pTitle = nullptr;
	int m_ScreenWidth = 0;
	int m_ScreenHeight = 0;

	float m_Fps = 0.0f;
	float m_FpsTimer = 0.0f;
	float m_ElapsedTime = 0.0f;
	float m_DeltaTime = 0.0f;

	int m_PrevTime = 0;
	int m_NowTime = 0;
	int m_FrameCount = 0;

	bool m_DestroyWindow = false;

};

