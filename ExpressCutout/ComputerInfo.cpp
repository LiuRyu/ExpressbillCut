#include "ComputerInfo.h"

#pragma comment(lib,"Iphlpapi.lib") //需要添加Iphlpapi.lib库

string*  Byte2Hex(unsigned char bArray[], int bArray_len);
//const std::string GetOsVersion()
//{
//	std::string os_version("");
//	SYSTEM_INFO system_info;
//	memset(&system_info, 0, sizeof(SYSTEM_INFO));
//	GetSystemInfo(&system_info);
//	OSVERSIONINFOEX os;
//	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
//	if (GetVersionEx((OSVERSIONINFO *)&os))
//	{
//		switch (os.dwMajorVersion){
//		case 4:
//			//1996年7月发布 
//			switch (os.dwMinorVersion){
//			case 0:
//				if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
//					os_version = "Microsoft Windows NT 4.0 ";
//				else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
//					os_version = "Microsoft Windows 95 ";
//				break;
//			case 10:
//				os_version = "Microsoft Windows 98 ";
//				break;
//			case 90:
//				os_version = "Microsoft Windows Me ";
//				break;
//			}
//			break;
//		case 5:
//			switch (os.dwMinorVersion){
//				//1999年12月发布 
//			case 0:
//				os_version = "Microsoft Windows 2000 ";
//				if (os.wSuiteMask == VER_SUITE_ENTERPRISE)
//					os_version.append("Advanced Server ");
//				break;
//				//2001年8月发布 
//			case 1:
//				os_version = "Microsoft Windows XP ";
//				if (os.wSuiteMask == VER_SUITE_EMBEDDEDNT)
//					os_version.append("Embedded ");
//				else if (os.wSuiteMask == VER_SUITE_PERSONAL)
//					os_version.append("Home Edition ");
//				else
//					os_version.append("Professional ");
//				break;
//			case 2:
//				if (os.wProductType == VER_NT_WORKSTATION &&
//					system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
//					os_version = "Microsoft Windows XP Professional x64 Edition ";
//				if (GetSystemMetrics(SM_SERVERR2) == 0 && os.wSuiteMask == VER_SUITE_BLADE)
//					os_version = "Microsoft Windows Server 2003 Web Edition ";
//				else if (GetSystemMetrics(SM_SERVERR2) == 0 && os.wSuiteMask == VER_SUITE_COMPUTE_SERVER)
//					os_version = ("Microsoft Windows Server 2003 Compute Cluster Edition ");
//				else if (GetSystemMetrics(SM_SERVERR2) == 0 && os.wSuiteMask == VER_SUITE_STORAGE_SERVER)
//					os_version = ("Microsoft Windows Server 2003 Storage Server ");
//				else if (GetSystemMetrics(SM_SERVERR2) == 0 && os.wSuiteMask == VER_SUITE_DATACENTER)
//					os_version = ("Microsoft Windows Server 2003 Datacenter Edition ");
//				else if (GetSystemMetrics(SM_SERVERR2) == 0 && os.wSuiteMask == VER_SUITE_ENTERPRISE)
//					os_version = ("Microsoft Windows Server 2003 Enterprise Edition ");
//				else if (GetSystemMetrics(SM_SERVERR2) != 0 && os.wSuiteMask == VER_SUITE_STORAGE_SERVER)
//					os_version = ("Microsoft Windows Server 2003 R2 Storage Server ");
//				break;
//			}
//			break;
//		case 6:
//			switch (os.dwMinorVersion){
//			case 0:
//				if (os.wProductType == VER_NT_WORKSTATION)
//				{
//					os_version = "Microsoft Windows Vista ";
//					if (os.wSuiteMask == VER_SUITE_PERSONAL)
//						os_version.append("Home ");
//				}
//				else if (os.wProductType != VER_NT_WORKSTATION)
//				{
//					os_version = "Microsoft Windows Server 2008 ";
//					if (os.wSuiteMask == VER_SUITE_DATACENTER)
//						os_version.append("Datacenter Server ");
//					else if (os.wSuiteMask == VER_SUITE_ENTERPRISE)
//						os_version.append("Enterprise ");
//				}
//				break;
//			case 1:
//				if (os.wProductType == VER_NT_WORKSTATION)
//					os_version = "Microsoft Windows 7 ";
//				else
//					os_version = "Microsoft Windows Server 2008 R2 ";
//				break;
//			}
//			break;
//		default:
//			os_version = "? ";
//		}
//	}
//	SYSTEM_INFO si;
//	SafeGetNativeSystemInfo(&si);
//	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
//		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
//		os_version.append("64bit");
//	else os_version.append("32bit");
//	return os_version;
//}

void SafeGetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	if (NULL == lpSystemInfo)
		return;
	typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
	LPFN_GetNativeSystemInfo nsInfo =
		(LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle(_T("kernel32")), "GetNativeSystemInfo");;
	if (NULL != nsInfo)
	{
		nsInfo(lpSystemInfo);
	}
	else
	{
		GetSystemInfo(lpSystemInfo);
	}
}

//const std::wstring GetComputerUerName()
//{
//	std::wstring uer_name(L"");
//	char  buffer[kMaxInfoBuffer];
//	DWORD length = kMaxInfoBuffer;
//	if (GetComputerName(buffer, &length))
//		uer_name.append(buffer, length);
//	return uer_name;
//}

const std::string GetMemoryStatus()
{
	std::string memory_info("");
	MEMORYSTATUSEX statusex;
	statusex.dwLength = sizeof(statusex);
	if (GlobalMemoryStatusEx(&statusex))
	{
		unsigned long long total = 0, remain_total = 0, avl = 0, remain_avl = 0;
		double decimal_total = 0, decimal_avl = 0;
		remain_total = statusex.ullTotalPhys % GBYTES;
		total = statusex.ullTotalPhys / GBYTES;
		avl = statusex.ullAvailPhys / GBYTES;
		remain_avl = statusex.ullAvailPhys % GBYTES;
		if (remain_total > 0)
			decimal_total = (remain_total / MBYTES) / DKBYTES;
		if (remain_avl > 0)
			decimal_avl = (remain_avl / MBYTES) / DKBYTES;

		decimal_total += (double)total;
		decimal_avl += (double)avl;
		char  buffer[kMaxInfoBuffer];
		sprintf_s(buffer, kMaxInfoBuffer, "%.2f GB (%.2f GB可用)", decimal_total, decimal_avl);
		memory_info.append(buffer);
	}
	return memory_info;
}

void GetNetCardAndIPInfo(std::string &adapter_info, std::string &MAC_address, std::string &IP)
{
	PIP_ADAPTER_INFO pIp_adapter_info = new IP_ADAPTER_INFO();
	unsigned long adapter_size = sizeof(IP_ADAPTER_INFO);
	int ret = GetAdaptersInfo(pIp_adapter_info, &adapter_size);
	if (ERROR_BUFFER_OVERFLOW == ret)
	{
		delete pIp_adapter_info;
		pIp_adapter_info = (PIP_ADAPTER_INFO)new BYTE[adapter_size];
		ret = GetAdaptersInfo(pIp_adapter_info, &adapter_size);
	}
	if (ERROR_SUCCESS == ret)
	{
		while (pIp_adapter_info)
		{
			adapter_info.append("name: ");
			adapter_info.append(pIp_adapter_info->AdapterName);
			adapter_info.append("\ndescription: ");
			adapter_info.append(pIp_adapter_info->Description);
			adapter_info.append("\ntype: ");
			std::string card_type("");
			switch (pIp_adapter_info->Type)
			{
			case MIB_IF_TYPE_OTHER:
				card_type = "other";
				break;
			case MIB_IF_TYPE_ETHERNET:
				card_type = "ethernet";
				break;
			case MIB_IF_TYPE_TOKENRING:
				card_type = "tokenring";
				break;
			case MIB_IF_TYPE_FDDI:
				card_type = "fddi";
				break;
			case MIB_IF_TYPE_PPP:
				card_type = "ppp";
				break;
			case MIB_IF_TYPE_LOOPBACK:
				card_type = "loopback";
				break;
			case MIB_IF_TYPE_SLIP:
				card_type = "slip";
				break;
			default:
				break;
			}
			adapter_info.append(card_type);
			MAC_address.append("\nMACAddr: ");
			char  buffer[kMaxInfoBuffer];
			for (DWORD i = 0; i < pIp_adapter_info->AddressLength; i++)
			if (i < pIp_adapter_info->AddressLength - 1)
			{
				sprintf_s(buffer, kMaxInfoBuffer, "%02X", pIp_adapter_info->Address[i]);
				MAC_address.append(buffer);
				MAC_address.append("-");
			}
			else
			{
				sprintf_s(buffer, kMaxInfoBuffer, "%02X", pIp_adapter_info->Address[i]);
				MAC_address.append(buffer);
				adapter_info.append("\n");
			}

			IP_ADDR_STRING *pIp_addr_string = &(pIp_adapter_info->IpAddressList);
			do
			{
				IP.append("IPAddr:");
				IP.append(pIp_addr_string->IpAddress.String);;
				IP.append("\nIpMask:");
				IP.append(pIp_addr_string->IpMask.String);
				IP.append("\nGateway:");
				IP.append(pIp_adapter_info->GatewayList.IpAddress.String);
				IP.append("\n");
				pIp_addr_string = pIp_addr_string->Next;
			} while (pIp_addr_string);
			adapter_info.append("\n");
			pIp_adapter_info = pIp_adapter_info->Next;
		}

	}
	if (pIp_adapter_info)
	{
		delete pIp_adapter_info;
		pIp_adapter_info = nullptr;
	}
}

//const std::string GetCpuInfo()
//{
//	std::string processor_name("");
//	LPCSTR  str_path = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
//	HKEY key;
//	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, str_path, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
//	{
//		char processor_value[256];
//		DWORD type = REG_SZ;
//		DWORD value_size = sizeof(processor_value);
//		if (::RegQueryValueEx(key, (LPCSTR)"ProcessorNameString", 0, &type, (LPBYTE)&processor_value, &value_size) == ERROR_SUCCESS)
//			processor_name.append(processor_value, value_size);
//		RegCloseKey(key);
//
//	}
//	return processor_name;
//}

const std::string  GetDisplayCardinfo()
{
	std::string memory_info("");
	return memory_info;
}


//--------------------------------------------------------------
//						主板序列号 -- 获取不到时为 None
//--------------------------------------------------------------
int GetBaseBoardByCmd(char *lpszBaseBoard, int len/*=128*/)
{
	const long MAX_COMMAND_SIZE = 10000; // 命令行输出缓冲大小	
	char szFetCmd[] = "wmic BaseBoard get SerialNumber"; // 获取主板序列号命令行	
	const string strEnSearch = "SerialNumber"; // 主板序列号的前导信息

	int    nameLength =0;
	HANDLE hReadPipe = NULL; //读取管道
	HANDLE hWritePipe = NULL; //写入管道	
	PROCESS_INFORMATION pi;   //进程信息	
	STARTUPINFO			si;	  //控制命令行窗口信息
	SECURITY_ATTRIBUTES sa;   //安全属性

	char			szBuffer[MAX_COMMAND_SIZE + 1] = { 0 }; // 放置命令行结果的输出缓冲区
	string			strBuffer;
	unsigned long	count = 0;
	long			ipos = 0;

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	memset(&sa, 0, sizeof(sa));

	pi.hProcess = NULL;
	pi.hThread = NULL;
	si.cb = sizeof(STARTUPINFO);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	//1.0 创建管道
	bool bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
	if (!bret)
	{
		goto END;
	}

	//2.0 设置命令行窗口的信息为指定的读写管道
	GetStartupInfo(&si);
	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	si.wShowWindow = SW_HIDE; //隐藏命令行窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	//3.0 创建获取命令行的进程
	bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if (!bret)
	{
		goto END;
	}

	//4.0 读取返回的数据
	WaitForSingleObject(pi.hProcess, 500/*INFINITE*/);
	bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);
	if (!bret)
	{
		goto END;
	}

	//5.0 查找主板序列号
	bret = FALSE;
	strBuffer = szBuffer;
	ipos = strBuffer.find(strEnSearch);

	if (ipos < 0) // 没有找到
	{
		goto END;
	}
	else
	{
		strBuffer = strBuffer.substr(ipos + strEnSearch.length());
	}

	memset(szBuffer, 0x00, sizeof(szBuffer));
	strcpy_s(szBuffer, strBuffer.c_str());

	//去掉中间的空格 \r \n
	int j = 0;
	for (int i = 0; i < strlen(szBuffer); i++)
	{
		if (szBuffer[i] != ' ' && szBuffer[i] != '\n' && szBuffer[i] != '\r')
		{
			lpszBaseBoard[j] = szBuffer[i];
			j++;
		}
	}
	nameLength = j;

END:
	//关闭所有的句柄
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return(nameLength);
}


void GetMAC(vector<string>  &vector_mac)
{
	//PIP_ADAPTER_INFO结构体指针存储本机网卡信息
	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
	//得到结构体大小,用于GetAdaptersInfo参数
	unsigned long stSize = sizeof(IP_ADAPTER_INFO);
	//调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
	int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	if (ERROR_BUFFER_OVERFLOW == nRel)
	{
		//如果函数返回的是ERROR_BUFFER_OVERFLOW
		//则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
		//这也是说明为什么stSize既是一个输入量也是一个输出量
		//释放原来的内存空间
		delete pIpAdapterInfo;
		//重新申请内存空间用来存储所有网卡信息
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
		//再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	}
	if (ERROR_SUCCESS == nRel)
	{
		//输出网卡MAC信息
		//可能有多网卡,因此通过循环去判断
		string mac;
		char m_MAC[14];
		while (pIpAdapterInfo)
		{
			//string mac = Byte2Hex(pIpAdapterInfo->Address, pIpAdapterInfo->AddressLength);
			
			
			int nIndex = 0;
			for (int i = 0; i<pIpAdapterInfo->AddressLength; i++)
			{
				sprintf(m_MAC + i * 2, "%02X\n", pIpAdapterInfo->Address[i]);
				int high = pIpAdapterInfo->Address[0] / 16, low = pIpAdapterInfo->Address[0] % 16;
				/*MAC[nIndex] = (high<10) ? ('0' + high) : ('A' + high - 10);
				MAC[nIndex + 1] = (low<10) ? ('0' + low) : ('A' + low - 10);*/
				nIndex += 2;
			}
			mac = m_MAC;
			vector_mac.push_back(mac);
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
	}
	//释放内存空间
	if (pIpAdapterInfo)
	{
		delete pIpAdapterInfo;
	}
//	getchar();
	return;
}

string*  Byte2Hex(unsigned char bArray[], int bArray_len)
{
	string *strHex = new string();
	int nIndex = 0;
	for (int i = 0; i<bArray_len; i++)
	{
		int high = bArray[0] / 16, low = bArray[0] % 16;
		strHex[nIndex] = (high<10) ? ('0' + high) : ('A' + high - 10);
		strHex[nIndex + 1] = (low<10) ? ('0' + low) : ('A' + low - 10);
		nIndex += 2;
	}
	return strHex;
}



