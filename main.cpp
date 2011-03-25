// main.cpp - Console host for dhcpsv, a small DHCP server
//
// Copyright (C) 2010 - 2011 Chris Poole
//

#pragma warning(4995,disable)
#include "stdafx.h"

#include "dhcpsv.h"

CDhcpServer s;

void showhelp()
{
	printf("Usage:\n\n  dhcpsv -c 192.168.1.210 [-s 192.168.1.181] [-n 255.255.255.0]\n\n    -c first client IP\n    -s server IP\n    -n subnet mask\n\n    -r retry every 10s if we fail or cannot start up\n\n    -v verbose\n    -vv very verbose, includes all dhcp option dumps\n    -vvv show me packet dumps\n\n");
}

void showbanner()
{
	printf("\n  dhcpsv v0.1 alpha\n\n  Copyright 2010 Chris Poole chris@hackernet.co.uk\n\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	showbanner();

	if (argc < 2)
	{
		showhelp();
		return 1;
	}

	s.Initialize();

	for (int v = 1; v < argc; v++)
	{
		if (
				(strcmp(argv[v], "-?") == 0) ||
				(strcmp(argv[v], "-h") == 0) ||
				(strcmp(argv[v], "--help") == 0)
			)
		{
			showhelp();
			return 1;
		}

		if (strcmp(argv[v - 1], "-s") == 0)
		{
			s.SetConf("server", argv[v]);
//			printf("Command line: server address set to %08X\n", conf.server);
		}
		if (strcmp(argv[v - 1], "-n") == 0) // eg 255.255.255.0
		{
			s.SetConf("subnet", argv[v]);
//			printf("Command line: netmask set to %08X\n", conf.subnet);
		}
		if (strcmp(argv[v - 1], "-c") == 0)
		{
			s.SetConf("client", argv[v]);
//			printf("Command line: first client IP set to %08X\n", conf.client);
		}

		if (strcmp(argv[v], "-r") == 0)
		{
			s.SetConf("retry", 1);
		}

		if (strcmp(argv[v], "-v") == 0)
		{
			s.SetConf("verbose", 1);
		}

		if (strcmp(argv[v], "-vv") == 0)
		{
			s.SetConf("verbose", 2);
		}

		if (strcmp(argv[v], "-vvv") == 0)
		{
			s.SetConf("verbose", 3);
		}
	}

	if (s.Start() == 0)
	{
		s.Run();
	}

	//-----------------------------------------------
	// Clean up and exit.
	printf("exiting\n");
	s.Cleanup();
	return 0;
}

