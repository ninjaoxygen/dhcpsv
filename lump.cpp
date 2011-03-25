// lump.cpp - For safely manipulating a lump of data, ie a packet
//
// Copyright (C) 2011 Chris Poole - chris@hackernet.co.uk
//

#include "stdafx.h"

// for u_char etc
#include <winsock2.h>

#include "lump.h"

CLump::CLump()
{
	bad = false;
	len = 0;
}

void CLump::Reset()
{
	bad = false;
	len = 0;
}

void CLump::AddUchar(u_char val)
{
	if ((len + sizeof(val)) >= LUMPSIZE) { bad = true; return; };
	buf[len] = val;
	len += sizeof(val);
}

void CLump::AddUshort(u_short val)
{
	if ((len + sizeof(val)) >= LUMPSIZE) { bad = true; return; };
	*((u_short *) &buf[len]) = val;
	len += sizeof(val);
}

void CLump::AddUlong(u_long val)
{
	if ((len + sizeof(val)) >= LUMPSIZE) { bad = true; return; };
	*((u_long *) &buf[len]) = htonl(val);
	len += sizeof(val);
}

void CLump::AddLump(CLump *pLump)
{
	if ((len + pLump->len) >= LUMPSIZE) { bad = true; return; };

	memcpy(&buf[len], &pLump->buf, pLump->len);
	len += pLump->len;
}

void CLump::AddMAC(void *pMAC)
{
	if ((len + 6) >= LUMPSIZE) { bad = true; return; };
	memcpy(&buf[len], pMAC, 6);
	len += 6;
}

void CLump::AddPadded(void *data, int datalen, int padlen)
{
	if (datalen > padlen) { bad = true; return; };
	if ((len + padlen) >= LUMPSIZE) { bad = true; return; };
	if (data)
	{
		memcpy(&buf[len], data, datalen);
		memset(&buf[len + datalen], 0, padlen - datalen);
	}
	else
	{
		memset(&buf[len], 0, padlen);
	}
	len += padlen;
}

void CLump::AddOptChar(char opt, char val)
{
	if ((len + 3) >= LUMPSIZE) { bad = true; return; };
	buf[len++] = opt;
	buf[len++] = 1; // data len
	buf[len++] = val;
}

void CLump::AddOptUlong(char opt, u_long val)
{
	if ((len + 6) >= LUMPSIZE) { bad = true; return; };
	buf[len++] = opt;
	buf[len++] = 4;
	*((DWORD *)(buf + len)) = htonl(val);
	len += 4;
}
