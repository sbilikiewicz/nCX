/**********************************************************
		####     ##  #####  ##   ##
		## ##    ## ##   ##  ## ##	Crysis Wars nCX
		##  ##   ## ##        ###				by Arcziy
		##   ##  ## ##        ###
		##    ## ## ##   ##  ## ##
		##     ####  #####  ##   ##
**********************************************************/
#pragma once
#include <iostream>

class ExInfo
{
public:
    static const char* query(const char *host);
	static const char* GetCPU();
	static const char* GetRAM();
	static int GetCPUSpeed();
	static int GetCpuUsage();
	static int GetSystemTime();
	static const char* RegexBuilder(const char *msg);

};