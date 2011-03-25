// logbase.h - base class for logging system
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#pragma once

// At startup, we create a log with eg CLogBase::SetLog(new CLogFile("c:\\whatever.log"));
// then in code we can CLogBase::GetLog->Log("error: problem %d", getlasterror());

class CLogBase
{
protected:
	static CLogBase *pLog;

public:
	CLogBase(void);

	static inline CLogBase* GetLog()
	{
		return pLog;
	}

	static inline void SetLog(CLogBase *pNewLog)
	{
		if (pLog)
		{
			delete pLog;
		}

		pLog = pNewLog;
	}

	virtual void WriteLog(const char *_Format, ...)
	{
	}

	virtual ~CLogBase(void);
};


#define Log (CLogBase::GetLog()->WriteLog)