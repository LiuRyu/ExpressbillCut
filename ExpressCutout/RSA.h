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
	int  nBigNums[MAX_CODE_NUM];   /*nBigNums[98]������������ţ�1����0��nBigNums[99]�����ʵ�ʳ���*/   
	struct slink *pNext;   
};

void printIntArray(int a[MAX_CODE_NUM]);
void RSA_PrintStruct(struct slink *h);

void RSA_Genarate(int e[MAX_CODE_NUM],int d[MAX_CODE_NUM],int n[MAX_CODE_NUM]);

void RSA_LoadEnKey(char *loadFilename, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM]);
int RSA_LoadDeKey(char *loadFilename, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM]);
void RSA_SaveEnKey(int e[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename);
void RSA_SaveDekey(int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename);

////�Գ�һ��ϵͳ
//void RSA_fcEncode(char *cPriFilename,int e[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cCode);
//void RSA_fsEncode(char *cPriFilename, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sCode);
//void RSA_ssEncode(char *sInput, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sCode);
//void RSA_fcDecode(char *cCodeFilename,int d[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cResult);
int RSA_fsDecode(char *cCodeFilename, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM], string &sResult);
//void RSA_ssDecode(char *sCode, int d[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sResult);

//�Գ�һ��ϵͳ
struct slink *RSA_Input(char *cInputs,int nLen);//����ԭ�ģ����б���任
struct slink *RSA_Encode(struct slink *pInput,int e[MAX_CODE_NUM],int  n[MAX_CODE_NUM],char *cCodes);//��ԭ�ı�����м��ܣ��õ����ı���
void RSA_Decode(struct  slink *pCode,int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *cOutputs);//�����ı�����н���










