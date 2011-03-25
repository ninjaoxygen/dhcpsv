// loggui.h - override LogBase to provide gui notifications
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#include "StdAfx.h"
#include <stdarg.h>
#include <stdio.h>

#include "../LogBase.h"
#include "LogGUI.h"

CLogGUI::CLogGUI(HWND hWnd) : hWndLog(hWnd)
{
}

CLogGUI::~CLogGUI(void)
{
}

void CLogGUI::WriteLog(const char *format, ...)
{
    char buf[1024] = "";
    va_list va;
    va_start(va, format);
    _vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
	COPYDATASTRUCT cs;
	cs.dwData = 1010;
	cs.lpData = buf;
	cs.cbData = strlen(buf) + 1;
	SendMessage(hWndLog, WM_COPYDATA, (WPARAM)0, (LPARAM)&cs);
}
