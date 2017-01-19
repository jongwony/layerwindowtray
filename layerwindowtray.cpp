// layerwindowtray.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "layerwindowtray.h"
#include <shellapi.h>
#include <regex>
//#include <stdio.h>

#define MAX_LOADSTRING 100

// ���� ����:
HINSTANCE hInst;                                // ���� �ν��Ͻ��Դϴ�.
TCHAR szTitle[MAX_LOADSTRING];                  // ���� ǥ���� �ؽ�Ʈ�Դϴ�.
TCHAR szWindowClass[MAX_LOADSTRING];            // �⺻ â Ŭ���� �̸��Դϴ�.
HWND hWndOld, hWndActive;						//hWndCapture;// Ȱ��ȭ�� â�Դϴ�.
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;	// ����� ���� ���
POINT gpt;										// ���콺 ������ ��ġ
int ACTIVE = 90;
int INACTIVE = 50;


// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
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

	// TODO: ���⿡ �ڵ带 �Է��մϴ�.
	// ����� �ܼ�
	//AllocConsole();
	//errno_t err;
	//FILE *stream;
	//err = freopen_s(&stream, "CONOUT$", "w+", stdout);

	// ���� ���ڿ��� �ʱ�ȭ�մϴ�.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_LAYERWINDOWTRAY, szWindowClass, MAX_LOADSTRING);

	// �ߺ� ���� ����
	hWndOld = FindWindow(szWindowClass, szTitle);
	if (hWndOld != NULL)
	{
		AddNotificationPopup(hWndOld);
		SetForegroundWindow(hWndOld);
		SendMessage(hWndOld, WMAPP_NOTIFYCALLBACK, 0, WM_CONTEXTMENU);
		return 0;
	}

	MyRegisterClass(hInstance);

	// ���� ���α׷� �ʱ�ȭ�� �����մϴ�.
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAYERWINDOWTRAY));

	MSG msg;

	// �⺻ �޽��� �����Դϴ�.
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

// Ŀ���� â �ȿ� �ִ���
BOOL IsCurInRect(RECT win) {
	BOOL result = TRUE;
	result &= win.left < gpt.x;
	result &= gpt.x < win.right;
	result &= win.top < gpt.y;
	result &= gpt.y < win.bottom;
	return result;
}

// Ÿ�̸Ӹ��� ȣ��Ǵ� �ݹ��Լ�
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lparam) {
	// �� ���̴� â ���͸�
	if (!IsWindowVisible(hWnd)) {
		return TRUE;
	}
	// �θ� ����ȭ������
	if (GetParent(hWnd) != 0) {
		return TRUE;
	}
	// �ּ�ȭ�� â �ǳʶٱ�
	if (IsIconic(hWnd)) {
		return TRUE;
	}
	// ���콺 ���� ã��
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

// �� â�� �ݾ����� ������� �ǵ����� �ݹ��Լ�
BOOL CALLBACK EnumWindowsProcBack(HWND hWnd, LPARAM lparam) {
	// �� ���̴� â ���͸�
	if (!IsWindowVisible(hWnd)) {
		return TRUE;
	}
	// �θ� ����ȭ������
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
//  �Լ�: MyRegisterClass()
//
//  ����: â Ŭ������ ����մϴ�.
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
//   �Լ�: InitInstance(HINSTANCE, int)
//
//   ����: �ν��Ͻ� �ڵ��� �����ϰ� �� â�� ����ϴ�.
//
//   ����:
//
//        �� �Լ��� ���� �ν��Ͻ� �ڵ��� ���� ������ �����ϰ�
//        �� ���α׷� â�� ���� ���� ǥ���մϴ�.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

					   // TODO: Make this window substitute for Tray Icon with background process.
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0, 100, 0, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	// �� â ������
	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	return TRUE;
}

// ���� ��ȭ ������ �޽��� ó�����Դϴ�.
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

// Ʈ���� �������� ����ϴ�.
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
	lstrcpy(nid.szInfo, TEXT("â ����ȭ ���α׷��� �������Դϴ�."));

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL DeleteNotificationIcon()
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_GUID;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

// �˾� �޽��� ó�����Դϴ�.
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
//  �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ����:  �� â�� �޽����� ó���մϴ�.
//
//  WM_COMMAND  - ���� ���α׷� �޴��� ó���մϴ�.
//  WM_DESTROY  - ���� �޽����� �Խ��ϰ� ��ȯ�մϴ�.
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
		// �޴� ������ ���� �м��մϴ�.
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
