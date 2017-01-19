// layerwindowtray.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "layerwindowtray.h"
#include <shellapi.h>
#include <regex>
//#include <stdio.h>

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
TCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
TCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND hWndOld, hWndActive;						//hWndCapture;// 활성화된 창입니다.
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;	// 사용자 정의 명령
POINT gpt;										// 마우스 포인터 위치
int ACTIVE = 90;
int INACTIVE = 50;


// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK		EnumWindowsProc(HWND hWnd, LPARAM lparam);
BOOL CALLBACK		EnumWindowsProcBack(HWND hWnd, LPARAM lparam);
BOOL			AddNotificationIcon(HWND hWnd);
BOOL			AddNotificationPopup(HWND hWnd);
BOOL			DeleteNotificationIcon(void);
void			ShowContextMenu(HWND hWnd, POINT pt);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.
	// 디버깅 콘솔
	//AllocConsole();
	//errno_t err;
	//FILE *stream;
	//err = freopen_s(&stream, "CONOUT$", "w+", stdout);

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_LAYERWINDOWTRAY, szWindowClass, MAX_LOADSTRING);

	// 중복 실행 방지
	hWndOld = FindWindow(szWindowClass, szTitle);
	if (hWndOld != NULL)
	{
		AddNotificationPopup(hWndOld);
		SetForegroundWindow(hWndOld);
		SendMessage(hWndOld, WMAPP_NOTIFYCALLBACK, 0, WM_CONTEXTMENU);
		return 0;
	}

	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAYERWINDOWTRAY));

	MSG msg;

	// 기본 메시지 루프입니다.
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

// 커서가 창 안에 있는지
BOOL IsCurInRect(RECT win) {
	BOOL result = TRUE;
	result &= win.left < gpt.x;
	result &= gpt.x < win.right;
	result &= win.top < gpt.y;
	result &= gpt.y < win.bottom;
	return result;
}

// 타이머마다 호출되는 콜백함수
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lparam) {
	// 안 보이는 창 필터링
	if (!IsWindowVisible(hWnd)) {
		return TRUE;
	}
	// 부모가 바탕화면인지
	if (GetParent(hWnd) != 0) {
		return TRUE;
	}
	// 최소화된 창 건너뛰기
	if (IsIconic(hWnd)) {
		return TRUE;
	}
	// 마우스 영역 찾기
	RECT rt;
	GetWindowRect(hWnd, &rt);	

	try
	{
		// set WS_EX_LAYERED on this Window
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		// Current active windows ALPHA ACTIVE%, else windows INACTIVE%
		if (hWnd == hWndActive || IsCurInRect(rt)) {
			SetLayeredWindowAttributes(hWnd, 0, (255 * ACTIVE) / 100, LWA_ALPHA);
		}
		else {
			SetLayeredWindowAttributes(hWnd, 0, (255 * INACTIVE) / 100, LWA_ALPHA);
		}
	}
	catch (...)
	{
		return FALSE;
	}
	return TRUE;
}

// 주 창을 닫았을때 원래대로 되돌리는 콜백함수
BOOL CALLBACK EnumWindowsProcBack(HWND hWnd, LPARAM lparam) {
	// 안 보이는 창 필터링
	if (!IsWindowVisible(hWnd)) {
		return TRUE;
	}
	// 부모가 바탕화면인지
	if (GetParent(hWnd) != 0) {
		return TRUE;
	}
	try 
	{
		// Remove WS_EX_LAYERED from this Window styles
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | ~WS_EX_LAYERED);
		// Current active windows ALPHA rollback 
		SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
		// Ask the window and its children to repaint
		RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
	}
	catch (...)
	{
		return FALSE;
	}
	return TRUE;
}

//
//  함수: MyRegisterClass()
//
//  목적: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAYERWINDOWTRAY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = szWindowClass;
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAYERWINDOWTRAY);
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   목적: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   설명:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

					   // TODO: Make this window substitute for Tray Icon with background process.
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0, 100, 0, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	// 주 창 가리기
	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	return TRUE;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 트레이 아이콘을 만듭니다.
BOOL AddNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	// add the icon, setting the icon, tooltip, and callback message.
	// the icon will be identified with the GUID
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDC_LAYERWINDOWTRAY));
	LoadString(hInst, IDC_LAYERWINDOWTRAY, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL AddNotificationPopup(HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uFlags = NIF_INFO;
	nid.uTimeout = 200;
	nid.dwInfoFlags = NIIF_INFO;
	lstrcpy(nid.szInfoTitle, TEXT("LayerWindowTray"));
	lstrcpy(nid.szInfo, TEXT("창 투명화 프로그램이 실행중입니다."));

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL DeleteNotificationIcon()
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_GUID;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 팝업 메시지 처리기입니다.
void ShowContextMenu(HWND hWnd, POINT pt)
{
	HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_LAYERWINDOWTRAY));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hWnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL);
		}
		DestroyMenu(hMenu);
	}
}


//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  목적:  주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 응용 프로그램 메뉴를 처리합니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		// Initialization
		// Get Current Active Window
		hWndActive = GetForegroundWindow();
		// All Windows Iteration
		EnumWindows(EnumWindowsProc, NULL);
		// Old Pointer
		hWndOld = GetForegroundWindow();

		// add the notification icon
		if (!AddNotificationIcon(hWnd))
		{
			MessageBox(hWnd,
				L"Please read the ReadMe.txt file for troubleshooting",
				L"Error adding icon", MB_OK);
			return -1;
		}

		// add the notification popup
		AddNotificationPopup(hWnd);


		SetTimer(hWnd,						// handle to main window 
			1,								// timer identifier 
			200,							// 0.2-second interval 
			(TIMERPROC)NULL);               // no timer callback 
	}
	break;
	case WM_COMMAND:
	{

		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다.
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_A20:
			ACTIVE = 20;
			break;
		case ID_A30:
			ACTIVE = 30;
			break;
		case ID_A40:
			ACTIVE = 40;
			break;
		case ID_A50:
			ACTIVE = 50;
			break;
		case ID_A60:
			ACTIVE = 60;
			break;
		case ID_A70:
			ACTIVE = 70;
			break;
		case ID_A80:
			ACTIVE = 80;
			break;
		case ID_A90:
			ACTIVE = 90;
			break;
		case ID_A95:
			ACTIVE = 95;
			break;
		case ID_IN10:
			INACTIVE = 10;
			break;
		case ID_IN20:
			INACTIVE = 20;
			break;
		case ID_IN30:
			INACTIVE = 30;
			break;
		case ID_IN40:
			INACTIVE = 40;
			break;
		case ID_IN50:
			INACTIVE = 50;
			break;
		case ID_IN60:
			INACTIVE = 60;
			break;
		case ID_IN70:
			INACTIVE = 70;
			break;
		case ID_IN80:
			INACTIVE = 80;
			break;
		case ID_IN90:
			INACTIVE = 90;
			break;
		case IDM_EXIT:
			EnumWindows(EnumWindowsProcBack, NULL);
			KillTimer(hWnd, 1);
			DeleteNotificationIcon();
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_TIMER:
	{
		// Get Current Active Window
		hWndActive = GetForegroundWindow();

		// Get Mouse Capture Window
		GetCursorPos(&gpt);
		//hWndCapture = WindowFromPoint(gpt);

		// Optimization
		//if (hWndOld != hWndActive) {
		// All Windows Iteration
		EnumWindows(EnumWindowsProc, NULL);

		// Old Pointer
		hWndOld = hWndActive;
		//}
	}
	break;
	case WMAPP_NOTIFYCALLBACK:
	{
		switch (lParam)
		{
		case WM_CONTEXTMENU:
		{
			//GetCursorPos(&gpt);
			ShowContextMenu(hWnd, gpt);
		}
		break;
		}
	}
	break;
	case WM_DESTROY:
		EnumWindows(EnumWindowsProcBack, NULL);
		KillTimer(hWnd, 1);
		DeleteNotificationIcon();
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
