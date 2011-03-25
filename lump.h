// lump.h - For safely manipulating a lump of data, ie a packet
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#pragma once

class CLump
{
public:
	static const int LUMPSIZE = 1024;
	u_char buf[LUMPSIZE];
	int len;
	bool bad;

	CLump();
	void Reset();
	void AddUchar(u_char val);
	void AddUshort(u_short val);
	void AddUlong(u_long val);
	void AddLump(CLump *pLump);
	void AddMAC(void *pMAC);
	void AddPadded(void *data, int datalen, int padlen);
	void AddOptChar(char opt, char val);
	void AddOptUlong(char opt, u_long val);
};
