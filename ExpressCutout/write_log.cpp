/************************************************************************/ 
/*
  * 文件名称：write_log.cpp
  * 摘    要：此文件实现了普通WINDOWS程序中的日志功能
  *           主要有以下特点：
  *           1. 根据日期创建日志文件目录，每天的日志分别存放在不同的日志目录中；
  *           2. 日志内容分三种类型，根据不同需要，写不同的日志类型的日志文件，
  *              方便通过日志定位、分析问题；
  *           3. 函数经过比较好的封装，便于复用；
  *           待改进点：
  *           1. 为了方便，日志内容打印时使用了time函数，其精确度较低；
  *           2. 可将这些函数封装为一个日志类，或者动态库，使其更通用；
  *           3. 没有考虑跨平台情景，目前只使用于WINDOWS下
  *           4. 日志文件内容还可进一步改进，比如打印出当前文件名与行号，使用日志功能
  *              更加实用；
  *
  * 当前版本：1.0
  * 作    者：duanyongxing 
  * 完成日期：2009年10月11日
*/                                                                      
/************************************************************************/ 
#include "stdafx.h"  
#include <time.h>  
#include <memory.h>  
#include <stdio.h>  
#include <stdlib.h>   
#include <stdarg.h>  
#include <windows.h>
#include "write_log.h"

static char g_LogRootPath[MAX_FILE_PATH] = "C://ALGOLOG";	/* 默认日志文件根路径，可以由用户修改 */
static int g_LogIdentityCode = 0;

#pragma pack(push, 1)
typedef struct tagLOG_DATA				/* 日志内容结构体*/ 
{ 
 char			strDate[11];			/* 日期:格式为如:2009-10-11*/ 
 char			strTime[9];				/* 时间:格式为如:16:10:57*/ 
 unsigned int	iType;					/* 日志类型:3种:INFO(0)/ERROR(1)/SYSTEM(2)*/ 
 char			strText[MAX_LOGTEXT_LEN];	/*日志内容*/ 
}LOG_DATA, *LPLOG_DATA; 
#pragma pack(pop)
 
int Create_LogDir(const char *pStrPath); 
int Create_LogFile(const char *pStrFile, int iPos); 
int IsFileExist(const char *pStrFile); 
int GetLogPath(char *pStrPath); 
DWORD GetFileLenth(const char *pFile); 
int Write_Log_Text(LPLOG_DATA lpLogData); 

int GenerateLogIdCode()
{
	int idCode = (rand() << 16)
				| ((rand() << 8) & 0xff00)
				| (rand() & 0xff);
	g_LogIdentityCode = idCode;

	return idCode;
}

int SetLogPath(char *pStrPath)
{
	if(NULL == pStrPath) 
	{ 
		return LOG_FAILED; 
	} 
	int pathLen = strlen(pStrPath);
	if(pathLen > 200) {
		return LOG_FAILED;
	}
	memcpy(g_LogRootPath, pStrPath, pathLen * sizeof(char));

	return LOG_SUCCESS; 
}

/*********************************************************************
* 函数名称:void TestLogCase_One()
* 说明:简单的测试函数，读文件
* 调用者：main
* 输入参数:
* 无
* 输出参数：
* 无
* 返回值:
* void  -- 
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
void TestLogCase_One() 
{ 
	FILE *pFile = NULL; 
	char *pFieldContent = NULL; 
	char szFileName[] = "test_case.txt"; 
	pFieldContent = (char *)malloc(TEST_CASE_MAX_FILE_LEN); 
	if(NULL == pFieldContent) 
	{ 
		Write_Log(LOG_TYPE_ERROR, "malloc memory failed,program exit!"); 
		return; 
	} 
	memset(pFieldContent, 0, TEST_CASE_MAX_FILE_LEN); 
	Write_Log(LOG_TYPE_INFO, "malloc memory for pFiled successful,memory size is: %ld", 
	TEST_CASE_MAX_FILE_LEN); 
	pFile = fopen(szFileName, "r"); 
	if(NULL == pFile) 
	{ 
		fprintf(stderr, "open file failed."); 
		Write_Log(LOG_TYPE_ERROR, "Open file %s failed. program exit!", szFileName); 
		return; 
	} 
	Write_Log(LOG_TYPE_INFO, "Open file %s successful.", szFileName); 
	fread(pFieldContent, 1, TEST_CASE_MAX_FILE_LEN, pFile); 
	pFieldContent[TEST_CASE_MAX_FILE_LEN -1] = '/0'; 
  
	fclose(pFile); 
     
	printf("The file %s content is: \n%s\n", szFileName,  pFieldContent); 
	Write_Log(LOG_TYPE_INFO, "The file %s content is: \n%s\n", szFileName,  pFieldContent); 
} 
/*********************************************************************
* 函数名称:void Write_Log(unsigned int uiLogType, char *pstrFmt, ...)
* 说明:日志写函数，支持变长参数
* 调用者：任何需要写日志的地方
* 输入参数:
* unsigned iType --  日志类别
* char *pstrFmt  --  日志内容
* ...            --  变长参数
* 输出参数：
* 无
* 返回值:
* void  -- 
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
void Write_Log(unsigned int uiLogType, char *pstrFmt, ...) 
{ 
#if _LOG_WRITE_STATE_   /* 写日志与否的编译开关*/  
	LOG_DATA data; 
	time_t curTime; 
	struct tm *mt; 
	va_list v1; 
	memset(&data, 0, sizeof(LOG_DATA)); 
	va_start(v1, pstrFmt); 
	_vsnprintf(data.strText, MAX_LOGTEXT_LEN, pstrFmt, v1); 
	va_end(v1); 
	data.iType = uiLogType; 
	curTime = time(NULL); 
	mt = localtime(&curTime); 
	strftime(data.strDate, sizeof(data.strDate), "%Y-%m-%d", mt); 
	strftime(data.strTime, sizeof(data.strTime), "%H:%M:%S", mt); 
	Write_Log_Text(&data); 
#endif _LOG_WRITE_STATE_  
} 
/*********************************************************************
* 函数名称:int  GetLogPath(char *pStrPath)
* 说明:获取日志文件路径
* 调用者：Write_Log_Text
* 输入参数:
* 无
* 输出参数：
* char *pStrPath
* 返回值:
* int  -- LOG_FAILED:  失败
*      -- LOG_SUCCESS: 成功
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int  GetLogPath(char *pStrPath) 
{ 
	if(NULL == pStrPath) 
	{ 
		return LOG_FAILED; 
	} 
	int iRet = 0; 
	time_t curTime = time(NULL); 
	struct tm *mt = localtime(&curTime); 
    /* 根据日期组成文件夹名称*/ 
	sprintf(pStrPath, "%s//%d%02d%02d", g_LogRootPath, mt->tm_year + 1900, 
	mt->tm_mon + 1, mt->tm_mday); 
    iRet = Create_LogDir(pStrPath); 
	return iRet; 
} 
/*********************************************************************
* 函数名称:int GetLogFileName(int iLogType, const char *pStrPath, char *pStrName)
* 说明:获取日志文件名
* 调用者：Write_Log_Text
* 输入参数:
* int iLogType         -- 日志类型 3种:INFO(0)/ERROR(1)/SYSTEM(2)
* const char *pStrPath -- 日志路径 由GetLogPath得到
* 输出参数：
* char *pStrName       -- 日志文件名
* 返回值:
* int  -- LOG_FAILED:  失败
*      -- LOG_SUCCESS: 成功
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int GetLogFileName(int iLogType, const char *pStrPath, char *pStrName) 
{ 
	int i = 0;
	if(NULL == pStrPath) 
	{ 
		return LOG_FAILED; 
	} 
	char szLogName[MAX_FILE_PATH]; 
	char szLogWholeName[MAX_FILE_PATH];
	FILE *pFile = NULL; 
	memset(szLogName, 0, MAX_FILE_PATH); 
	switch (iLogType) 
	{ 
		case LOG_TYPE_INFO: 
			sprintf(szLogName, "%s//algo_info", pStrPath); 
			break; 
		case LOG_TYPE_ERROR: 
			sprintf(szLogName, "%s//algo_error", pStrPath); 
			break; 
		case LOG_TYPE_SYSTEM: 
			sprintf(szLogName, "%s//algo_system", pStrPath); 
			break; 
		default: 
			return LOG_FAILED; 
		break; 
	}
// 	strcat(szLogName, ".log"); 
// 	if(IsFileExist(szLogName)) 
// 	{ 
// 		/* 如果文件长度大于指定的最大长度，重新创建一文件，覆盖原文件*/ 
// 		if((int)GetFileLenth(szLogName) + 256 >= MAX_LOG_FILE_SIZE) 
// 		{
// 			Create_LogFile(szLogName, 0); 
// 		} 
// 	} 
// 	else 
// 	{ 
// 		Create_LogFile(szLogName, 0); 
// 	} 
	for(i = 1; i <= 1024; i++) {
		sprintf(szLogWholeName, "%s-%d.log", szLogName, i);
		if(IsFileExist(szLogWholeName)) 
		{ 
		 	/* 如果文件长度大于指定的最大长度，创建下一序号*/ 
		 	if((int)GetFileLenth(szLogWholeName) + 256 >= MAX_LOG_FILE_SIZE) {
				continue;
			}
		 	else
				break;
		} 
		else 
		{ 
		 	Create_LogFile(szLogWholeName, 0); 
			break;
		} 
	}

//	sprintf(pStrName, "%s", szLogName); 
	sprintf(pStrName, "%s", szLogWholeName); 
	return LOG_SUCCESS; 
} 
/*********************************************************************
* 函数名称:int Create_LogDir(const char *pStrPath)
* 说明:创建日志存放路径
* 调用者：GetLogPath
* 输入参数:
* const char *pStrPath --用户指定的根路径
* 输出参数：
* 无
* 返回值:
* int  -- LOG_FAILED:  失败
*      -- LOG_SUCCESS: 成功
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int Create_LogDir(const char *pStrPath) 
//int Create_LogDir(char *pStrPath)
{ 
	if(NULL == pStrPath) 
	{ 
		return LOG_FAILED; 
	} 
	int iRet = 0; 
	char szSub[MAX_FILE_PATH]; 
	char *pSub = NULL; 
	int iIndex = 0; 
	int iLen = 0; 
	int bFind = 0; 
	memset(szSub, 0, sizeof(MAX_FILE_PATH)); 
     
	/* 逐层创建目录*/ 
	while(1) 
	{ 
		pSub = (char *)strchr(pStrPath + iLen, '//'); 
		if(NULL == pSub) 
		{ 
			if(iLen == 0) 
			{ 
				return LOG_FAILED; 
			} 
			iRet = CreateDirectory(pStrPath, NULL); 
			if(0 == iRet) 
			{ 
				iRet = GetLastError(); 
				if(ERROR_ALREADY_EXISTS == iRet) 
				{ 
					return LOG_SUCCESS; 
				} 
				return LOG_FAILED; 
			} 
			return LOG_SUCCESS; 
		} 
		else 
		{ 
			if (!bFind) 
			{ 
				bFind = 1; 
			} 
			else 
			{ 
				memset(szSub, 0, sizeof(szSub)); 
				strncpy(szSub, pStrPath, pSub - pStrPath); 
				CreateDirectory(szSub, NULL); 
			} 
			iLen = pSub - pStrPath + 1; 
		} 
	} 
	return LOG_SUCCESS; 
} 
/*********************************************************************
* 函数名称:int Create_LogFile(const char *pStrFile, int iPos)
* 说明:创建日志文件
* 调用者：GetLogFileName
* 输入参数:
* const char *pStrFile --文件名
* int iPos             --文件指针位置
* 输出参数：
* 无
* 返回值:
* int  -- LOG_FAILED:  失败
*      -- LOG_SUCCESS: 成功
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int Create_LogFile(const char *pStrFile, int iPos) 
{ 
	HANDLE hd = 0; 
	int iRet = 0; 
	if(NULL == pStrFile) 
	{ 
		return LOG_FAILED; 
	} 
	hd = CreateFile(pStrFile,  
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			NULL, 
			CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL); 
	if(INVALID_HANDLE_VALUE == hd) 
	{ 
		return LOG_FAILED; 
	} 
	if(DWORD_NULL == SetFilePointer(hd, iPos, NULL, FILE_BEGIN)) 
	{ 
		return LOG_FAILED; 
	} 
	iRet = SetEndOfFile(hd); 
	CloseHandle(hd); 
	return iRet; 
} 
/*********************************************************************
* 函数名称:int IsFileExist(const char *pStrFile)
* 说明:判断指定的文件是否存在
* 调用者：GetLogFileName
* 输入参数:
* const char *pStrFile --文件名
* 输出参数：
* 无
* 返回值:
* int  -- LOG_BOOL_FALSE:  不存在
*      -- LOG_BOOL_TRUE: 存在
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int IsFileExist(const char *pStrFile) 
{ 
	int iLen = 0; 
	WIN32_FIND_DATA finddata; 
	memset(&finddata, 0, sizeof(WIN32_FIND_DATA)); 
	HANDLE hd = FindFirstFile(pStrFile, &finddata); 
	if(INVALID_HANDLE_VALUE == hd) 
	{ 
		DWORD dwRet = GetLastError(); 
		if(ERROR_FILE_NOT_FOUND == dwRet || ERROR_PATH_NOT_FOUND == dwRet) 
		{ 
			return LOG_BOOL_FALSE; 
		} 
	} 
	FindClose(hd); 
	return LOG_BOOL_TRUE; 
} 
/*********************************************************************
* 函数名称:DWORD GetFileLenth(const char *pFile)
* 说明:判断指定的文件大小
* 调用者：GetLogFileName
* 输入参数:
* const char *pFile --文件名
* 输出参数：
* 无
* 返回值:
* DWORD -- 文件大小
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
DWORD GetFileLenth(const char *pFile) 
{ 
	WIN32_FIND_DATA buff; 
	HANDLE hd = NULL; 
	memset(&buff, 0, sizeof(WIN32_FIND_DATA)); 
	hd = FindFirstFile(pFile, &buff); 
	FindClose(hd); 
	return (buff.nFileSizeHigh * MAXDWORD) + buff.nFileSizeLow; 
} 
/*********************************************************************
* 函数名称:int Write_Log_Text(LPLOG_DATA lpLogData)
* 说明:写日志内容
* 调用者：Write_Log
* 输入参数:
* LPLOG_DATA lpLogData --日志内容结构体量
* 输出参数：
* 无
* 返回值:
* int  -- LOG_FAILED:  失败
*      -- LOG_SUCCESS: 成功
* 作者: duanyongxing
* 时间 : 2009-10-11
*********************************************************************/ 
int Write_Log_Text(LPLOG_DATA lpLogData) 
{ 
	char szFilePath[MAX_FILE_PATH]; 
	char szFileName[MAX_LOG_FILE_NAME_LEN]; 
	FILE *pFile = NULL; 
	char szLogText[MAX_LOGTEXT_LEN]; 
	memset(szFilePath, 0, MAX_FILE_PATH); 
	memset(szFileName, 0, MAX_LOG_FILE_NAME_LEN); 
	memset(szLogText, 0, MAX_LOGTEXT_LEN); 
	GetLogPath(szFilePath); 
	GetLogFileName(lpLogData->iType, szFilePath, szFileName); 
	pFile = fopen(szFileName, "a+"); 
	if(NULL == pFile) 
	{ 
		return LOG_FAILED; 
	} 
	sprintf(szLogText, "<%08x> %s %s %s\n", g_LogIdentityCode, lpLogData->strDate, 
		lpLogData->strTime, lpLogData->strText); 
	fwrite(szLogText, 1, strlen(szLogText), pFile); 
	fclose(pFile); 
	return LOG_SUCCESS; 
} 
