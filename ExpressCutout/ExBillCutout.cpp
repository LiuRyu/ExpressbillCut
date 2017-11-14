#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "RyuCore.h"
#include "PerspectiveTrans.h"
#include "BarcodeImageproc.h"
#include "ExpressBarcodeDetection.h"
#include "ExBillCutout.h"

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
#include "OpenCv_debugTools.h"
#endif
#endif

const int nSetBillWidth = 384, nSetBillHeight = 320;
const int nSetCodeWidth = 64, nSetCodeOffsetX = 224, nSetCodeOffsetY = 128;

const int nGlobalGrayThreshold = 54;
const int nGradInterval = 16;

int gnInitScale = 0;
unsigned char * gRotateBillImage = 0;
unsigned char * gDilateBillImage = 0;
int * gHorizProj = 0, * gVertiProj = 0;

int gnExBillCutInitFlag = 0;


int ExBillCut_init(int input_width, int input_height)
{
	int ret_val = 0;
	//gnInitScale = (int)sqrt(1.0 * input_width * input_width + 1.0 * input_height * input_height);
	gnInitScale = nSetBillWidth + nSetBillHeight;

	gRotateBillImage = (unsigned char *)malloc(gnInitScale * gnInitScale * sizeof(unsigned char));
	if(!gRotateBillImage) {
		ret_val = -105020001;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d. gRotateBillImage=0x%x", ret_val, gRotateBillImage);
#endif
		goto nExit;
	}
	memset(gRotateBillImage, 0, gnInitScale * gnInitScale * sizeof(unsigned char));

	gDilateBillImage = (unsigned char *)malloc(gnInitScale * gnInitScale * sizeof(unsigned char));
	if(!gDilateBillImage) {
		ret_val = -105020002;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d. gDilateBillImage=0x%x", ret_val, gDilateBillImage);
#endif
		goto nExit;
	}
	memset(gDilateBillImage, 0, gnInitScale * gnInitScale * sizeof(unsigned char));

	gHorizProj = (int *)malloc(gnInitScale * sizeof(int));
	if(!gHorizProj) {
		ret_val = -105020003;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d. gHorizProj=0x%x", ret_val, gHorizProj);
#endif
		goto nExit;
	}
	memset(gHorizProj, 0, gnInitScale * sizeof(int));
	gVertiProj = (int *)malloc(gnInitScale * sizeof(int));
	if(!gVertiProj) {
		ret_val = -105020004;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d. gVertiProj=0x%x", ret_val, gVertiProj);
#endif
		goto nExit;
	}
	memset(gVertiProj, 0, gnInitScale * sizeof(int));

	gnExBillCutInitFlag = 1;
	ret_val = 1;
nExit:
	return ret_val;
}


void ExBillCut_release()
{
	gnExBillCutInitFlag = 0;
	if(gRotateBillImage) {
		free(gRotateBillImage);
		gRotateBillImage = 0;
	}
	if(gDilateBillImage) {
		free(gDilateBillImage);
		gDilateBillImage = 0;
	}
	if(gHorizProj) {
		free(gHorizProj);
		gHorizProj = 0;
	}
	if(gVertiProj) {
		free(gVertiProj);
		gVertiProj = 0;
	}
}


int ExBillCut_run(unsigned char * im, int width, int height, int * code_bounds,
		int code_orient, unsigned char * bill, int * bill_width, int * bill_height)
{
	int ret_val = 0;
	int i = 0, j = 0;

	RyuImage * iplRotate = 0, * iplDilate = 0;

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL_TIME
	long TimeCost = 0;
	LARGE_INTEGER m_frequency = {0}, m_time1 = {0}, m_time2 = {0};
	int support = QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_time2);
#endif
#endif

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 绘制图像
	IplImage * ipl_im3C = 0;
	IplImage * ipl_im1C = 0;
	IplImage * ipl_im = 0;

	IplImage * ipl_rim = 0;
	IplImage * ipl_dim = 0, * ipl_dim3C = 0;
#endif
#endif

	if(NULL == im || NULL == bill 
		|| NULL == bill_width || NULL == bill_height
		|| NULL == code_bounds) {
		ret_val = -105020101;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d.", ret_val);
#endif
		goto nExit;
	}

	if(0 >= width || 0 >= height) {
		ret_val = -105020102;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "初始化错误，错误号: %d.", ret_val);
#endif
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 绘制图像
	ipl_im3C = cvCreateImage(cvSize(width>>3, height>>3), 8, 3);
	ipl_im1C = cvCreateImage(cvGetSize(ipl_im3C), 8, 1);
	ipl_im = cvCreateImage(cvSize(width, height), 8, 1);

	uc2IplImageGray(im, ipl_im);
	cvResize(ipl_im, ipl_im1C);
	cvCvtColor(ipl_im1C, ipl_im3C, CV_GRAY2RGB);
	cvNamedWindow("ExBill");
	cvShowImage("ExBill", ipl_im3C);
	//cvWaitKey();
#endif
#endif

	// 计算条码四点坐标
	int codeX[4], codeY[4];
	for(i = 0; i < 4; i++) {
		codeX[i] = code_bounds[i] >> 16;
		if(codeX[i] & 0x8000) {
			codeX[i] = 0xffff0000 | codeX[i];
		}
		codeY[i] = code_bounds[i] & 0xffff;
		if(codeY[i] & 0x8000) {
			codeY[i] = 0xffff0000 | codeY[i];
		}
	}

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 绘制条码四点坐标
	for(int iid = 0; iid < 4; iid++)
		cvCircle(ipl_im3C, cvPoint(codeX[iid]>>3, codeY[iid]>>3), 
			2, CV_RGB(0, 255, 0), CV_FILLED);
	cvShowImage("ExBill", ipl_im3C);
	//cvWaitKey();
#endif
#endif

	// 计算条码宽度及条码中心
	int dx = abs(codeX[0] - codeX[1]);
	int dy = abs(codeY[0] - codeY[1]);
	int nTmp0 = (int)sqrt(1.0 * dx * dx + 1.0 * dy * dy);
	dx = abs(codeX[0] - codeX[2]);
	dy = abs(codeY[0] - codeY[2]);
	int nTmp1 = (int)sqrt(1.0 * dx * dx + 1.0 * dy * dy);
	int codeWidth = RYUMAX(300, RYUMAX(nTmp0, nTmp1));
	int codeCentX = (codeX[0] + codeX[3]) / 2;
	int codeCentY = (codeY[0] + codeY[3]) / 2;

	// 计算相对位置及粗切割四点坐标
	int alpha = 0;
	double rate = codeWidth / nSetCodeWidth;
	int rltvX[4], rltvY[4];
	int billX[4], billY[4];
	if(code_orient >= 180) {
		alpha = code_orient - 180;
		rltvX[0] = (int)(nSetCodeOffsetX * rate);
		rltvY[0] = (int)(nSetCodeOffsetY * rate);
		rltvX[1] = 0 - (int)((nSetBillWidth - nSetCodeOffsetX) * rate);
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = 0 - (int)((nSetBillHeight - nSetCodeOffsetY) * rate);
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	} else {
		alpha = code_orient;
		rltvX[0] = 0 - (int)(nSetCodeOffsetX * rate);
		rltvY[0] = 0 - (int)(nSetCodeOffsetY * rate);
		rltvX[1] = (int)((nSetBillWidth - nSetCodeOffsetX) * rate);
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = (int)((nSetBillHeight - nSetCodeOffsetY) * rate);
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	}
	for(i = 0; i < 4; i++) {
		CoorRelative2Absolute(codeCentX, codeCentY, rltvX[i], rltvY[i], (unsigned char)alpha, &billX[i], &billY[i]);
	}

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 绘制条码四点坐标
	cvLine(ipl_im3C, cvPoint(billX[0]>>3, billY[0]>>3), 
		cvPoint(billX[1]>>3, billY[1]>>3), CV_RGB(0, 255, 0));
	cvLine(ipl_im3C, cvPoint(billX[0]>>3, billY[0]>>3), 
		cvPoint(billX[2]>>3, billY[2]>>3), CV_RGB(0, 255, 0));
	cvLine(ipl_im3C, cvPoint(billX[3]>>3, billY[3]>>3), 
		cvPoint(billX[1]>>3, billY[1]>>3), CV_RGB(0, 255, 0));
	cvLine(ipl_im3C, cvPoint(billX[3]>>3, billY[3]>>3), 
		cvPoint(billX[2]>>3, billY[2]>>3), CV_RGB(0, 255, 0));
	cvShowImage("ExBill", ipl_im3C);
	//cvWaitKey();
#endif
#endif

	// 粗定位面单图像采样旋转
	int rotateWid = nSetBillWidth;
	int rotateHei = nSetBillHeight;
	int srcX[8] = {0}, dstX[8] = {0};
	double matrix[9];
	srcX[0] = 0;			srcX[1] = 0;	srcX[2] = 0;			srcX[3] = rotateHei-1;
	srcX[4] = rotateWid-1;	srcX[5] = 0;	srcX[6] = rotateWid-1;	srcX[7] = rotateHei-1;
	dstX[0] = billX[0];	dstX[1] = billY[0];	dstX[2] = billX[2];	dstX[3] = billY[2];
	dstX[4] = billX[1];	dstX[5] = billY[1];	dstX[6] = billX[3];	dstX[7] = billY[3];

	// 运算次数较少,虽为浮点,却基本不耗时
	IPA_getPerspectiveTransformMat(srcX, dstX, matrix);
	/*ret_val = IPA_warpPerspectiveTransformFixed(imageData, nInitImgWid, nInitImgHei,
		gRotateImage, rotateWid, rotateHei, matrix);*/
	//解决不同大小图片需要切割的问题 2016.7.27
	unsigned char * pRotateImage = gRotateBillImage;
	IPA_warpPerspectiveTransformFixed(im, width, height,
		pRotateImage, rotateWid, rotateHei, matrix);

	unsigned char * pDilateImage = gDilateBillImage;
	iplRotate = ryuCreateImageHeader(ryuSize(rotateWid, rotateHei), 8, 1);
	iplRotate->imagedata = pRotateImage;
	iplDilate = ryuCreateImageHeader(ryuSize(rotateWid, rotateHei), 8, 1);
	iplDilate->imagedata = pDilateImage;
	ryuDilate(iplRotate, iplDilate);

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
// 	ipl_rim = cvCreateImage(cvSize(rotateWid, rotateHei), 8, 1);
// 	uc2IplImageGray(pRotateImage, ipl_rim);
// 	cvNamedWindow("ExBill-1");
// 	cvShowImage("ExBill-1", ipl_rim);
	ipl_dim = cvCreateImage(cvSize(rotateWid, rotateHei), 8, 1);
	uc2IplImageGray(pDilateImage, ipl_dim);
// 	cvNamedWindow("ExBill-2");
// 	cvShowImage("ExBill-2", ipl_dim);
// 	cvWaitKey();
#endif
#endif

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	int dbgMaxH = 0, dbgMaxV = 0;
#endif
#endif

	// 获取水平及垂直投影
	int * pHP = gHorizProj;
	int * pVP = gVertiProj;
	memset(pHP, 0, rotateHei * sizeof(int));
	memset(pVP, 0, rotateWid * sizeof(int));
	unsigned char * pDI = pDilateImage;

	for(j = 0; j < rotateHei; j++) {
		for(i = 0; i < rotateWid; i++) {
			pHP[j] += (pDI[i] > nGlobalGrayThreshold) ? pDI[i] : 0;
		}
		pDI += rotateWid;
#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
		dbgMaxH = RYUMAX(dbgMaxH, pHP[j]);
#endif
#endif
	}

	unsigned char * pD = 0;
	pDI = pDilateImage;
	for(j = 0; j < rotateWid; j++) {
		pD = pDI;
		for(i = 0; i < rotateHei; i++) {
			pVP[j] += (*pD > nGlobalGrayThreshold) ? *pD : 0;
			pD += rotateWid;
		}
		pDI += 1;
#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
		dbgMaxV = RYUMAX(dbgMaxV, pVP[j]);
#endif
#endif
	}

	// 确定边界，隔行采样梯度下降
	int projBound[4] = {0, rotateWid-1, 0, rotateHei-1};
	int projThre = 0;
	// 确定左界
	for(i = nSetCodeOffsetX; i >= nGradInterval; i-=nGradInterval/2) {
		if(pVP[i] - pVP[i-nGradInterval] > (pVP[i] >> 1)) {
			projBound[0] = i - nGradInterval;
			break;
		}
	}
	// 确定右界
	for(i = nSetCodeOffsetX; i < nSetBillWidth - nGradInterval; i+=nGradInterval/2) {
		if(pVP[i] - pVP[i+nGradInterval] > (pVP[i] >> 1)) {
			projBound[1] = i + nGradInterval;
			break;
		}
	}
	// 确定上界
	for(i = nSetCodeOffsetY; i >= nGradInterval; i-=nGradInterval/2) {
		if(pHP[i] - pHP[i-nGradInterval] > (pHP[i] >> 1)) {
			projBound[2] = i - nGradInterval;
			break;
		}
	}
	// 确定下界
	for(i = nSetCodeOffsetY; i < nSetBillHeight - nGradInterval; i+=nGradInterval/2) {
		if(pHP[i] - pHP[i+nGradInterval] > (pHP[i] >> 1)) {
			projBound[3] = i + nGradInterval;
			break;
		}
	}

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	dbgMaxV++;
	dbgMaxH++;
	ipl_dim3C = cvCreateImage(cvGetSize(ipl_dim), 8, 3);
	cvCvtColor(ipl_dim, ipl_dim3C, CV_GRAY2BGR);
	for(int iid = 0; iid < rotateWid-1; iid++) {
		cvLine(ipl_dim3C, cvPoint(iid, rotateHei-rotateHei*pVP[iid]/dbgMaxV/2), 
			cvPoint(iid+1, rotateHei-rotateHei*pVP[iid+1]/dbgMaxV/2),
			CV_RGB(0, 255, 0));
	}
	for(int iid = 0; iid < rotateHei-1; iid++) {
		cvLine(ipl_dim3C, cvPoint(rotateWid-rotateWid*pHP[iid]/dbgMaxH/2, iid),
			cvPoint(rotateWid-rotateWid*pHP[iid+1]/dbgMaxH/2, iid+1),
			CV_RGB(255, 0, 0));
	}

	cvLine(ipl_dim3C, cvPoint(projBound[0], 0), cvPoint(projBound[0], rotateHei),
		CV_RGB(0, 128, 255));
	cvLine(ipl_dim3C, cvPoint(projBound[1], 0), cvPoint(projBound[1], rotateHei),
		CV_RGB(0, 128, 255));
	cvLine(ipl_dim3C, cvPoint(0, projBound[2]), cvPoint(rotateWid, projBound[2]),
		CV_RGB(0, 128, 255));
	cvLine(ipl_dim3C, cvPoint(0, projBound[3]), cvPoint(rotateWid, projBound[3]),
		CV_RGB(0, 128, 255));

	cvNamedWindow("ExBill-3");
	cvShowImage("ExBill-3", ipl_dim3C);
	//cvWaitKey();
#endif
#endif

	// 还原坐标
	if(code_orient >= 180) {
		alpha = code_orient - 180;
		rltvX[0] = (int)((nSetCodeOffsetX-projBound[0]) * rate);
		rltvY[0] = (int)((nSetCodeOffsetY-projBound[2]) * rate);
		rltvX[1] = 0 - (int)((projBound[1] - nSetCodeOffsetX) * rate);
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = 0 - (int)((projBound[3] - nSetCodeOffsetY) * rate);
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	} else {
		alpha = code_orient;
		rltvX[0] = 0 - (int)((nSetCodeOffsetX-projBound[0]) * rate);
		rltvY[0] = 0 - (int)((nSetCodeOffsetY-projBound[2]) * rate);
		rltvX[1] = (int)((projBound[1] - nSetCodeOffsetX) * rate);
		rltvY[1] = rltvY[0];
		rltvX[2] = rltvX[0];
		rltvY[2] = (int)((projBound[3] - nSetCodeOffsetY) * rate);
		rltvX[3] = rltvX[1];
		rltvY[3] = rltvY[2];
	}
	for(i = 0; i < 4; i++) {
		CoorRelative2Absolute(codeCentX, codeCentY, rltvX[i], rltvY[i], (unsigned char)alpha, &billX[i], &billY[i]);
	}

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 绘制条码四点坐标
	cvLine(ipl_im3C, cvPoint(billX[0]>>3, billY[0]>>3), 
		cvPoint(billX[1]>>3, billY[1]>>3), CV_RGB(0, 128, 255));
	cvLine(ipl_im3C, cvPoint(billX[0]>>3, billY[0]>>3), 
		cvPoint(billX[2]>>3, billY[2]>>3), CV_RGB(0, 128, 255));
	cvLine(ipl_im3C, cvPoint(billX[3]>>3, billY[3]>>3), 
		cvPoint(billX[1]>>3, billY[1]>>3), CV_RGB(0, 128, 255));
	cvLine(ipl_im3C, cvPoint(billX[3]>>3, billY[3]>>3), 
		cvPoint(billX[2]>>3, billY[2]>>3), CV_RGB(0, 128, 255));
	cvShowImage("ExBill", ipl_im3C);
	//cvWaitKey();
#endif
#endif

	// 旋转精确切割面单图像，并存入输出图像内存区
	dx = abs(rltvX[0] - rltvX[1]);
	dy = abs(rltvY[0] - rltvY[1]);
	rotateWid = (int)sqrt(1.0 * dx * dx + 1.0 * dy * dy);
	dx = abs(rltvX[0] - rltvX[2]);
	dy = abs(rltvY[0] - rltvY[2]);
	rotateHei = (int)sqrt(1.0 * dx * dx + 1.0 * dy * dy);

	if(rotateWid > width) {
		rate = 1.0 * width / rotateWid;
		rotateWid = width;
		rotateHei = (int)(rotateHei * rate);
	}
	if(rotateHei > height) {
		rate = 1.0 * height / rotateHei;
		rotateHei = height;
		rotateWid = (int)(rotateWid * rate);
	}

	srcX[0] = 0;			srcX[1] = 0;	srcX[2] = 0;			srcX[3] = rotateHei-1;
	srcX[4] = rotateWid-1;	srcX[5] = 0;	srcX[6] = rotateWid-1;	srcX[7] = rotateHei-1;
	dstX[0] = billX[0];	dstX[1] = billY[0];	dstX[2] = billX[2];	dstX[3] = billY[2];
	dstX[4] = billX[1];	dstX[5] = billY[1];	dstX[6] = billX[3];	dstX[7] = billY[3];

		// 运算次数较少,虽为浮点,却基本不耗时
	IPA_getPerspectiveTransformMat(srcX, dstX, matrix);
	IPA_warpPerspectiveTransformFixed(im, width, height,
		bill, rotateWid, rotateHei, matrix);

	*bill_width = rotateWid;
	*bill_height = rotateHei;

	ret_val = 1;

nExit:
	if(iplRotate)
		ryuReleaseImageHeader(&iplRotate);
	if(iplDilate)
		ryuReleaseImageHeader(&iplDilate);

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL_TIME
	QueryPerformanceCounter((LARGE_INTEGER*) &m_time1);
	TimeCost = (long)(1000.0*1000.0*(m_time1.QuadPart-m_time2.QuadPart)/m_frequency.QuadPart);
	printf("ExBillCut_run Timecost = %ld us\n", TimeCost);
#endif
#endif

#ifdef _DEBUG_
#ifdef _DEBUG_EXBILL
	// 内存释放
	if(ipl_im3C)
		cvReleaseImage(&ipl_im3C);
	if(ipl_im1C)
		cvReleaseImage(&ipl_im1C);
	if(ipl_im)
		cvReleaseImage(&ipl_im);
	if(ipl_rim)
		cvReleaseImage(&ipl_rim);
	if(ipl_dim)
		cvReleaseImage(&ipl_dim);
	if(ipl_dim3C)
		cvReleaseImage(&ipl_dim3C);
#endif
#endif

	return ret_val;
}