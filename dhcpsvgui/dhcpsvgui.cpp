// dhcpsvgui.cpp : GUI host for dhcpsv
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#include "stdafx.h"
#include "dhcpsvgui.h"

#include "SystemTraySDK.h"

#include "../dhcpsv.h"
#include "../LogBase.h"
#include "LogGUI.h"

// Using v6 common controls needs a manifest (XP SP1 and above)
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

#define	WM_ICON_NOTIFY WM_APP+10

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

TCHAR sIP[32] = "";

CDhcpServer s;
CSystemTray TrayIcon;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DlgCIP(HWND, UINT, WPARAM, LPARAM);

BOOL	bServerActive = FALSE;
HWND	hWndMain = NULL;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DHCPSVGUI, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DHCPSVGUI));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWndMain, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DHCPSVGUI));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DHCPSVGUI);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_COMPOSITED, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	//hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	hWndMain = hWnd;

	if (!hWnd)
	{
		return FALSE;
	}

	// Create the tray icon
	if (!TrayIcon.Create(hInstance,
		hWnd,                            // Parent window
		WM_ICON_NOTIFY,                  // Icon notify message to use
		_T("dhcpsv"),  // tooltip
		::LoadIcon(hInstance, (LPCTSTR)IDI_DHCPSVGUI),
		IDC_DHCPSVGUI)) 
		return FALSE;

	TrayIcon.SetIconList(IDI_ICON_GREEN, IDI_ICON_RED);

//	ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	return TRUE;
}

DWORD WINAPI DhcpThreadProc(LPVOID lpParameter)
{
	__try
	{
		s.Initialize();

	//	s.SetConf("server", argv[v]);
	//	s.SetConf("subnet", argv[v]);
		s.SetConf("client", sIP);
		s.SetConf("retry", 1);
		s.SetConf("verbose", 3);

		if (s.Start() == 0)
		{
			s.Run();
		}

		//-----------------------------------------------
		// Clean up and exit.
		s.Cleanup();
	}
	__finally
	{
		bServerActive = FALSE;
	}

	return 0;
}


HWND hEditMsg = NULL;

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
//	PAINTSTRUCT ps;
//	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		{
			LoadLibrary(_T("Msftedit.dll"));
// | WS_EX_CONTROLPARENT
			hEditMsg = CreateWindowEx(WS_EX_CLIENTEDGE, "RICHEDIT50W", 0,
				WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_TABSTOP | WS_VSCROLL,
				8, 140, 256, 130,
				hWnd, (HMENU)0, GetModuleHandle(0), 0);

			HFONT hFont = CreateFont (13, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, 
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
				DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"));

			SendMessage(hEditMsg, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			ShowWindow(hEditMsg, SW_SHOW);

			RegisterHotKey(hWnd, 137, MOD_WIN, 'Y');

			CLogBase::SetLog(new CLogGUI(hWnd));
		}
		break;

	case WM_HOTKEY:
		if (wParam == 137)
		{
			if (bServerActive)
			{
				SendMessage(hWnd, WM_COMMAND, IDM_SERVERSTOP, (LPARAM)0);
			}
			else
			{
				SendMessage(hWnd, WM_COMMAND, IDM_SERVERSTART, (LPARAM)0);
			}
		}
		break;

    case WM_ICON_NOTIFY:
        return TrayIcon.OnTrayNotification(wParam, lParam);

	case WM_COPYDATA:
		{
			COPYDATASTRUCT *cs = (COPYDATASTRUCT *)lParam;

			CHARRANGE cr;
			cr.cpMin = -1;
			cr.cpMax = -1;

			if (strcmp((LPCTSTR)cs->lpData, _T("ON\n")) == 0)
			{
				bServerActive = TRUE;
				TrayIcon.SetIcon(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_GREEN)));
				TrayIcon.ShowBalloon(_T("DHCP Server Started"), _T("DHCP Server"), NIIF_INFO);
			}

			if (strcmp((LPCTSTR)cs->lpData, _T("OFF\n")) == 0)
			{
				bServerActive = FALSE;
				TrayIcon.SetIcon(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DHCPSVGUI)));
				TrayIcon.ShowBalloon(_T("DHCP Server Stopped"), _T("DHCP Server"), NIIF_INFO);
			}

			if (strncmp((LPCTSTR)cs->lpData, _T("LEASE:"), 7) == 0)
				TrayIcon.ShowBalloon(((LPCTSTR)cs->lpData) + 7, _T("DHCP Server"), NIIF_INFO);

			SendMessage(hEditMsg, EM_EXSETSEL, 0, (LPARAM)&cr);
			SendMessage(hEditMsg, EM_REPLACESEL, 0, (LPARAM)cs->lpData);
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_SHOW:
			ShowWindow(hWnd, SW_RESTORE);
			break;

		case IDM_SERVERSTART:
			if (!bServerActive)
			{
				if (strlen(sIP) == 0)
				{
					INT_PTR rv = DialogBox(hInst, MAKEINTRESOURCE(IDD_CIP_DIALOG), hWnd, DlgCIP);
					if (rv == IDOK)
					{
						CreateThread(NULL, 0, DhcpThreadProc, NULL, 0, NULL);
					}
				}
				else
				{
					CreateThread(NULL, 0, DhcpThreadProc, NULL, 0, NULL);
				}
			}
			break;

		case IDM_SERVERSTOP:
			{
				s.Terminate();
			}
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

/*	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;*/

	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:		
		SetWindowPos(hEditMsg, NULL, 
			8,									HIWORD(lParam) / 2,
			max(16, LOWORD(lParam) - 16),		HIWORD(lParam) / 2 - 8, SWP_NOACTIVATE | SWP_NOZORDER);

		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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

// Message handler for IP entry box.
INT_PTR CALLBACK DlgCIP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			GetDlgItemText(hDlg, IDC_CLIENTIPADDRESS, sIP, sizeof(sIP));
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
