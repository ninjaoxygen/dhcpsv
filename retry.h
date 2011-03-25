// retry.h - Incremental backoff retry controller
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#pragma once

class CRetry
{
private:
	DWORD dwDelayOrig;
	DWORD dwDelay;


public:
	CRetry(DWORD dwDelay)
	{
		dwDelayOrig = dwDelay;
	}


};
