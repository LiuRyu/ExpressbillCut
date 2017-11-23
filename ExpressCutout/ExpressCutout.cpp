// ExpressCutout.cpp : 定义 DLL 应用程序的入口点。
//

#include "stdafx.h"
#include <math.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "time.h"
#include "RyuCore.h"
#include "ExpressBarcodeDetection.h"
#include "PerspectiveTrans.h"
#include "ExBillCutout.h"
#include "ExpressCutout.h"
#include <vector>
#include "ComputerInfo.h"
#include "RSA.h"
#include  <io.h>

#ifdef _DEBUG_FLAG
#include "OpenCv_debugTool.h"
#else
#ifdef INPUTTYPE_FILEPATH
#include "OpenCv_debugTools.h"
#endif
#endif

 #define MAX(a,b) ((a)>(b)?(a):(b))
 #define MIN(a,b) ((a)>(b)?(b):(a))
using namespace std;

// 算法方法
#define USE_PERSPECTIVE_TRANS

int nIsExpressCutoutInit = 0;
int nIsExpressCutoutSet = 0;

int nInitImgWid = 0, nInitImgHei = 0;
//float nSetBillWidth = 450, nSetBillHeight = 350, nSetCodeWidth = 100, nSetCodeOffsetX = 280, nSetCodeOffsetY = 100;
float nSetBillWidth = 550, nSetBillHeight = 500, nSetCodeWidth = 100, nSetCodeOffsetX = 300, nSetCodeOffsetY = 200;

unsigned char * algoResult = 0;
unsigned char * gRotateImage = 0;

int *sum=0;
float *fsum=0;

int mSuccessFrame = 0, mTotalCntFrame = 0;

std::vector<std::string> split(std::string str, std::string pattern);
int ExpressCutout_initEx(int imageWidth, int imageHeight);

std::vector<std::string> split(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;//扩展字符串以方便操作
	int size = str.size();

	for (int i = 0; i<size; i++)
	{
		pos = str.find(pattern, i);
		if (pos<size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}
#ifdef _WRITE_LOG
int ExpressCutout_init(int imageWidth, int imageHeight, char * LogPath)

#else
int ExpressCutout_init(int imageWidth, int imageHeight)
#endif
{
	char p[100];
	int ret_val = 0;

#ifdef INPUTTYPE_FILEPATH
#ifdef _WRITE_LOG
	SetLogPath(LogPath);
#endif
#endif

#ifdef _WRITE_LOG
	//GenerateLogIdCode();
	Write_Log(LOG_TYPE_SYSTEM, "++++++++++++++++++++++++++++++++");
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm init...");
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "++++++++++++++++++++++++++++++++");
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm init...");
#endif
#endif

	//////////////////////////////////////////////////////////////////////////
	//加入Code校验 ivycc 2017-03-06
	bool bMacMach = false;
	bool bMainBoardMatch = false;
	string sResult;
	int nArrayD[MAX_CODE_NUM], nArrayN[MAX_CODE_NUM];
	char *DeKeyFilename = "./DeKey.txt";
	char *CodeFilename = "./Code.txt";
	vector<std::string> machineInfo;
	string mainBoard;

	vector<string> vector_mac;
	int ret = RSA_LoadDeKey(DeKeyFilename, nArrayD, nArrayN);//得到D、N
	if (ret < 0)
	{
		ret_val = -20000001;
		goto nExit;
	}
	ret = RSA_fsDecode(CodeFilename, nArrayD, nArrayN, sResult);//利用D、N解密
	if (ret < 0)
	{
		ret_val = -20000002;
		goto nExit;
	}
	machineInfo = split(sResult, ";");

	//MAC地址+主板序列号
	char lpszBaseBoard[128];
	memset(lpszBaseBoard, 0, 128);
	int nameLength = GetBaseBoardByCmd(lpszBaseBoard);

	//获取三遍 有时不一定能获取到
	mainBoard = lpszBaseBoard;
	if (mainBoard == "")
	{
		GetBaseBoardByCmd(lpszBaseBoard);
		mainBoard = lpszBaseBoard;
		if (mainBoard == "")
		{
			GetBaseBoardByCmd(lpszBaseBoard);
			mainBoard = lpszBaseBoard;
		}
	}
	GetMAC(vector_mac);


	machineInfo[0] = machineInfo[0] + "\n";
	machineInfo[1] = machineInfo[1];
	int count = vector_mac.size();
	for (int i = 0; i < count; i++)
	{
		if (vector_mac[i].compare(machineInfo[0]) == 0)
		{
			bMacMach = true;
			if (mainBoard.compare(machineInfo[1]) == 0 || machineInfo[1] == "000")
			{
				bMainBoardMatch = true;
				break;
			}

		}
	}

	if (!bMacMach)
	{
		ret_val = -20000003;
		vector<string> temp;
		vector_mac.swap(temp);
		vector<string> temp1;
		machineInfo.swap(temp1);
		goto nExit;
	}
	if (!bMainBoardMatch)
	{
		ret_val = -20000004;
		vector<string> temp;
		vector_mac.swap(temp);
		vector<string> temp1;
		machineInfo.swap(temp1);
		goto nExit;
	}
	//////////////////////////////////////////////////////////////////////////
	
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- RSA init done.");
#endif

	if(nIsExpressCutoutInit) {
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "警告:算法重复调用初始化. InitFlag=%d", nIsExpressCutoutInit);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "WARN- 警告:算法重复调用初始化. InitFlag=%d", nIsExpressCutoutInit);
#endif
#endif
		ExpressCutout_release();
	}

	if(imageWidth <= 0 || imageHeight <= 0) {
		ret_val = -10503004;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误-[%d]. wid=0x%x, hei=0x%x", 
			ret_val, imageWidth, imageHeight);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 初始化错误-[%d]. wid=0x%x, hei=0x%x", 
			ret_val, imageWidth, imageHeight);
#endif
#endif
		goto nExit;
	}

	nInitImgWid = imageWidth;
	nInitImgHei = imageHeight;

	ret_val = algorithm_init(imageWidth, imageHeight, &algoResult);
	if(1 != ret_val) {
		ret_val = -10503006;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误. 错误号:%d", ret_val);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 初始化错误. 错误号:%d", ret_val);
#endif
#endif
		goto nExit;
	}

	ret_val = ExBillCut_init(imageWidth, imageHeight);
	if(1 != ret_val) {
		ret_val = -10503029;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d.", ret_val);
#endif
		goto nExit;
	}

	gRotateImage = (unsigned char *)malloc((imageWidth<<2) * (imageWidth<<2) * sizeof(unsigned char));
	if(!gRotateImage) {
		ret_val = -10503028;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误. 错误号:%d. im=0x%x", ret_val, gRotateImage);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 初始化错误. 错误号:%d. im=0x%x", ret_val, gRotateImage);
#endif
#endif
		goto nExit;
	}
	memset(gRotateImage, 0, (imageWidth<<2) * (imageWidth<<2) * sizeof(unsigned char));

	nIsExpressCutoutInit = 1;
	ret_val = 1;

nExit:
#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm init done-[%d].", ret_val);
	Write_Log(LOG_TYPE_SYSTEM, "++++++++++++++++++++++++++++++++");
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm init done-[%d].", ret_val);
	Write_Log(LOG_TYPE_INFO, "++++++++++++++++++++++++++++++++");
#endif
#endif
	return ret_val;
}

void ExpressCutout_release()
{
#ifdef _WRITE_LOG
	//GenerateLogIdCode();
	Write_Log(LOG_TYPE_SYSTEM, "++++++++++++++++++++++++++++++++");
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm release...");
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "++++++++++++++++++++++++++++++++");
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm release...");
#endif
#endif

	nIsExpressCutoutInit = 0;
	algorithm_release();

	ExBillCut_release();

	if(gRotateImage) {
		free(gRotateImage);
		gRotateImage = 0;
	}

#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm release done.");
	Write_Log(LOG_TYPE_SYSTEM, "++++++++++++++++++++++++++++++++");
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm release done.");
	Write_Log(LOG_TYPE_INFO, "++++++++++++++++++++++++++++++++");
#endif
#endif

	return;
}

unsigned char * ExpressCutout_getFullResult()
{
	return algoResult;
}

int ExpressCutout_getSuccessCount()
{
	printf("算法内部统计！共完成%d帧，其中可识别%d帧，识别率%.3f\n",\
		mTotalCntFrame, mSuccessFrame, mSuccessFrame*1.0/mTotalCntFrame);
	return mSuccessFrame;
}


#ifndef INPUTTYPE_FILEPATH
int ExpressCutout_run(unsigned char * imageData, int * imageWidth, int * imageHeight, char * code, int * codeLength)
#else
int ExpressCutout_run(char * InputImagePath, char * CutOutPath, char * SuccessPath, 
	char * FailPath, char * ErrorPath, char * code, int * codeLength)
#endif
{
#ifdef _WRITE_LOG
	// 生成日志序号
	GenerateLogIdCode();
#endif

#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "================================");
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm entry...");
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "================================");
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm entry...");
#endif
#endif
	int ret_val = 0, status = 0;
	int inputImgWid = 0, inputImgHei = 0;

	AlgorithmResult * result_node = 0;
	int * result_cnt = 0;

	int i = 0, j = 0;
	int nTmp = 0;
	AlgorithmResult tmpNode;

	int codeX = 0, codeY = 0;
	int alpha = 0;
	float rate = 0, gain_rate = 0;

	int billX[4] = {0}, billY[4] = {0}, rltvX[4] = {0}, rltvY[4] = {0};
	int ptBill[4] = {0};
	
#ifdef USE_PERSPECTIVE_TRANS
	int srcX[8] = {0}, dstX[8] = {0};
	double matrix[9];
#endif

	int isFindCode = 0, isCodeValid = 0;

	int rotateWid = 0, rotateHei = 0;

	const float pi = 3.1415927 / 180.0;
	float fTmp0 = 0, fTmp1 = 0;

#ifdef INPUTTYPE_FILEPATH
	IplImage * iplInData = 0;
	IplImage * iplCutData = 0;
	unsigned char * imageData = 0;
	int cutImgWid = 0, cutImgHei = 0;
#endif

#ifdef _LOG_COSTTIME
	// 记录算法运行时
	long TimeCost = 0;
	LARGE_INTEGER m_frequency = {0}, m_time1 = {0}, m_time2 = {0};
	int support = QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_time2);
#endif

// 	if(!nIsExpressCutoutInit) {
// 		ret_val = -10503030;
// #ifdef _WRITE_LOG
// 		Write_Log(LOG_TYPE_ERROR, "错误号: %d. nIsExpressCutoutInit=%d", 
// 			ret_val, nIsExpressCutoutInit);
// #endif
// 		goto nExit;
// 	}

#ifndef INPUTTYPE_FILEPATH
	if(!imageData || !imageWidth || !imageHeight || !code || !codeLength) {
		ret_val = -10503031;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. imageData=0x%x, imageWidth=0x%x, \
			imageHeight=0x%x, code=0x%x, codeLength=0x%x", 
			ret_val, imageData, imageWidth, imageHeight, code, codeLength);
#endif
		goto nExit;
	}

	inputImgWid = *imageWidth;
	inputImgHei = *imageHeight;
#else
	if(!InputImagePath || !CutOutPath || !SuccessPath || !FailPath || !ErrorPath
		|| !code || !codeLength) {
		ret_val = -20000101;			// 给入参数错误
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号:%d. InputImagePath=0x%x, CutOutPath=0x%x, \
			SuccessPath=0x%x, FailPath=0x%x, ErrorPath=0x%x, code=0x%x, codeLength=0x%x", 
			ret_val, InputImagePath, CutOutPath, SuccessPath, FailPath, ErrorPath, code, codeLength);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 错误号:%d. InputImagePath=0x%x, CutOutPath=0x%x, \
			SuccessPath=0x%x, FailPath=0x%x, ErrorPath=0x%x, code=0x%x, codeLength=0x%x", 
			ret_val, InputImagePath, CutOutPath, SuccessPath, FailPath, ErrorPath, code, codeLength);
#endif
#endif
		goto nExit;
	}

#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "IMPATH- %s", InputImagePath);
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "IMPATH- %s", InputImagePath);
#endif
#endif

	// 根据文件路径读取图像
	iplInData = cvLoadImage(InputImagePath, CV_LOAD_IMAGE_GRAYSCALE);
	if(0 == iplInData) {
		ret_val = -20000102;			// 无法读取图片，文件移动至ERROR文件夹
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号:%d. iplInData=0x%x", ret_val, iplInData);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 错误号:%d. iplInData=0x%x", ret_val, iplInData);
#endif
#endif
		goto nExit;
	}
	inputImgWid = iplInData->widthStep;
	inputImgHei = iplInData->height;
	imageData = (unsigned char *)iplInData->imageData;
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- cvLoadImage done.");
#endif
#endif

	if(*codeLength < 0) {
		ret_val = -10503035;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. *codeLength=%d", ret_val, *codeLength);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. *codeLength=%d", ret_val, *codeLength);
#endif
#endif
		goto nExit;
	}

	if(inputImgWid <= 0 || inputImgHei <= 0) {
		ret_val = -10503032;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. inputImgWid=%d, inputImgHei=%d",
			ret_val, inputImgWid, inputImgHei);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. inputImgWid=%d, inputImgHei=%d",
			ret_val, inputImgWid, inputImgHei);
#endif
#endif
		goto nExit;
	}

	if(!nIsExpressCutoutInit) {
		ret_val = -10503030;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "警告：未经初始化的算法调用. 错误号:%d. nIsExpressCutoutInit=%d", 
			ret_val, nIsExpressCutoutInit);
		Write_Log(LOG_TYPE_SYSTEM, "警告：未经初始化的算法调用. 错误号:%d. nIsExpressCutoutInit=%d", 
			ret_val, nIsExpressCutoutInit);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "WARN- 警告：未经初始化的算法调用. 错误号:%d. nIsExpressCutoutInit=%d", 
			ret_val, nIsExpressCutoutInit);
#endif
#endif
		nInitImgWid = inputImgWid;
		nInitImgHei = inputImgHei;
#ifndef INPUTTYPE_FILEPATH
		status = ExpressCutout_init(nInitImgWid, nInitImgHei);
#else
		status = ExpressCutout_init(nInitImgWid, nInitImgHei, 0);
#endif
		if(1 != status) {
			ret_val = -10503036;
#ifdef _WRITE_LOG
			Write_Log(LOG_TYPE_ERROR, "Algorithm re-initialize failed. 错误号:%d. status=%d",
				ret_val, status);
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "ERROR- Algorithm re-initialize failed. 错误号:%d. status=%d",
				ret_val, status);
#endif
#endif
			goto nExit;
		}
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "TRACE- ExpressCutout_init done.");
#endif
		goto nExit;
	}

	// 如果给入图像宽高超过初始化图像宽高，则重新初始化
	/*if(inputImgWid != nInitImgWid || inputImgHei != nInitImgHei) {*/
	//解决不同大小图片需要切割的问题 2016.7.27
	if(inputImgWid > nInitImgWid || inputImgHei > nInitImgHei) {
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_SYSTEM, "Algorithm re-initialize, inputImgWid=%d, inputImgHei=%d", 
			inputImgWid, inputImgHei);
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm re-initialize, inputImgWid=%d, inputImgHei=%d", 
			inputImgWid, inputImgHei);
#endif
#endif
		ExpressCutout_release();

		nInitImgWid = inputImgWid;
		nInitImgHei = inputImgHei;
#ifndef INPUTTYPE_FILEPATH
		status = ExpressCutout_init(nInitImgWid, nInitImgHei);
#else
		status = ExpressCutout_init(nInitImgWid, nInitImgHei, 0);
#endif
		if(1 != status) {
			ret_val = -10503033;
#ifdef _WRITE_LOG
			Write_Log(LOG_TYPE_ERROR, "错误号: %d. Algorithm re-initialize failed. status=%d",
				ret_val, status);
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- 错误号: %d. Algorithm re-initialize failed. status=%d",
				ret_val, status);
#endif
#endif
			goto nExit;
		}
	}

#ifdef _DEBUG_
#ifdef _DEBUG_CUTOUT
	cvNamedWindow("original im");
	cvShowImage("original im", iplInData);
	cvWaitKey();
#endif
#endif

	memset(algoResult, 0, 10 * sizeof(AlgorithmResult) + 4);

	if(*codeLength)
		memset(code, 0, *codeLength * sizeof(char));
	else
		code[0] = 0;

	(mTotalCntFrame < 0x1fffffff) ? (mTotalCntFrame++) : (mTotalCntFrame = 0);

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Key algorithm enter...");
#endif
#endif
	status = algorithm_run(1, imageData, inputImgWid, inputImgHei, &algoResult);
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Key algorithm done. status=%d", status);
#endif
#endif
	if(status > 0)
		mSuccessFrame++;
	else if(-1 >= status) {
		ret_val = status;
		goto nExit;
	}

	result_cnt = (int *)algoResult;
	result_node = (AlgorithmResult*)(algoResult+4);

	// 按照条码大小进行排序
	for(i = 0; i < *result_cnt; i++) {
		for(j = i + 1; j < *result_cnt; j++) {
			if(result_node[i].nCodeWidth * result_node[i].nCodeHeight
				< result_node[j].nCodeWidth * result_node[j].nCodeHeight) {
					tmpNode = result_node[i];
					result_node[i] = result_node[j];
					result_node[j] = tmpNode;
			}
		}
	}

	// 依次对比位数, 去除非法字符
	for(i = 0; i < *result_cnt; i++) {
		// 检验位数
		if(*codeLength != strlen(result_node[i].strCodeData)
			&& 0 != *codeLength) 
			continue;
		// 依次检验字符
		isCodeValid = 1;
		for(j = 0; j < strlen(result_node[i].strCodeData); j++) {
			if(result_node[i].strCodeData[j] < '0' || result_node[i].strCodeData[j] > '9')
				isCodeValid = 0;
		}
		if(!isCodeValid)
			continue;

		strcpy(code, result_node[i].strCodeData);
		tmpNode = result_node[i];
		isFindCode = 1;
		break;
	}

	if(!isFindCode) {
		tmpNode = result_node[0];
		*codeLength = 0;
	} else {
		*codeLength = strlen(code);
	}

#ifdef _WRITE_LOG
	if (2 == tmpNode.nFlag)
		Write_Log(LOG_TYPE_SYSTEM, "Code recognized by OCR, digit=%d, code=%s", tmpNode.nCodeCharNum, tmpNode.strCodeData);
#endif

	//////////////////////////////////////////////////////////////////////////
	// start testing ExBillCutout
	int codeXY[4];
	codeXY[0] = tmpNode.ptCodeBound1;
	codeXY[1] = tmpNode.ptCodeBound2;
	codeXY[2] = tmpNode.ptCodeBound3;
	codeXY[3] = tmpNode.ptCodeBound4;

	ExBillCut_run(imageData, inputImgWid, inputImgHei, codeXY, tmpNode.nCodeOrient, 
		gRotateImage, &rotateWid, &rotateHei);

#ifdef _DEBUG_
//#ifdef _DEBUG_MAIN
	IplImage * iplOrigImage = cvCreateImage(cvSize(iplInData->width/4, iplInData->height/4), 8, 1);
	cvResize(iplInData, iplOrigImage);
	cvNamedWindow("original im");
	cvShowImage("original im", iplOrigImage);
	cvReleaseImage(&iplOrigImage);

	IplImage * iplRotateImage = cvCreateImage(cvSize(rotateWid, rotateHei), 8, 1);
	uc2IplImageGray(gRotateImage, iplRotateImage);
	CvFont font;
	char txt[50];
	cvInitFont(&font, CV_FONT_HERSHEY_COMPLEX_SMALL, 1.0, 1.0, 
		0.0, 1.0, CV_AA);
	sprintf(txt, "%s", tmpNode.strCodeData);
	cvPutText(iplRotateImage, txt, cvPoint(10, 20), &font, CV_RGB(255,255,255));
	sprintf(txt, "%s", code);
	cvPutText(iplRotateImage, txt, cvPoint(10, 50), &font, CV_RGB(255,255,255));
	cvNamedWindow("iplRotateImage");
	cvShowImage("iplRotateImage", iplRotateImage);
	cvWaitKey();
	cvReleaseImage(&iplRotateImage);
//#endif
#endif
	// end testing ExBillCutout
	//////////////////////////////////////////////////////////////////////////

/* START-注释张磊切割算法
	// 根据条码位置计算快递单位置
 	fTmp0 = fabs(sin(pi * tmpNode.nCodeOrient));
	fTmp1 = fabs(cos(pi * tmpNode.nCodeOrient));
	gain_rate = 1.0 + (1.0 - ((fTmp0 > fTmp1) ? fTmp0 : fTmp1));
	//rate = tmpNode.nCodeWidth * gain_rate / nSetCodeWidth;

	rate = tmpNode.nCodeWidth / nSetCodeWidth;

	codeX = (tmpNode.ptCodeBound1 >> 16);
	codeX = (codeX & 0x8000) ? (0xffff0000 | codeX) : codeX;
	codeY = (tmpNode.ptCodeBound1 & 0xffff);
	codeY = (codeY & 0x8000) ? (0xffff0000 | codeY) : codeY;

#ifdef _DEBUG_FLAG
	cvCircle(iplColor, cvPoint(codeX/2, codeY/2), 3, CV_RGB(255,0,0), CV_FILLED);
	nTmp = 0; sprintf(txt, "%d", nTmp);
	cvPutText(iplColor, txt, cvPoint(codeX/2+2, codeY/2), &font, CV_RGB(255,0,0));
#endif

	if(tmpNode.nCodeOrient >= 180) {
		alpha = tmpNode.nCodeOrient - 180;
		rltvX[0] = nSetCodeOffsetX * rate;
		rltvY[0] = nSetCodeOffsetY * rate;
		rltvX[1] = 0 - (nSetBillWidth - nSetCodeOffsetX) * rate;
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = 0 - (nSetBillHeight - nSetCodeOffsetY) * rate;
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	} else {
		alpha = tmpNode.nCodeOrient;
		rltvX[0] = 0 - nSetCodeOffsetX * rate;
		rltvY[0] = 0 - nSetCodeOffsetY * rate;
		rltvX[1] = (nSetBillWidth - nSetCodeOffsetX) * rate;
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = (nSetBillHeight - nSetCodeOffsetY) * rate;
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	}
	for(i = 0; i < 4; i++) {
		CoorRelative2Absolute(codeX, codeY, rltvX[i], rltvY[i], (unsigned char)alpha, &billX[i], &billY[i]);
		ptBill[i] = (billX[i] << 16) | (billY[i] & 0xffff);
#ifdef _DEBUG_FLAG
		nTmp = i; sprintf(txt, "%d", nTmp);
		cvCircle(iplColor, cvPoint(billX[i]/2, billY[i]/2), 3, CV_RGB(0,255,0), CV_FILLED);
		cvPutText(iplColor, txt, cvPoint(billX[i]/2+2, billY[i]/2), &font, CV_RGB(0,255,0));
#endif
	}

#ifdef _DEBUG_FLAG
	codeX = tmpNode.ptCodeBound2 >> 16;
	codeX = (codeX & 0x8000) ? (0xffff0000 | codeX) : codeX;
	codeY = tmpNode.ptCodeBound2 & 0xffff;
	codeY = (codeY & 0x8000) ? (0xffff0000 | codeY) : codeY;
	cvCircle(iplColor, cvPoint(codeX/2, codeY/2), 3, CV_RGB(255,0,0), CV_FILLED);
	nTmp = 1; sprintf(txt, "%d", nTmp);
	cvPutText(iplColor, txt, cvPoint(codeX/2+2, codeY/2), &font, CV_RGB(255,0,0));
#endif

#ifdef _DEBUG_FLAG
	codeX = tmpNode.ptCodeBound3 >> 16;
	codeX = (codeX & 0x8000) ? (0xffff0000 | codeX) : codeX;
	codeY = tmpNode.ptCodeBound3 & 0xffff;
	codeY = (codeY & 0x8000) ? (0xffff0000 | codeY) : codeY;
	cvCircle(iplColor, cvPoint(codeX/2, codeY/2), 3, CV_RGB(255,0,0), CV_FILLED);
	nTmp = 2; sprintf(txt, "%d", nTmp);
	cvPutText(iplColor, txt, cvPoint(codeX/2+2, codeY/2), &font, CV_RGB(255,0,0));
#endif

#ifdef _DEBUG_FLAG
	codeX = tmpNode.ptCodeBound4 >> 16;
	codeX = (codeX & 0x8000) ? (0xffff0000 | codeX) : codeX;
	codeY = tmpNode.ptCodeBound4 & 0xffff;
	codeY = (codeY & 0x8000) ? (0xffff0000 | codeY) : codeY;
	cvCircle(iplColor, cvPoint(codeX/2, codeY/2), 3, CV_RGB(255,0,0), CV_FILLED);
	nTmp = 3; sprintf(txt, "%d", nTmp);
	cvPutText(iplColor, txt, cvPoint(codeX/2+2, codeY/2), &font, CV_RGB(255,0,0));
#endif

#ifndef USE_PERSPECTIVE_TRANS
	nTmp = ptBill[2]; ptBill[2] = ptBill[3]; ptBill[3] = nTmp;

	status = IPA_RotateImg(imageData, (short)nInitImgWid, (short)nInitImgHei, 
		gRotateImage, (short*)&rotateWid, (short*)&rotateHei, 
		ptBill, tmpNode.nCodeOrient);
	if(1 != status || 0 >= rotateWid || 0 >= rotateHei) {
		ret_val = -10503034;
		goto nExit;
	}

#else
	//////////////////////////////////////////////////////////////////////////
	// 旋转方案二
	//rate = tmpNode.nCodeWidth / nSetCodeWidth;
	rotateWid = nSetBillWidth * rate;
	rotateHei = nSetBillHeight * rate;

	//解决不同大小图片需要切割的问题 2016.7.27
	if(rotateWid > inputImgWid || rotateHei > inputImgHei) {
		fTmp0 = rotateWid * 1.0 / inputImgWid;
		fTmp1 = rotateHei * 1.0 / inputImgHei;
		if(fTmp0 > fTmp1) {
			rotateWid = inputImgWid;
			rotateHei = rotateHei / fTmp0;
		} else {
			rotateHei = inputImgHei;
			rotateWid = rotateWid / fTmp1;
		}
	}


	srcX[0] = 0;		srcX[1] = 0;	srcX[2] = 0;		srcX[3] = rotateHei-1;
	srcX[4] = rotateWid-1;	srcX[5] = 0;	srcX[6] = rotateWid-1;	srcX[7] = rotateHei-1;
	dstX[0] = billX[0];	dstX[1] = billY[0];	dstX[2] = billX[2];	dstX[3] = billY[2];
	dstX[4] = billX[1];	dstX[5] = billY[1];	dstX[6] = billX[3];	dstX[7] = billY[3];

	// 运算次数较少,虽为浮点,却基本不耗时
	IPA_getPerspectiveTransformMat(srcX, dstX, matrix);
	//解决不同大小图片需要切割的问题 2016.7.27
	IPA_warpPerspectiveTransformFixed(imageData, inputImgWid, inputImgHei,
		gRotateImage, rotateWid, rotateHei, matrix);
	//////////////////////////////////////////////////////////////////////////
#endif

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Perspective Transform done.");
#endif
#endif

	int sum[50];
	float fsum[50];
	int Thr=130;
	ImageBinarization(gRotateImage,rotateWid,rotateHei,&Thr); 
	Thr-=15;
	memset(sum,0,50*sizeof(int));
	//memset(fsum,0,50*sizeof(float));
	int k=0;
	int w1=0,w2=rotateWid;
	
	float max=0;
	float fmax=0;
	int max_k=0;
	for(int i=0;i<rotateWid;i=i+rotateWid/20)
	{

		for(int j=0;j<rotateHei;j++)
		{
			if(gRotateImage[j*rotateWid+i]>Thr)
			{
				sum[k]+=1;
			//fsum[k]+=gRotateImage[j*rotateWid+i];
			}
		}
		//fsum[k]=fsum[k]/(sum[k]+0.0000001);
		k++;
	}

	for(int i=0;i<k;i++)
	{
		
		if(sum[i]/(1.0*rotateHei)>max)
		{
			//max_k=i*rotateWid/20;
			max=(sum[i]/(1.0*rotateHei));
		}

	}
	
	float st=rotateWid*0.02;
	int step =0;	
	if(st<1){
		step=1;
	}else{
		step=st;
	}
	int sum1=0;
	int sum2=0;
	int flag=0;

	for(int i=rotateWid*280/450;i>0;i-=step)
	{
		sum1=0;
		for(int j=0;j<rotateHei;j++)
		{
			if(gRotateImage[j*rotateWid+i]>Thr)
				sum1+=1;
		}
		if(sum1/(rotateHei*1.0)<(max*0.2)&&i>3*step)
		{
			sum2=0;
			for(int l=0;l<rotateHei;l++)
		      {
			     if(gRotateImage[(int)(l*rotateWid+i-3*step)]>Thr)
				   sum2+=1;
		      }
			if(sum2/(rotateHei*1.0)<(max*0.2))
			{
				w1=i-3*step;
				flag=1;
				break;
			}
			
		}		
		
	}
	if(flag==0)
	{
		w1=0;
	}
	sum1=0;
	sum2=0;
	flag=0;
	for(int i=rotateWid*380/450;i<rotateWid;i+=step)
	{
		sum1=0;
		for(int j=0;j<rotateHei;j++)
		{
			if(gRotateImage[j*rotateWid+i]>Thr)
				sum1+=1;
		}
		if(sum1/(rotateHei*1.0)<(max*0.2)&&i<(rotateWid-3*step))
		{
			sum2=0;
			for(int l=0;l<rotateHei;l++)
		      {
			     if(gRotateImage[(int)(l*rotateWid+i+3*step)]>Thr)
				   sum2+=1;
		      }
			if(sum2/(rotateHei*1.0)<(max*0.2))
			{
				w2=i+3*step;
				flag=1;
				break;
			}
		}
		
	}
	if(flag==0)
	{
		w2=rotateWid;
	}


	memset(sum,0,50*sizeof(int));
	//memset(fsum,0,50*sizeof(float));
	 k=0;
	int h1=0,h2=rotateHei;
	
	 max=0;
	// fmax=0;
	 max_k=0;
	for(int i=0;i<rotateHei;i=i+rotateHei/20)
	{

		for(int j=0;j<rotateWid;j++)
		{
			if(gRotateImage[i*rotateWid+j]>Thr)
			{
				sum[k]+=1;
				//fsum[k]+=gRotateImage[i*rotateWid+j];
			}
		}
		//fsum[k]=fsum[k]/(sum[k]+0.00000001);
		k++;
	}

	for(int i=0;i<k;i++)
	{
		
		if(sum[i]/(1.0*rotateWid)>max)
		{
			//max_k=i*rotateHei/20;
			max=(sum[i]/(1.0*rotateWid));
		}

	}
	
	 st=rotateHei*0.02;
	 if(st<1){
		step=1;
	}else{
		step=st;
	}
	 sum1=0;
	 sum2=0;
	 flag=0;

	for(int i=rotateHei*100/350;i>0;i-=step)
	{
		sum1=0;
		for(int j=0;j<rotateWid;j++)
		{
			if(gRotateImage[i*rotateWid+j]>Thr)
				sum1+=1;
		}
		if(sum1/(rotateWid*1.0)<(max*0.2)&&i>3*step)
		{
			sum2=0;
			for(int l=0;l<rotateWid;l++)
		      {
			     if(gRotateImage[(int)((i-3*step)*rotateWid+l)]>Thr)
				   sum2+=1;
		      }
			if(sum2/(rotateWid*1.0)<(max*0.2))
			{
				h1=i-3*step;
				flag=1;
				break;
			}
			
		}		
		
	}
	if(flag==0)
	{
		h1=0;
	}
	sum1=0;
	sum2=0;
	flag=0;
	for(int i=rotateHei*130/350;i<rotateHei;i+=step)
	{
		sum1=0;
		for(int j=0;j<rotateWid;j++)
		{
			if(gRotateImage[i*rotateWid+j]>Thr)
			
				sum1+=1;
		}
		if(sum1/(rotateWid*1.0)<(max*0.2)&&i<(rotateHei-3*step))
		{
			sum2=0;
			for(int l=0;l<rotateWid;l++)
		      {
			     if(gRotateImage[(int)((i+3*step)*rotateWid+l)]>Thr)
				   sum2+=1;
		      }
			if(sum2/(rotateWid*1.0)<(max*0.2))
			{
				h2=i+3*step;
				flag=1;
				break;
			}
		}
		
	}
	if(flag==0)
	{
		h2=rotateHei;
	}

#ifndef INPUTTYPE_FILEPATH
	if(w2>w1&&h2>h1)
	{
		for(int i=0;i<(h2-h1+1);i++)
		  for(int j=0;j<(w2-w1+1);j++)
		  {
			  imageData[i*(w2-w1+1)+j]=gRotateImage[(i+h1)*rotateWid+j+w1];
		  }

		*imageWidth = w2-w1+1;
		*imageHeight = h2-h1+1;
	}
#else	
	if(w2>w1&&h2>h1) {
		cutImgWid = w2-w1+1;
		cutImgHei = h2-h1+1;
		iplCutData = cvCreateImage(cvSize(cutImgWid, cutImgHei), 8, 1);
		if(!iplCutData) {
			ret_val = -20000105;	
#ifdef _WRITE_LOG
			Write_Log(LOG_TYPE_ERROR, "错误号: %d. iplCutData=0x%x", ret_val, iplCutData);
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. iplCutData=0x%x", ret_val, iplCutData);
#endif
#endif
			goto nExit;
		}
		for(int i = 0; i < cutImgHei; i++) {
			for(int j = 0; j  <cutImgWid; j++) {
				iplCutData->imageData[i*iplCutData->widthStep+j] = gRotateImage[(i+h1)*rotateWid+j+w1];
			}
		}
	}
#endif
注释张磊切割算法-END */

#ifndef INPUTTYPE_FILEPATH
	memcpy(imageData, gRotateImage, rotateWid * rotateHei * sizeof(unsigned char));
	*imageWidth = rotateWid;
	*imageHeight = rotateHei;
#else
	iplCutData = cvCreateImage(cvSize(rotateWid, rotateHei), 8, 1);
	if(!iplCutData) {
		ret_val = -20000105;	
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. iplCutData=0x%x", ret_val, iplCutData);
#endif
		goto nExit;
}
	uc2IplImageGray(gRotateImage, iplCutData);
#endif

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Express List find and cut done.");
#endif
#endif
	
	if(isFindCode && *codeLength > 0)
		ret_val = 2;
	else
		ret_val = 1;

nExit:
#ifdef INPUTTYPE_FILEPATH
	// 移动图像文件，保存切割图片
	if(ret_val > 0) {
		if(iplCutData) {
			std::string strName(CutOutPath);
			std::string strPath(CutOutPath);
			int nPos = strPath.rfind('\\');
			if (-1 != nPos)
			{
				strName = strPath.substr(nPos + 1, strName.length() - nPos - 1);
				strPath.assign(strPath.c_str(), nPos++);
				//strPath = strPath.substr(nPos, strPath.length() - strName.length-1);
			}
			
			std::string NewPath  = strPath +"\\"+ code + "-" + strName;
			const char * path = NewPath.c_str();
			cvSaveImage(path, iplCutData);
			
			int mf_ret = MoveFileEx(InputImagePath, SuccessPath, MOVEFILE_REPLACE_EXISTING);
			if(1 != mf_ret) {
				ret_val = -20000103;		// 切割成功，但是无法移动文件
#ifdef _WRITE_LOG
				Write_Log(LOG_TYPE_ERROR, "错误号: %d. Cannot move imfile.", ret_val);
#ifdef _LOG_TRACE
				Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. Cannot move imfile.", ret_val);
#endif
#endif				
			}
		} else {
			int mf_ret = MoveFile(InputImagePath, FailPath);
			if(1 != mf_ret) {
				ret_val = -20000104;		// 切割失败，但是无法移动文件
#ifdef _WRITE_LOG
				Write_Log(LOG_TYPE_ERROR, "错误号: %d. Cannot move imfile.", ret_val);
#ifdef _LOG_TRACE
				Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. Cannot move imfile.", ret_val);
#endif
#endif	
			}
		}
	} else if(ret_val < -10000000) {
		MoveFile(InputImagePath, ErrorPath);
	} else {
		int mf_ret = MoveFile(InputImagePath, FailPath);
		if(1 != mf_ret) {
			ret_val = -20000104;		// 切割失败，且无法移动文件
#ifdef _WRITE_LOG
			Write_Log(LOG_TYPE_ERROR, "错误号: %d. Cannot move imfile.", ret_val);
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "ERROR- 错误号: %d. Cannot move imfile.", ret_val);
#endif
#endif	
		}
	}

	// 释放图像内存
	if(iplInData)
		cvReleaseImage(&iplInData);
	if(iplCutData)
		cvReleaseImage(&iplCutData);
#endif

#ifdef _LOG_COSTTIME
	QueryPerformanceCounter((LARGE_INTEGER*) &m_time1);
	TimeCost = 1000.0*(m_time1.QuadPart-m_time2.QuadPart)/m_frequency.QuadPart;
#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "Timecost=%ldms", TimeCost);
#endif
#ifdef _PRINT_LOG
	printf("[SYSTEM] Algorithm exit! ret=%d, timecost=%ldms", ret_val, TimeCost);
#endif
#endif

#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm exit! ret=%d", ret_val);
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- Algorithm exit! ret=%d", ret_val);
#endif
#endif
	
	return ret_val;
}

int ExpressCutout_set(int code_count, int code_symbology, int code_digit, int code_validity, int code_validityEx)
{
	int ret_val = 0;
	AlgorithmParamSet params;

#ifdef _WRITE_LOG
	// 生成日志序号
	GenerateLogIdCode();
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm params SETTING...");
#endif

	params.nCodeCount = code_count;
	params.nCodeSymbology = code_symbology;
	params.nCodeDgtNum = code_digit;
	params.nCodeValidity = code_validity;
	params.nCodeValidityExt = code_validityEx;

	ret_val = algorithm_setparams(&params);
	if(1 != ret_val) {
		goto nExit;
	}

	nIsExpressCutoutSet = 1;
	ret_val = 1;

nExit:
#ifdef _WRITE_LOG
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm params SETTING done. ret=%d", ret_val);
#endif
	return ret_val;
}

void ExpressCutout_reset()
{
#ifdef _WRITE_LOG
	// 生成日志序号
	GenerateLogIdCode();
	Write_Log(LOG_TYPE_SYSTEM, "Algorithm params RESET.");
#endif

	nIsExpressCutoutSet = 0;
	algorithm_resetparams();
	return;
}


int Add(int a, int b)
{
	return a + b + b;
}

void PretreatmentImage(char imageData[],int *imageWidth,int *imageHeight,char* code,int codeLength)
{
	int width = *imageWidth;
	int height = *imageHeight;
	//for(int i = 0; i < codeLength; i ++)
	//{
	//	code[i] = i + 1;
	//}

	for(int i=0; i<width; i++)
		for(int j=0; j<height; j++)
		{
			int gray = (int) imageData[j*width + i];
			imageData[j*width + i] = (unsigned char)((gray + 50)%256);
		}
	*imageWidth = width; //-10
	*imageHeight = height;
	if(codeLength == 12)
	{
		code[0] = '7';
	    code[1] = '0';
	    code[2] = '0';
		code[3] = '0';
		code[4] = '4';
		code[5] = '9';
		code[6] = '3';
		code[7] = '0';
		code[8] = '2';
		code[9] = '3';
		code[10] = '3';
		code[11] = '3';
		code[12] = '3';
	}

	//strcpy(code, "123456789012");
	//code[12] = NULL;

}
