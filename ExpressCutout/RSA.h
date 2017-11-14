//#include <afx.h>
#include "stdafx.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <conio.h>  
#include <sstream>
using namespace std;

#define MAX_CODE_NUM 100   
#define FORMAT_MSG_BUFFER_SIZE 204800 
struct slink   
{    
	int  nBigNums[MAX_CODE_NUM];   /*nBigNums[98]用来标记正负号，1正，0负nBigNums[99]来标记实际长度*/   
	struct slink *pNext;   
};

void printIntArray(int a[MAX_CODE_NUM]);
void RSA_PrintStruct(struct slink *h);

void RSA_Genarate(int e[MAX_CODE_NUM],int d[MAX_CODE_NUM],int n[MAX_CODE_NUM]);

void RSA_LoadEnKey(char *loadFilename, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM]);
int RSA_LoadDeKey(char *loadFilename, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM]);
void RSA_SaveEnKey(int e[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename);
void RSA_SaveDekey(int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename);

////自成一套系统
//void RSA_fcEncode(char *cPriFilename,int e[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cCode);
//void RSA_fsEncode(char *cPriFilename, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sCode);
//void RSA_ssEncode(char *sInput, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sCode);
//void RSA_fcDecode(char *cCodeFilename,int d[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cResult);
int RSA_fsDecode(char *cCodeFilename, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM], string &sResult);
//void RSA_ssDecode(char *sCode, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sResult);

//自成一套系统
struct slink *RSA_Input(char *cInputs,int nLen);//输入原文，进行编码变换
struct slink *RSA_Encode(struct slink *pInput,int e[MAX_CODE_NUM],int  n[MAX_CODE_NUM],char *cCodes);//对原文编码进行加密，得到密文编码
void RSA_Decode(struct  slink *pCode,int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *cOutputs);//对密文编码进行解密










