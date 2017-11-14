/************************************************************************/ 
/*
  * �ļ����ƣ�write_log.cpp
  * ժ    Ҫ�����ļ�ʵ������ͨWINDOWS�����е���־����
  *           ��Ҫ�������ص㣺
  *           1. �������ڴ�����־�ļ�Ŀ¼��ÿ�����־�ֱ����ڲ�ͬ����־Ŀ¼�У�
  *           2. ��־���ݷ��������ͣ����ݲ�ͬ��Ҫ��д��ͬ����־���͵���־�ļ���
  *              ����ͨ����־��λ���������⣻
  *           3. ���������ȽϺõķ�װ�����ڸ��ã�
  *           ���Ľ��㣺
  *           1. Ϊ�˷��㣬��־���ݴ�ӡʱʹ����time�������侫ȷ�Ƚϵͣ�
  *           2. �ɽ���Щ������װΪһ����־�࣬���߶�̬�⣬ʹ���ͨ�ã�
  *           3. û�п��ǿ�ƽ̨�龰��Ŀǰֻʹ����WINDOWS��
  *           4. ��־�ļ����ݻ��ɽ�һ���Ľ��������ӡ����ǰ�ļ������кţ�ʹ����־����
  *              ����ʵ�ã�
  *
  * ��ǰ�汾��1.0
  * ��    �ߣ�duanyongxing 
  * ������ڣ�2009��10��11��
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

static char g_LogRootPath[MAX_FILE_PATH] = "C://ALGOLOG";	/* Ĭ����־�ļ���·�����������û��޸� */
static int g_LogIdentityCode = 0;

#pragma pack(push, 1)
typedef struct tagLOG_DATA				/* ��־���ݽṹ��*/ 
{ 
 char			strDate[11];			/* ����:��ʽΪ��:2009-10-11*/ 
 char			strTime[9];				/* ʱ��:��ʽΪ��:16:10:57*/ 
 unsigned int	iType;					/* ��־����:3��:INFO(0)/ERROR(1)/SYSTEM(2)*/ 
 char			strText[MAX_LOGTEXT_LEN];	/*��־����*/ 
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
* ��������:void TestLogCase_One()
* ˵��:�򵥵Ĳ��Ժ��������ļ�
* �����ߣ�main
* �������:
* ��
* ���������
* ��
* ����ֵ:
* void  -- 
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
* ��������:void Write_Log(unsigned int uiLogType, char *pstrFmt, ...)
* ˵��:��־д������֧�ֱ䳤����
* �����ߣ��κ���Ҫд��־�ĵط�
* �������:
* unsigned iType --  ��־���
* char *pstrFmt  --  ��־����
* ...            --  �䳤����
* ���������
* ��
* ����ֵ:
* void  -- 
* ����: duanyongxing
* ʱ�� : 2009-10-11
*********************************************************************/ 
void Write_Log(unsigned int uiLogType, char *pstrFmt, ...) 
{ 
#if _LOG_WRITE_STATE_   /* д��־���ı��뿪��*/  
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
* ��������:int  GetLogPath(char *pStrPath)
* ˵��:��ȡ��־�ļ�·��
* �����ߣ�Write_Log_Text
* �������:
* ��
* ���������
* char *pStrPath
* ����ֵ:
* int  -- LOG_FAILED:  ʧ��
*      -- LOG_SUCCESS: �ɹ�
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
    /* ������������ļ�������*/ 
	sprintf(pStrPath, "%s//%d%02d%02d", g_LogRootPath, mt->tm_year + 1900, 
	mt->tm_mon + 1, mt->tm_mday); 
    iRet = Create_LogDir(pStrPath); 
	return iRet; 
} 
/*********************************************************************
* ��������:int GetLogFileName(int iLogType, const char *pStrPath, char *pStrName)
* ˵��:��ȡ��־�ļ���
* �����ߣ�Write_Log_Text
* �������:
* int iLogType         -- ��־���� 3��:INFO(0)/ERROR(1)/SYSTEM(2)
* const char *pStrPath -- ��־·�� ��GetLogPath�õ�
* ���������
* char *pStrName       -- ��־�ļ���
* ����ֵ:
* int  -- LOG_FAILED:  ʧ��
*      -- LOG_SUCCESS: �ɹ�
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
// 		/* ����ļ����ȴ���ָ������󳤶ȣ����´���һ�ļ�������ԭ�ļ�*/ 
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
		 	/* ����ļ����ȴ���ָ������󳤶ȣ�������һ���*/ 
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
* ��������:int Create_LogDir(const char *pStrPath)
* ˵��:������־���·��
* �����ߣ�GetLogPath
* �������:
* const char *pStrPath --�û�ָ���ĸ�·��
* ���������
* ��
* ����ֵ:
* int  -- LOG_FAILED:  ʧ��
*      -- LOG_SUCCESS: �ɹ�
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
     
	/* ��㴴��Ŀ¼*/ 
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
* ��������:int Create_LogFile(const char *pStrFile, int iPos)
* ˵��:������־�ļ�
* �����ߣ�GetLogFileName
* �������:
* const char *pStrFile --�ļ���
* int iPos             --�ļ�ָ��λ��
* ���������
* ��
* ����ֵ:
* int  -- LOG_FAILED:  ʧ��
*      -- LOG_SUCCESS: �ɹ�
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
* ��������:int IsFileExist(const char *pStrFile)
* ˵��:�ж�ָ�����ļ��Ƿ����
* �����ߣ�GetLogFileName
* �������:
* const char *pStrFile --�ļ���
* ���������
* ��
* ����ֵ:
* int  -- LOG_BOOL_FALSE:  ������
*      -- LOG_BOOL_TRUE: ����
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
* ��������:DWORD GetFileLenth(const char *pFile)
* ˵��:�ж�ָ�����ļ���С
* �����ߣ�GetLogFileName
* �������:
* const char *pFile --�ļ���
* ���������
* ��
* ����ֵ:
* DWORD -- �ļ���С
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
* ��������:int Write_Log_Text(LPLOG_DATA lpLogData)
* ˵��:д��־����
* �����ߣ�Write_Log
* �������:
* LPLOG_DATA lpLogData --��־���ݽṹ����
* ���������
* ��
* ����ֵ:
* int  -- LOG_FAILED:  ʧ��
*      -- LOG_SUCCESS: �ɹ�
* ����: duanyongxing
* ʱ�� : 2009-10-11
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
