
#include "stdafx.h"
#include <math.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include<vector>
using namespace std;



//Get Windows system informantion
static const int kMaxInfoBuffer = 256;
#define  GBYTES  1073741824
#define  MBYTES  1048576
#define  KBYTES  1024
#define  DKBYTES 1024.0
void SafeGetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo);
//const std::string  GetCpuInfo();
//const std::string  GetOsVersion();
const std::string  GetMemoryStatus();
//const std::wstring GetComputerUerName();
const std::string  GetDisplayCardinfo();
void  GetNetCardAndIPInfo(std::string &adapter_info, std::string &MAC_address, std::string &IP);
BOOL GetBaseBoardByCmd(char *lpszBaseBoard, int len = 128);
void GetMAC(vector<string>  &vector_mac);