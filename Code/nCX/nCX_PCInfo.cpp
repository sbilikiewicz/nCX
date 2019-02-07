/**********************************************************
####     ##  #####  ##   ##
## ##    ## ##   ##  ## ##  Crysis nCX
##  ##   ## ##        ###     by MrHorseDick
##   ##  ## ##        ###       and
##    ## ## ##   ##  ## ##        MrCtaostrach
##     ####  #####  ##   ##
**********************************************************/
#include "StdAfx.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <WS2tcpip.h>
#include <Windows.h>
#include "nCX_PCInfo.h"
#include <intrin.h>
#include <Iphlpapi.h>
#pragma comment(lib, "wsock32.lib")
#pragma comment (lib, "WINMM.LIB")

const char* nCX_PCInfo::query(const char *host)
{
	ICVar *pCVar = gEnv->pConsole->GetCVar("sv_lanonly");
	int mode = pCVar->GetIVal();
	if (mode == 1) 
		return "EU";
	SOCKET sock;
	SOCKADDR_IN sin;
	struct addrinfo hints;
	char buffer[512];
	string whoisQuery;
	int i;

	SecureZeroMemory(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (-1 == (sock = socket(AF_INET, SOCK_STREAM, 0)))
		return "EU";
 
	sin.sin_addr.s_addr = inet_addr("193.0.6.135");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(43);
	if (-1 == connect(sock, (SOCKADDR *)&sin, sizeof(sin)))
		return "EU";
 
	whoisQuery.assign(host);
	whoisQuery.append("\r\n");
	if (-1 == send(sock, whoisQuery.c_str(), whoisQuery.length(), 0))
	{
		i = errno;
		closesocket(sock);
		return "EU";
	}
 
	string temp;
	string country;
	string desc;
	while (i = recv(sock, buffer, sizeof(buffer), 0))
	{
		temp = string(buffer);
		int found = temp.find("country");
		if (found != -1)
		{
			country.append(temp.begin()+found+16, temp.begin()+found+18);
		}
	}
	closesocket(sock);
	return country.c_str();
}

const char* nCX_PCInfo::GetCPU()
{
	//CPU Detect
	int CPUInfo[4] = {-1};
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];
	char CPUBrandString[0x40] = { 0 };
	for( unsigned int i=0x80000000; i<=nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		if  (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if(i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if(i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}
	return (string)CPUBrandString;
}

const char* nCX_PCInfo::GetRAM()
{
	MEMORYSTATUS theStatus;
	ZeroMemory(&theStatus,sizeof(theStatus));
	theStatus.dwLength = sizeof(theStatus);
	GlobalMemoryStatus(&theStatus);
	DWORD dwRAM = (DWORD)(theStatus.dwTotalPhys/(1024*1024));
	if(theStatus.dwTotalPhys != dwRAM*1024*1024)
		++dwRAM;
	
	//char output[24];
	//sprintf(output,"%dMB / %dMB", theStatus.dwAvailPhys/(1024*1024), dwRAM);
	string output;
	output.Format("%dMB / %dMB", theStatus.dwAvailPhys/(1024*1024), dwRAM);
	return output.c_str();
}

int nCX_PCInfo::GetCPUSpeed()
{
    __int64 timeStart, timeStop;
    __int64 startTick, endTick;
    __int64 overhead;
	overhead = __rdtsc() - __rdtsc();
	timeStart = timeGetTime();
	while(timeGetTime() == timeStart) 
    {
		timeStart = timeGetTime();
    }
	for(;;)
	{
		timeStop = timeGetTime();
		if ((timeStop - timeStart) > 1)	
		{
			startTick = __rdtsc();
			break;
		}
	}
	timeStart = timeStop;
	for(;;)
	{
		timeStop = timeGetTime();
		if ((timeStop - timeStart) > 1000)	
		{
			endTick = __rdtsc();
			break;
		}
	}
	return ((__int64)((endTick - startTick) + (overhead)) / 1000000);
}

int nCX_PCInfo::GetCpuUsage()
{
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;
	BOOL res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
	int SysTime = kernelTime.dwLowDateTime + userTime.dwLowDateTime;
	int Speed = int( (SysTime - idleTime.dwLowDateTime) *100 / SysTime );
	return Speed;
}

int nCX_PCInfo::GetSystemTime()
{
	__int64 time = timeGetTime();
	return (time/1000)/60;
}

const char* nCX_PCInfo::RegexBuilder(const char* msg)
{
	string reg = msg;
	string regex;
	string remove = "[^a-z^A-Z]*";

	for (int check = 0; check < reg.size(); ++check)
	{
		string curchar(reg[check]);
		regex.append("[" + curchar.MakeLower());
		regex.append(curchar.MakeUpper() + "]+");
		if (check+1 < reg.size())
			regex.append(remove.c_str());
	}
	return regex.c_str();
}