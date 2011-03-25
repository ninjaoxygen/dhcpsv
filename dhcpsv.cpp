// dhcpsv.cpp : A small DHCP server, console or gui
//
// Copyright (C) 2010 - 2011 Chris Poole
//

#include "stdafx.h"
#include <time.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <strsafe.h>

#include <string>
#include <map>
#include <sstream>

#include <winerror.h>

#include "lump.h"
#include "dhcpsv.h"
#include "logbase.h"

using namespace std;

// for GetAdaptersInfo
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

WSADATA wsaData;
SOCKET RecvSocket;		// used to receive our DHCP requests
SOCKET RecvInfoSocket;  // used to watch for other servers responding
sockaddr_in RecvAddr;
char RecvBuf[CDhcpServer::RBufLen];
int  BufLen;
sockaddr_in SenderAddr;
int SenderAddrSize;

// Utility functions
int wildcmp(const char *wild, const char *string)
{
	// Written by Jack Handy - jakkhandy@hotmail.com
	const char *cp = NULL, *mp = NULL;

	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		wild++;
		string++;
	}

	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
	{
		wild++;
	}
	return !*wild;
}

// TODO: replace the printf in here with single line consolidation then Log
void dumphex(void *buf, int nlen)
{
	int i;
	int j;
	int k = 0;

	unsigned char *cbuf = (unsigned char *)buf;

	for (i = 0; i < nlen; i += 16)
	{
		printf("%04X ", i);
		for (j = 0; j < 16; j++)
		{
			if (k < nlen)
			{
				printf("%02X ", (int)(cbuf[k]));
			}
			else
			{
				printf("   ");
			}

			if (j == 7)
				printf(" ");

			k++;
		}
		k -= 16;
		for (j = 0; j < 16; j++)
		{
			if (k < nlen)
			{
				if (cbuf[k] == 0) printf(".");
				else if ((cbuf[k] >= 32) && (cbuf[k] < 128))
					printf("%c", (int)(cbuf[k]));
				else
					printf(":");
			}
			else
			{
				printf(" ");
			}
			k++;
		}

		printf("\n");
	}
}

string ip2str(DWORD dwIP)
{
	char szTmp[16];
	_snprintf(szTmp, 16, "%d.%d.%d.%d", (dwIP >> 24) & 0xFF, (dwIP >> 16) & 0xFF, (dwIP >> 8) & 0xFF, dwIP & 0xFF);
	return szTmp;
}

CLump resp;

class CLeaseInfo
{
public:
	DWORD IP;
	time_t lastseen;

	CLeaseInfo()
	{
		IP = 0;
		lastseen = 0;
	}

	CLeaseInfo(DWORD dwIP)
	{
		IP = dwIP;
		lastseen = time(0);
	}

	bool IsValid(DWORD dwIP)
	{
		if (IP != dwIP)
			return false;

		return true;
	}
};


class CLeasePool
{
	std::map<std::string, CLeaseInfo> leases;
public:
	bool AddLease(std::string mac, CLeaseInfo lease)
	{
		Log("Leased %s to %s", ip2str(lease.IP).c_str(), mac.c_str());
		leases[mac] = lease;
		return true;
	}

	bool GetLease(std::string mac, CLeaseInfo **lease)
	{
		map<string, CLeaseInfo>::iterator iter;

		*lease = NULL;

		iter = leases.find(mac);

		if(iter == leases.end())
		{
#ifdef DEBUG_LEASEPOOL
			cout << "Not in lease pool (" << mac << ")" << endl;
#endif
			//printf("Not in lease pool (%s)\n", mac);
			return false;
		}
		else
		{
#ifdef DEBUG_LEASEPOOL
			cout << "Found a lease for (" << mac << ") for address " << iter->second.IP << endl;
#endif
			//printf("Found a lease for (%s) is %08X\n", mac, iter->second);
			*lease = &iter->second;
			return true;
		}
	}

	bool CheckLease(std::string mac, DWORD dwIP)
	{
		CLeaseInfo *pLease = NULL;

		if (!GetLease(mac, &pLease))
			return false;

		return pLease->IsValid(dwIP);
	}

	bool RemoveLease(std::string mac, DWORD dwIP)
	{
		leases.erase(mac);
	}

	void ListLeases()
	{
		map<string, CLeaseInfo>::iterator iter;

		for( iter = leases.begin(); iter != leases.end(); ++iter)
		{
			Log("%s:%s", iter->first.c_str(), ip2str(iter->second.IP).c_str());
		}
	}
};

CLeasePool lp;

class CServerConfig
{
public:
	u_long subnet;
	u_long server;
	u_long client;
	u_long verbose;
	u_long retry;
};

CServerConfig conf;

int CDhcpServer::Initialize()
{
	int err;

	BufLen = RBufLen;
	SenderAddrSize = sizeof(SenderAddr);

	//-----------------------------------------------
	// Initialize Winsock
	err = WSAStartup(MAKEWORD(2,2), &wsaData);
	if ( err != 0 )
	{
		Log("error: WSAStartup failed (%d)\n", WSAGetLastError());
		return 1;
	}

	// Establish default config
	conf.server = 0;
	conf.subnet = 0;
	conf.client = 0;
	conf.verbose = 0;
	conf.retry = 0;

	return 0;
}

int CDhcpServer::Cleanup()
{
	WSACleanup();

	return 0;
}

int CDhcpServer::Validate()
{
	// Validate parameters
	if (conf.client == 0)
	{
		Log("error: no client IP address given\n");
		return 2;
	}

	do {
		if (locateadapterconf() == 0)
		{
			return 0;
		}

		Log("error: failed to locate an adapter matching the client address\n");

		// TODO: exit flag for validate
		if (conf.retry)
		{
			Log("validate: waiting to retry...\n");
			Sleep(10000);
			Log("validate: retrying now\n");
		}
	} while (conf.retry);

	return 1;
}

int CDhcpServer::Start()
{	
again:
	if (Validate() > 0)
	{
		Log("error: parameter validation failed, not starting\n");
		if (conf.retry)
		{
			Log("start: waiting to retry...\n");
			Sleep(10000);
			Log("start: retrying now\n");
			goto again;
		}
		else
		{
			return 2;
		}
	}

	exitflag = false;

	Log("\nstarting with config:\n");
	Log(" server IP    = %s\n", ip2str(conf.server).c_str());
	Log(" subnet mask  = %s\n", ip2str(conf.subnet).c_str());
	Log(" first client = %s\n", ip2str(conf.client).c_str());
	Log("\n");

	return 0;
}

void ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40)*sizeof(TCHAR)); 

    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf);

	Log((LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

int CDhcpServer::Run()
{
	Log("ON\n");
	//-----------------------------------------------
	// Create a receiver socket to receive datagrams
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	RecvInfoSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Set all sockets to non-blocking
	u_long nonblocking = 1;
	ioctlsocket(RecvSocket, FIONBIO, &nonblocking);
	ioctlsocket(RecvInfoSocket, FIONBIO, &nonblocking);

	//-----------------------------------------------
	// Bind the socket to any address and the specified port.
	memset(&RecvAddr, 0, sizeof(RecvAddr));
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(PortDhcp);
//	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	RecvAddr.sin_addr.s_addr = htonl(conf.server);

	do
	{
		__try
		{
			int rv;

			Log("Binding request listener socket\n");
			rv = bind(RecvSocket, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
			if (rv != 0)
			{
				Log("error: RecvSocket bind failed: %d\n", WSAGetLastError());
				ErrorExit("RecvSocket bind");
				__leave;
			}

			Log("Binding info listener socket\n");

			memset(&RecvAddr, 0, sizeof(RecvAddr));
			RecvAddr.sin_family = AF_INET;
			RecvAddr.sin_port = htons(PortInfo);
			//RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			RecvAddr.sin_addr.s_addr = htonl(conf.server);

			rv = bind(RecvInfoSocket, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
			if (rv != 0)
			{
				Log("error: RecvInfoSocket bind failed: %d\n", WSAGetLastError());
//				ErrorExit("RecvInfoSocket bind");
//				__leave;
			}

			// Configure each socket
			BOOL optval;
			if (setsockopt(RecvSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
			{
				Log("setsockopt failed: setting SO_BROADCAST on RecvSocket error %d\n", WSAGetLastError());
				__leave;
			}

			if (setsockopt(RecvInfoSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
			{
				Log("setsockopt failed: setting SO_BROADCAST on RecvInfoSocket error %d\n", WSAGetLastError());
				__leave;
			}


			// Prepare an fd_set to wait on
			fd_set listeners; // all our listening sockets to check for read
			fd_set empty1;	// an empty fd_set to pass
			fd_set empty2;	// an empty fd_set to pass
			FD_ZERO(&listeners);
			FD_ZERO(&empty1);
			FD_ZERO(&empty2);

			FD_SET(RecvSocket, &listeners);
			//FD_SET(RecvInfoSocket, &listeners);
			timeval timeout;

			// Wake every second to check for exit etc
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			//-----------------------------------------------
			// Call the recvfrom function to receive datagrams
			// on the bound socket.
			Log("Receiving datagrams...\n");
			int nsel;
			while(exitflag == 0)
			{
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;

				FD_ZERO(&listeners);
				FD_SET(RecvSocket, &listeners);
				FD_ZERO(&empty1);
				FD_ZERO(&empty2);

				nsel = select(listeners.fd_count, &listeners, &empty1, &empty2, &timeout);

				switch(nsel)
				{
				case 0:
					// just a timeout
					break;

				case SOCKET_ERROR:
						Log("select error: %d\n", WSAGetLastError());
						ErrorExit("select");
						break;

				default:
					{
						// check each socket
						int l;
						
						// Check main dhcp server socket
						l = recvfrom(
							RecvSocket, 
							RecvBuf, 
							BufLen, 
							0, 
							(SOCKADDR *)&SenderAddr, 
							&SenderAddrSize
							);

						switch(l)
						{
						case 0: break; // no data
						case SOCKET_ERROR: Log("RecvSocket error [%s]\n", WSAGetLastError()); break;
						default:
							{
								Log("------------------\nPacket received len = %d\n", l);
								if (conf.verbose >= 3)
								{
									dumphex(RecvBuf, l);
									printf("\n");
								}
								if (readoptions(RecvBuf, RecvBuf + 0xF0, l))
								{
									Log("Sending response packet...\n");
									sockaddr_in RecvAddr;
									RecvAddr.sin_family = AF_INET;
									RecvAddr.sin_port = htons(68);
									RecvAddr.sin_addr.s_addr = INADDR_BROADCAST;

									int rv = sendto(
										RecvSocket,
										(char *)resp.buf,
										resp.len,
										0,
										(SOCKADDR *)&RecvAddr, 
										SenderAddrSize);

									Log("sendto: %d (Error=%d)\n", rv, WSAGetLastError());
								}
								Log("\n\n");
							}
							break;
						}

						// check dhcp response socket
						l = recvfrom(
							RecvInfoSocket, 
							RecvBuf, 
							BufLen, 
							0, 
							(SOCKADDR *)&SenderAddr, 
							&SenderAddrSize
							);

						switch(l)
						{
						case 0: break; // no data
						case SOCKET_ERROR: Log("RecvInfoSocket error - exiting\n", WSAGetLastError()); break;
						default:
							{
								Log("------------------\nInfo received len = %d\n", l);
								if (conf.verbose >= 3)
								{
									dumphex(RecvBuf, l);
									Log("\n");
								}
								readoptions(RecvBuf, RecvBuf + 0xF0, l);
								Log("\n\n");
							}
							break;
						}
						break;
					}
				}
			}
		}
		__finally
		{
			if (AbnormalTermination())
			{
				Log("Exception caught!\n");
			}
			//-----------------------------------------------
			// Close the socket when finished receiving datagrams
			Log("Closing sockets\n");
			closesocket(RecvSocket);
			closesocket(RecvInfoSocket);
		}

		if (conf.retry && !exitflag)
		{
			Log("waiting to retry...\n");
			Sleep(10000);
			Log("retrying now\n");
		}
	} while (conf.retry && !exitflag);

	if (exitflag)
	{
		Log("OFF\n");
		Log("exiting\n");
	}

	return 0;
}

int CDhcpServer::readoptions(char *pkt, void *buf, int len)
{
	CLeaseInfo *pLease = NULL;
	int reply = 0;
	char ptype;
	unsigned char *cbuf;
	cbuf = (unsigned char *)buf;
	unsigned char *dbuf;
	string cid; // client identifier

	u_long tid; // bootp transaction ID

	tid = ntohl(*((u_long *)(pkt + 4)));

	ptype = pkt[0];

	switch(ptype)
	{
	case 1:		Log("Type: %d DHCP Boot Request\n", (int)ptype);  break;
	case 2:		Log("Type: %d DHCP Reply - CAUTION ANOTHER DHCP IS ACTIVE!\n", (int)ptype); break; // TODO: we'd have to be on 68 too for these
	default:	Log("Type: %d UNKNOWN\n", (int)ptype); break;
	}

	while ((cbuf[0] != 0xff) && (cbuf[0] != 0x0))
	{
		dbuf = &cbuf[2];
		Log("\n");
		Log("Option: %d len=%d\n", cbuf[0], cbuf[1]);
		switch(cbuf[0])
		{
		case 1: Log("Subnet mask\n"); break;
		case 2: Log("Time offset\n"); break;
		case 3: Log("Router option\n"); break; // this can contain multi sets of 4 octets
		case 4: Log("Time server\n"); break;
		case 5: Log("Name server\n"); break;
		case 6: Log("DNS\n"); break; // this can contain multi sets of 4 octets
		case 7: Log("Log server\n"); break;
		case 12: Log("Host name\n"); break;
		case 17: Log("Domain name\n"); break;
		case 22: Log("Maximum Datagram Reassembly Size: %d", ntohs(((u_short *)(dbuf))[0])); break;
		case 23: Log("Default IP TTL %d\n", dbuf[0]); break;
		case 28: Log("Broadcast address %d.%d.%d.%d\n", dbuf[0], dbuf[1], dbuf[2], dbuf[3]); break;
		case 31: Log("Perform Router Discovery\n"); break;
		case 33: Log("Static Routing Table\n"); break;
		case 42: Log("NTP server\n"); break;
		case 43: Log("Vendor Specific Information\n"); break;
		case 44: Log("NetBIOS over TCP/IP Name Server\n"); break;
		case 45: Log("NetBIOS over TCP/IP Distribution Server\n"); break;
		case 46: Log("NetBIOS over TCP/IP Node Type\n"); break;
		case 47: Log("NetBIOS over TCP/IP Scope Option\n"); break;
		case 50: Log("Requested IP Address %d.%d.%d.%d\n", dbuf[0], dbuf[1], dbuf[2], dbuf[3]); break;
		case 51: Log("IP Address Lease Time %d\n", ntohl(((u_long *)(dbuf))[0]));break;
		case 53: 
			switch(cbuf[2])
			{
			case 1: Log("DHCP Message Type %d (discover)\n", cbuf[2]); reply = 1; break;
			case 2: Log("DHCP Message Type %d (offer)\n", cbuf[2]); break;
			case 3: Log("DHCP Message Type %d (request)\n", cbuf[2]); reply = 2; break;
			case 4: Log("DHCP Message Type %d (decline)\n", cbuf[2]); break;
			case 5: Log("DHCP Message Type %d (ack)\n", cbuf[2]); break;
			case 6: Log("DHCP Message Type %d (nak)\n", cbuf[2]); break;
			case 7: Log("DHCP Message Type %d (release)\n", cbuf[2]); break;
			case 8: Log("DHCP Message Type %d (inform)\n", cbuf[2]); break;
			default: Log("DHCP Message Type %d (unknown)\n", cbuf[2]); break;
			}
			break;

		case 54: Log("DHCP Server Identifier\n"); break;

		case 55: Log("Parameter request list\n");
				 {
					 for (int i = 0; i < cbuf[1]; i++)
					 {
						switch(dbuf[i])
						{
						case   1: Log("  %2d - Subnet mask\n", dbuf[i]); break;
						case   3: Log("  %2d - Router\n", dbuf[i]); break;
						case   6: Log("  %2d - Domain Name Server\n", dbuf[i]); break;
						case  15: Log("  %2d - Domain Name\n", dbuf[i]); break;
						case  28: Log("  %2d - Broadcast Address\n", dbuf[i]); break;
						case  31: Log("  %2d - Perform Router Discovery\n", dbuf[i]); break;
						case  33: Log("  %2d - Static Routing Table\n", dbuf[i]); break;
						case  43: Log("  %2d - Vendor Specific Information\n", dbuf[i]); break;
						case  44: Log("  %2d - NetBIOS over TCP/IP Name Server\n", dbuf[i]); break;
						case  46: Log("  %2d - NetBIOS over TCP/IP Node Type\n", dbuf[i]); break;
						case  47: Log("  %2d - NetBIOS over TCP/IP Scope Option\n", dbuf[i]); break;
						case  51: Log("  %2d - IP Address Lease Time\n", dbuf[i]); break;
						case  58: Log("  %2d - Renewal Time Value\n", dbuf[i]); break;
						case  59: Log("  %2d - Rebinding Time Value\n", dbuf[i]); break;
						case 121: Log("  %2d - Classless Static Route\n", dbuf[i]); break;
						case 249: Log("  %2d - Classless Static Routes\n", dbuf[i]); break;
						default:  Log("  %2d - Unknown\n", dbuf[i]); break;
						}
//						 Log("Parameter requested: %d
					 }
				 }
				 break;
		case 56: Log("Message (for nak / decline)\n"); break;
		case 57: Log("Maximum DHCP Message Size: %d\n", ntohs(((u_short *)(dbuf))[0])); break;
		case 58: Log("Renewal (T1) Time Value\n"); break;
		case 59: Log("Rebinding (T2) Time Value\n"); break;
		case 60: Log("Vendor Class Identifier\n"); break;
		case 61: Log("Client identifier\n"); 
				 {
					 char tbuf[3];
					 string sb;
					 string s;
					 for (int i = 0; i < cbuf[1];i++)
					 {
						 _snprintf_s(tbuf, sizeof(tbuf), sizeof(tbuf) - 1, "%02X", dbuf[i]);
						 // todo: stringbuf!
						 s = tbuf;
						 sb = sb + s;
					 }
					 cid = sb;
					 Log("cid=[%s]", cid.c_str());
				 }
				 break;
		case 81: Log("Client FQDN 0x%02X\n", dbuf[0]); break;
		}
		dumphex(&cbuf[2], cbuf[1]);
		cbuf += cbuf[1] + 2;
	}

	switch(reply)
	{
	case 1: // reply with an OFFER
		{
			DWORD dwOfferIP;
			string ipstr;

			if (lp.GetLease(cid, &pLease))
			{
				dwOfferIP = pLease->IP;
				Log("OFFER: Found previously offered IP %s for client %s\n", ip2str(dwOfferIP).c_str(), cid.c_str());
			}
			else
			{
				dwOfferIP = conf.client++;
				Log("OFFER: Assigning new IP %08X for client %s\n", dwOfferIP, cid.c_str());
				lp.AddLease(cid, CLeaseInfo(dwOfferIP));
			}

			Log("Replying with an OFFER\n");
			resp.Reset();
			resp.AddUchar(2); // BOOTP reply
			resp.AddUchar(1); // Hardware type: Ethernet
			resp.AddUchar(6); // Hardware address length: 6
			resp.AddUchar(0); // Hops: 0
			resp.AddUlong(tid); // Transaction ID
			resp.AddUshort(0); // Seconds elapsed: 0
			resp.AddUshort(0); // Bootp flags
			resp.AddUlong(0); // Client IP Address
			resp.AddUlong(dwOfferIP); // Your (client) IP Address
			resp.AddUlong(0); // Next server IP address
			resp.AddUlong(0); // Relay agent IP address
			resp.AddPadded(pkt + 0x1C, 6, 0x10); // Client MAC
			resp.AddPadded("oxygen", 6, 0x40); // server host name
			resp.AddPadded(NULL, 0, 0x80); // boot file name
			resp.AddUlong(0x63825363); // magic cookie

			resp.AddOptChar(53, 2); // dhcp message, offer
			resp.AddOptUlong(1, conf.subnet); // subnet mask
			resp.AddOptUlong(51, 0xFFFFFFFF); // lease time, infinity
			resp.AddOptUlong(54, conf.server); // DHCP server ID
			resp.AddUchar(0xFF); // options end
		}
		break;

	case 2: // reply with an ACK
		Log("ACK: checking for a lease for %s\n", cid.c_str());

		pLease = NULL;
		if (!lp.GetLease(cid, &pLease))
		{
			Log("ACK: error - no lease found!\n");
			
			return 0;
		}
		else
		{
			Log("ACK: GetLease returned %d\n", pLease);
			Log("LEASE:Assigned %s to %s\n", ip2str(pLease->IP).c_str(), cid.c_str());
		}

		Log("ACK: Replying with an ACK for IP %08X\n", pLease->IP);
		resp.Reset();
		resp.AddUchar(2); // BOOTP reply
		resp.AddUchar(1); // Hardware type: Ethernet
		resp.AddUchar(6); // Hardware address length: 6
		resp.AddUchar(0); // Hops: 0
		resp.AddUlong(tid); // Transaction ID
		resp.AddUshort(0); // Seconds elapsed: 0
		resp.AddUshort(0); // Bootp flags
		resp.AddUlong(0); // Client IP Address
		resp.AddUlong(pLease->IP); // Your (client) IP Address
		resp.AddUlong(0); // Next server IP address
		resp.AddUlong(0); // Relay agent IP address
		resp.AddPadded(pkt + 0x1C, 6, 0x10); // Client MAC
		resp.AddPadded("oxygen", 6, 0x40); // server host name
		resp.AddPadded(NULL, 0, 0x80); // boot file name
		resp.AddUlong(0x63825363); // magic cookie

		resp.AddOptChar(53, 5); // dhcp message, ack
		resp.AddOptUlong(1, conf.subnet); // subnet mask
		resp.AddOptUlong(51, 0xFFFFFFFF); // lease time, infinity
		resp.AddOptUlong(54, conf.server); // DHCP server ID
		resp.AddUchar(0xFF); // options end
		break;
	}
	return reply;
}


int CDhcpServer::locateadapterconf() // find an adapter that matches the IP range
{
	IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information for up to 16 NICs
	DWORD dwBufLen = sizeof (AdapterInfo);  // Save memory size of buffer

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);                  // [in] size of receive data buffer
	if (dwStatus != ERROR_SUCCESS)
	{
		Log("GetAdaptersInfo failed. err=%d\n", GetLastError ());
		return 1;
	}

	Log("Finding adapter parameters for client address\n");

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to  current adapter info
	Log("Checking network adapters:\n\n");

	while (pAdapterInfo)
	{
		Log("  %s\n", pAdapterInfo->Description);
		PIP_ADDR_STRING pCIA = &pAdapterInfo->IpAddressList;
		while (pCIA)
		{
			if (pCIA->Context)
			{
				Log("  IP: %s\n", pCIA->IpAddress.String);

				u_long ulMask = ntohl(inet_addr(pCIA->IpMask.String));
				u_long ulIP = ntohl(inet_addr(pCIA->IpAddress.String));
				Log("    Network of adapter: %s vs\n    Network of client:  %s\n", ip2str(ulIP & ulMask).c_str() , ip2str(conf.client & ulMask).c_str());
				if ((ulIP & ulMask) == (conf.client & ulMask))
				{
					Log("Adapter address range matched client address\n");
					if (conf.subnet == 0)
					{
						Log("Using subnet mask %s\n", ip2str(ulMask).c_str());
						conf.subnet = ulMask;
					}
					
					if (conf.server == 0)
					{
						Log("Using server IP %s\n", ip2str(ulIP).c_str());
						conf.server = ulIP;
					}
					return 0;
				}
			}
			else
			{
				Log("  Interface is down\n");
			}
			pCIA = pCIA->Next;
		}

		Log("\n");

		pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list
	}

	return 1; // fail
}

int CDhcpServer::SetConf(char *sEntry, unsigned long ulValue)
{
	if (strcmp(sEntry, "verbose") == 0) { conf.verbose = ulValue; return 0; }
	if (strcmp(sEntry, "retry") == 0) { conf.retry = ulValue; return 0; }

	return 1;
}

int CDhcpServer::SetConf(char *sEntry, char *szValue)
{
	if (strcmp(sEntry, "server") == 0) { conf.server = ntohl(inet_addr(szValue)); return 0; }
	if (strcmp(sEntry, "subnet") == 0) { conf.subnet = ntohl(inet_addr(szValue)); return 0; }
	if (strcmp(sEntry, "client") == 0) { conf.client = ntohl(inet_addr(szValue)); return 0; }
	
	return 1;
}

int CDhcpServer::Terminate()
{
	exitflag = 1;
	return 0;
}
