#ifndef __WRITELOG_H__  
#define __WRITELOG_H__ 

#define _LOG_WRITE_STATE_ 1				/* �������뿪�أ�1��д��־��0����д��־ */
#define LOG_SUCCESS (0)  
#define LOG_FAILED  (-1)  
#define LOG_BOOL_TRUE (1)  
#define LOG_BOOL_FALSE (0)  
#define DWORD_NULL  (0xFFFFFFFF)  
#define MAX_LOGTEXT_LEN (2048)			/* ÿ����־����󳤶� */
#define MAX_FILE_PATH (255)				/* ��־�ļ�·������󳤶� */
#define MAX_LOG_FILE_SIZE (1024 * 1024)	/* ��־�ļ����ݵ���󳤶� */
#define MAX_LOG_FILE_NAME_LEN (256)		/* ��־�ļ�������󳤶� */

#define LOG_TYPE_INFO    0				/* ��־����: ��Ϣ���� */
#define LOG_TYPE_ERROR   1				/* ��־����: �������� */
#define LOG_TYPE_SYSTEM  2				/* ��־����: ϵͳ���� */
#define TEST_CASE_MAX_FILE_LEN (1024)	/* ���Ժ������ļ�������󳤶� */

int GenerateLogIdCode();
int SetLogPath(char *pStrPath);
void Write_Log(unsigned int uiLogType, char *pstrFmt, ...); 

#endif	__WRITELOG_H__