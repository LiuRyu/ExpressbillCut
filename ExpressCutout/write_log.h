#ifndef __WRITELOG_H__  
#define __WRITELOG_H__ 

#define _LOG_WRITE_STATE_ 1				/* 条件编译开关，1：写日志，0：不写日志 */
#define LOG_SUCCESS (0)  
#define LOG_FAILED  (-1)  
#define LOG_BOOL_TRUE (1)  
#define LOG_BOOL_FALSE (0)  
#define DWORD_NULL  (0xFFFFFFFF)  
#define MAX_LOGTEXT_LEN (2048)			/* 每行日志的最大长度 */
#define MAX_FILE_PATH (255)				/* 日志文件路径的最大长度 */
#define MAX_LOG_FILE_SIZE (1024 * 1024)	/* 日志文件内容的最大长度 */
#define MAX_LOG_FILE_NAME_LEN (256)		/* 日志文件名的最大长度 */

#define LOG_TYPE_INFO    0				/* 日志类型: 信息类型 */
#define LOG_TYPE_ERROR   1				/* 日志类型: 错误类型 */
#define LOG_TYPE_SYSTEM  2				/* 日志类型: 系统类型 */
#define TEST_CASE_MAX_FILE_LEN (1024)	/* 测试函数中文件内容最大长度 */

int GenerateLogIdCode();
int SetLogPath(char *pStrPath);
void Write_Log(unsigned int uiLogType, char *pstrFmt, ...); 

#endif	__WRITELOG_H__