// loggui.h - override LogBase to provide gui notifications
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#pragma once

// At startup, we create a log with eg CLogBase::SetLog(new CLogFile("c:\\whatever.log"));
// then in code we can CLogBase::GetLog->Log("error: problem %d", getlasterror());

class CLogGUI: public CLogBase
{
private:
	HWND hWndLog;

public:
	CLogGUI(HWND );
	virtual void WriteLog(const char *, ...);
	virtual ~CLogGUI(void);
};
