// dhcpsv.h - interface for dhcpsv
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#pragma once

class CLogBase;
class CServerConfig;

class CDhcpServer
{
public:
	static const int PortDhcp = 67;
	static const int PortInfo = 68;
	static const int RBufLen = 1024;

	int Initialize(); // do any one time initialization
	int Validate(); // validate parameters, also called automatically by Start to validate before starting
	int Cleanup(); // do any one time teardown

	int Start(); // attempt a start, includes validating params
	int Run(); // main loop, returns when server exits

	int Terminate(); // request termination of Run function

	int SetConf(char *sEntry, unsigned long ulValue);
	int SetConf(char *sEntry, char *szValue);

	int SetLog(CLogBase *pLog);

private:

	CLogBase *pLog;

	bool exitflag;

	int readoptions(char *pkt, void *buf, int len);
	int locateadapterconf();
};
