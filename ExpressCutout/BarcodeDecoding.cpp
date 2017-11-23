#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "RyuCore.h"

#include "Decoder_I2of5.h"
#include "Decoder_EAN13.h"
#include "BarcodeDecoding.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODE
#include "OpenCv_debugTools.h"
#endif
#endif


int gnDcdMaxWidth = 0, gnDcdMaxHeight = 0;
int gnDcdMaxLineSize = 0;

int * gpnDcdColumnscanArr = 0;
int * gpnDcdPartitionArr = 0;
int * gpnDcdDecodeArr = 0;
int * gpnDcdDecodeArrProc = 0;

RyuPoint * gptDcdStartstop = 0;

DecodeDemarcateNode * gpDDNOrig_arr = 0;
DecodeDemarcateNode * gpDDNEffc_arr = 0;

float * gpfDcdCoor_basic = 0;
float * gpfDcdDecodeArr_basic = 0;

float * gpfDcdCoor_strict = 0;
float * gpfDcdDecodeArr_strict = 0;

int * gpnDcdDecodeArr2 = 0;

unsigned char * gpDcdDecoderMemSets = 0;
const int gnDcdDecoderMemSize = 12800;

DecodeResultNode * gpDRNCandArr = 0;
const int gnDRNCandMaxCnt = 32;
int gnDRNCandCnt = 0;

int gnDcdInitFlag = 0;

void BarcodeColumnScan( unsigned char * bina, int width, int height, int * column );

void BarcodeBlackWhitePartition( int * column, int width, int height, int * partition );

int FindPartitionStartStop( int * partition, int width, int height, RyuPoint * startstop );

void DecodeArrayAnalysis( int * column, int width, int height, RyuPoint * startstop, int count, int * decode_arr);

int BarcodeDecoding_DemarcateAnalysis( unsigned char * im, int * integr, int width, int height, 
									  int slice_height, int slice_top, int slice_bottom,
									  int * decode_bcnt, int * decode_scnt, float * min_module);

// 结果累计功能
void resetDecodeResultCandidates();

int pushDecodeResult2Candidates(DecodeResultNode result);

int acquireDecodeResults(DecodeResultNode * result);

int BarcodeResultCheck(DecodeResultNode result, int setDigit, int setValid);


// v2.5前版本解码算法
int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH, 
		char * code_result, int * code_type, int * char_num, int * module_num, 
		int * code_direct, int * leftOffset, int * rightOffset)
{
	int nRet = 0, i = 0, status = 0;
	int nIsCodeState = 0;

	int nColCount = 0;

	int slice_cnt = 0, slice_remain = 0, slice_offset = 0;
	int slice_top = 0, slice_bottom = 0, slice_height = 0;

	unsigned char * pBina = 0;

	char strResult[128];
	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nLeftIdx = 0, nRightIdx = 0, nCodeType = 0;
	int nCodeWidth = 0, nLeftOffset = 0, nRightOffset = 0;
	float fModuleWid = 0.0F;

	if( 1 != gnDcdInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! DecodeBarcode run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !bina ) {
#ifdef	_PRINT_PROMPT
		printf( "ERROR! Invalid input of DecodeBarcode, bina=0x%x\n", bina );
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnDcdMaxWidth || height > gnDcdMaxHeight ) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of DecodeBarcode, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

	if( sliceH <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of DecodeBarcode, sliceH=%d\n", sliceH);
#endif
		nRet = -1;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	IplImage * iplBinaImg = 0;
	IplImage * iplBinaImg3C = 0;
#endif
#endif

	sliceH = sliceH & (~0x1);	// 必须是偶数
	slice_offset = sliceH >> 1;
	slice_cnt = (height < sliceH) ? 0 : (height / slice_offset - 1);
	slice_remain = height % slice_offset;
	slice_cnt = (slice_remain) ? (slice_cnt + 1) : slice_cnt;

	// 分块识别
	for( i = 0; i < slice_cnt; i++ ) {
		if( i < slice_cnt - 1 ) {
			slice_top = i * slice_offset;
			slice_bottom = slice_top + sliceH - 1;
			slice_height = sliceH;
		} else {
			slice_bottom = height - 1;
			slice_top = height - sliceH;
			slice_top = RYUMAX( 0, slice_top );
			slice_height = slice_bottom - slice_top + 1;
		}

		if(slice_bottom >= height) {
#ifdef	_PRINT_PROMPT
			printf("Warning! Unexpected value of slice_bottom, loop=%d, slice_bottom=%d\n",
				i, slice_bottom);
#endif
			break;
		}

		pBina = bina + slice_top * width;
		// 统计列情况
		BarcodeColumnScan( pBina, width, slice_height, gpnDcdColumnscanArr );
		// 黑白分界
		BarcodeBlackWhitePartition( gpnDcdColumnscanArr, width, slice_height, gpnDcdPartitionArr );

		nColCount = FindPartitionStartStop( gpnDcdPartitionArr, width, slice_height, gptDcdStartstop );

		DecodeArrayAnalysis( gpnDcdColumnscanArr, width, slice_height, gptDcdStartstop, nColCount, gpnDcdDecodeArr);

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
#if		1
		printf("--------Decode Array--------\n");
		for(int x = 0; x < nColCount; x++) {
			printf("%6d", gpnDcdDecodeArr[x]);
			if(!((x+1) % 12))
				printf("\n");
		}
		printf("\n");
#endif
		iplBinaImg = cvCreateImage( cvSize(width, height+sliceH*3+8+1), 8, 1 );
		iplBinaImg3C = cvCreateImage( cvGetSize(iplBinaImg), 8, 3 );
		int offsetY = 0;
		// 绘制图像
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int gloom = bina[y*width+x];
				if(y < slice_top || y > slice_bottom)
					gloom = (0 == gloom) ? (gloom+80) : (gloom-80);
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*y))[x] = gloom;
			}
		}
		offsetY += height;
		// 绘制分割区
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
			}
		}
		offsetY += 8;
		// 绘制目标分块
		for (int y = 0; y < slice_height; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = bina[(y+slice_top)*width+x];
			}
		}
		offsetY += slice_height;
		// 绘制分界结果
		cvCvtColor(iplBinaImg, iplBinaImg3C, CV_GRAY2BGR);
		for(int x = 0; x < width; x++) {
			if(1 == gpnDcdPartitionArr[x])
				cvLine(iplBinaImg3C, cvPoint(x, offsetY), cvPoint(x, offsetY+slice_height-1), 
					CV_RGB(0,255,0));
			else if(-1 == gpnDcdPartitionArr[x])
				cvLine(iplBinaImg3C, cvPoint(x, offsetY), cvPoint(x, offsetY+slice_height-1), 
					CV_RGB(255,0,0));
			else
				cvLine(iplBinaImg3C, cvPoint(x, offsetY), cvPoint(x, offsetY+slice_height-1), 
					CV_RGB(255,255,0));
		}
		offsetY += slice_height;
		// 绘制起止点分界结果
		for (int y = 0; y < slice_height+1; y++) {
			for (int x = 0; x < iplBinaImg3C->widthStep; x++) {
				((unsigned char *)(iplBinaImg3C->imageData + iplBinaImg3C->widthStep*(y+offsetY)))[x] = 0;
			}
		}
		offsetY += 1;
		int color_diff1 = 0, color_diff2 = 0;
		for(int x = 0; x < nColCount; x++) {
			if(gptDcdStartstop[x].y > 0) {
				color_diff1 = (0 == color_diff1) ? 255 : 0;
				for(int y = gptDcdStartstop[x].x; y <= gptDcdStartstop[x].y; y++) {
					cvLine(iplBinaImg3C, cvPoint(y, offsetY), cvPoint(y, offsetY+slice_height/2-1), 
						CV_RGB(0,255-color_diff1/2,color_diff1));
				}
			} else if(gptDcdStartstop[x].y < 0) {
				color_diff2 = (0 == color_diff1) ? 255 : 0;
				for(int y = 0-gptDcdStartstop[x].x; y <= 0-gptDcdStartstop[x].y; y++) {
					cvLine(iplBinaImg3C, cvPoint(y, offsetY+slice_height/2), cvPoint(y, offsetY+slice_height-1), 
						CV_RGB(255-color_diff2/2,0,color_diff2));
				}
			}
		}
		offsetY += slice_height;
#endif
#endif
		status = 0;
		while(1) {	// 此while循环是为了跳出时保留当前参数值
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode128(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 0x1;
				break;
			} 

			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode39(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<1;
				break;
			} 

			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode93(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<2;
				break;
			}

			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCodeI2of5(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<3;
				break;
			}

			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCodeEAN13(gpnDcdDecodeArr, gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<4;
				break;
			}

			break;
		}

		if( 1 == status ) {
#if    1
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
			if( 0x1 == nCodeType )
				printf("-找到code128条码：%s\n", strResult);
			else if( 0x2 == nCodeType)
				printf("-找到code39条码：%s\n", strResult);
			else if( 0x4 == nCodeType)
				printf("-找到code93条码：%s\n", strResult);
			else if( 0x8 == nCodeType)
				printf("-找到I2of5条码：%s\n", strResult);
			else if( 0x10 == nCodeType)
				printf("-找到EAN-13条码：%s\n", strResult);
			printf("-digit:%d, module%d, direction:%d, leftIdx:%d, rightIdx:%d\n", \
				nDigitCnt, nModuleCnt, nDirection, nLeftIdx, nRightIdx);
#endif
#endif
#endif
			//////////////////////////////////////////////////////////////////////////
			// 2.0.6算法改进
			// 这里加入条码字符检验函数，过滤含有无效字符的条码
// 			if( 1 == BarcodeValidityCheck(strResult, nDigitCnt) )
// 				break;
// 			else
// 				status = 0;
			break;
		}
#if    1
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		else {
			printf("-没有找到条码, return=%d\n", status);
		}
		cvNamedWindow("Decode");
		cvShowImage("Decode", iplBinaImg3C);
		cvWaitKey();
// 		cvReleaseImage(&iplBinaImg);
// 		cvReleaseImage(&iplBinaImg3C);
#endif
#endif
#endif
	}


	if( status > 0 ) {
		nLeftOffset = abs(gptDcdStartstop[nLeftIdx].x);
		nRightOffset = abs(gptDcdStartstop[nRightIdx].y);
		nCodeWidth = nRightOffset - nLeftOffset;
		fModuleWid = nCodeWidth * 1.0 / (nModuleCnt+1);
		nRightOffset = width - abs(gptDcdStartstop[nRightIdx].y) - 1;
		// 结果写入
		memcpy(code_result, strResult, sizeof(char) * CODE_RESULT_ARR_LENGTH);
		*code_type = nCodeType;
		*char_num = nDigitCnt;
		*module_num = nModuleCnt;
		*code_direct = nDirection;
		*leftOffset = nLeftOffset;
		*rightOffset = nRightOffset;
		nRet = 1;
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		if( 0x1 == nCodeType )
			printf("-找到code128条码：%s\n", strResult);
		else if( 0x2 == nCodeType)
			printf("-找到code39条码：%s\n", strResult);
		else if( 0x4 == nCodeType)
			printf("-找到code93条码：%s\n", strResult);
		else if( 0x8 == nCodeType)
			printf("-找到I2of5条码：%s\n", strResult);
		else if( 0x10 == nCodeType)
			printf("-找到EAN-13条码：%s\n", strResult);
		printf("-digit:%d, module:%d, direction:%d, codeWid:%d, moduleWid:%.2f, leftOffset:%d, rightOffset:%d\n", \
			nDigitCnt, nModuleCnt, nDirection, nCodeWidth, fModuleWid, nLeftOffset, nRightOffset);
#endif
#endif
	} else {
		nRet = 0;
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		printf("-没有找到条码, return=%d, nIsCodeState=%d\n", status, nIsCodeState);
#endif
#endif
	}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		cvNamedWindow("Decode");
		cvShowImage("Decode", iplBinaImg3C);
		cvWaitKey();
		cvReleaseImage(&iplBinaImg);
		cvReleaseImage(&iplBinaImg3C);
#endif
#endif

nExit:
	return nRet;
}


// v2.6版本开始使用的解码算法
int BarcodeDecoding_run(unsigned char * im, int * integr, int width, int height,  
						char * code_result, int * code_type, int * char_num, int * char_valid, int * module_num, 
						int * code_direct, int * leftOffset, int * rightOffset, float * minModule)
{
	int ret_val = 0, status = 0;
	int i = 0, j = 0, k = 0;

	int sliceHs[32] = {0};
	int sliceHs_cnt = 0, slice_height = 0;
	int slice_cnt = 0, slice_top = 0, slice_bottom = 0;

	float * fDemarcCoord_basic = gpfDcdCoor_basic;
	float * fDecodeElements_basic = gpfDcdDecodeArr_basic;
	int nColCnt_basic = 0;

	float * fDemarcCoord_strict = gpfDcdCoor_strict;
	float * fDecodeElements_strict = gpfDcdDecodeArr_strict;
	int nColCnt_strict = 0;

	int * nDecodeArr_basic = gpnDcdDecodeArr;
	int * nDecodeArr_strict = gpnDcdDecodeArr2;
	int * nDecodeArrProc = gpnDcdDecodeArrProc;

	int setDigit = 0, setType = 0, setValid = 0;

	char strResult[CODE_RESULT_ARR_LENGTH] = {0};
	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nLeftIdx = 0, nRightIdx = 0, nCodeType = 0;
	int nLeftOffset = 0, nRightOffset = 0;

	DecodeResultNode tmpResult;
	float fModuleWid = 0.0F;

	int nColCount = 0;
	int * pDecodeArr = 0;
	float * pDemarcCoord = 0;

	float fMinModule = 0.0, fAccModule = 0.0;
	int cntAccModule = 0;

	if( 1 != gnDcdInitFlag ) {
		ret_val = -1;
		goto nExit;
	}

	if( NULL == im || NULL == integr) {
		ret_val = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnDcdMaxWidth || height > gnDcdMaxHeight ) {
			ret_val = -1;
			goto nExit;
	}

	setType = *code_type;
	setDigit = *char_num;
	setValid = *char_valid;

	resetDecodeResultCandidates();

	// 计算划分高度
	sliceHs_cnt = 0;
	slice_height = height;
	while(slice_height >= 24) {
		sliceHs[sliceHs_cnt++] = slice_height;
		slice_height = height / (sliceHs_cnt + 1);
	}
	if(16 <= height)
		sliceHs[sliceHs_cnt++] = 16;
	if(12 <= height)
		sliceHs[sliceHs_cnt++] = 12;
	if(8 <= height)
		sliceHs[sliceHs_cnt++] = 8;
	if(4 <= height)
		sliceHs[sliceHs_cnt++] = 4;
	else {
		sliceHs[0] = height;
		sliceHs_cnt = 1;
	}

	// 宽度递减进行识别
	for(j = 0; j < sliceHs_cnt; j++) {
		slice_height = sliceHs[j];
		slice_cnt = height / (slice_height >> 1) - 1;
		slice_top = 0;
		slice_bottom = slice_height - 1;
		for(i = 0; i < slice_cnt; i++) {
			BarcodeDecoding_DemarcateAnalysis(im, integr, width, height, slice_height, slice_top, slice_bottom,
				&nColCnt_basic, &nColCnt_strict, &fMinModule);
			fAccModule = fAccModule + fMinModule;
			cntAccModule++;

			// 分别使用basic和strict策略生成的宽度数组进行解码
			for(k = 0; k < 2; k++) {
				if(0 == k && nColCnt_basic > 0) {
					pDecodeArr = nDecodeArr_basic;
					nColCount = nColCnt_basic;
					pDemarcCoord = fDemarcCoord_basic;
				} else if(1 == k && nColCnt_strict > 0) {
					pDecodeArr = nDecodeArr_strict;
					nColCount = nColCnt_strict;
					pDemarcCoord = fDemarcCoord_strict;
				} else {
					continue;
				}

				if((setType & 0x1) || 0 == setType) {
					status = 0;
					memcpy( gpnDcdDecodeArrProc, pDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
					status = Decoder_code128(gpnDcdDecodeArrProc, nColCount, &tmpResult);
					if(1 == BarcodeResultCheck(tmpResult, setDigit, setValid)) {
						tmpResult.left_idx = (int)fabs(pDemarcCoord[tmpResult.left_idx]);
						tmpResult.right_idx = (int)fabs(pDemarcCoord[tmpResult.right_idx+1]);
						pushDecodeResult2Candidates(tmpResult);
					} 
				}

				if((setType & 0x2) || 0 == setType) {
					status = 0;
					memcpy( gpnDcdDecodeArrProc, pDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
					status = Decoder_code39(gpnDcdDecodeArrProc, nColCount, &tmpResult);
					if(1 == BarcodeResultCheck(tmpResult, setDigit, setValid)) {
						tmpResult.left_idx = (int)fabs(pDemarcCoord[tmpResult.left_idx]);
						tmpResult.right_idx = (int)fabs(pDemarcCoord[tmpResult.right_idx+1]);
						pushDecodeResult2Candidates(tmpResult);
					} 
				}

				if((setType & 0x4) || 0 == setType) {
					status = 0;
					memcpy( gpnDcdDecodeArrProc, pDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
					status = Decoder_code93(gpnDcdDecodeArrProc, nColCount, &tmpResult);
					if(1 == BarcodeResultCheck(tmpResult, setDigit, setValid)) {
						tmpResult.left_idx = (int)fabs(pDemarcCoord[tmpResult.left_idx]);
						tmpResult.right_idx = (int)fabs(pDemarcCoord[tmpResult.right_idx+1]);
						pushDecodeResult2Candidates(tmpResult);
					}
				}

				/*
				if((setType & 0x8) || 0 == setType) {
					status = 0;
					memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
					memcpy( gpnDcdDecodeArrProc, pDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
					status = RecgCodeI2of5(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
						&nDirection, &nLeftIdx, &nRightIdx);
					if( 1 == status ) {
						nCodeType = 0x8;
						break;
					}
				}

				if((setType & 0x10) || 0 == setType) {
					status = 0;
					memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
					memcpy( gpnDcdDecodeArrProc, pDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
					status = RecgCodeEAN13(gpnDcdDecodeArr, gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
						&nDirection, &nLeftIdx, &nRightIdx);
					if( 1 == status ) {
						nCodeType = 0x10;
						break;
					}
				}
				*/
			}
			slice_top += (slice_height >> 1);
			slice_bottom = slice_top + slice_height - 1;
		}
/*
		if( status > 0 ) {
			nRightOffset = width - nRightOffset - 1;
			// 结果写入
			memcpy(code_result, strResult, sizeof(char) * CODE_RESULT_ARR_LENGTH);
			*code_type = nCodeType;
			*char_num = nDigitCnt;
			*module_num = nModuleCnt;
			*code_direct = nDirection;
			*leftOffset = nLeftOffset;
			*rightOffset = nRightOffset;
			ret_val = 1;
			break;

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
			printf("-BarcodeDecoding_run找到条码, slice_height=%d, ", slice_height);
			if( 0x1 == nCodeType )
				printf("-找到code128条码：%s\n", strResult);
			else if( 0x2 == nCodeType)
				printf("-找到code39条码：%s\n", strResult);
			else if( 0x4 == nCodeType)
				printf("-找到code93条码：%s\n", strResult);
			else if( 0x8 == nCodeType)
				printf("-找到I2of5条码：%s\n", strResult);
			else if( 0x10 == nCodeType)
				printf("-找到EAN-13条码：%s\n", strResult);
			printf("-digit:%d, module:%d, direction:%d, codeWid:%d, moduleWid:%.2f, leftOffset:%d, rightOffset:%d\n", \
				nDigitCnt, nModuleCnt, nDirection, nCodeWidth, fModuleWid, nLeftOffset, nRightOffset);
#endif
#endif
		} else {
			ret_val = 0;
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
			printf("-BarcodeDecoding_run没有找到条码, slice_height=%d, ", slice_height);
			printf("min_module=%2.3f\n", *minModule);
#endif
#endif
		}
		*/
	}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	printf("条码高度 = %d, 识别总次数 = %d\n", height, sliceHs_cnt * slice_cnt);
#endif
#endif
	status = acquireDecodeResults(&tmpResult);
	if(1 == status) {
		// 结果写入
		memcpy(code_result, tmpResult.strCodeData, sizeof(char) * CODE_RESULT_ARR_LENGTH);
		*code_type = tmpResult.code_type;
		*char_num = tmpResult.rslt_digit;
		*module_num = tmpResult.code_module;
		*code_direct = tmpResult.code_direct;
		*leftOffset = tmpResult.left_idx;
		*rightOffset = width - tmpResult.right_idx - 1;;
		ret_val = 1;
	} else {
		ret_val = 0;
	}

	if(0 < cntAccModule) {
		*minModule = fAccModule / cntAccModule;
	} else {
		*minModule = 2.0;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	cvWaitKey();
#endif _DEBUG_DECODE
#endif _DEBUG_

nExit:
	return ret_val;
}


int BarcodeDecoding_DemarcateAnalysis( unsigned char * im, int * integr, int width, int height, 
									  int slice_height, int slice_top, int slice_bottom,
									  int * decode_bcnt, int * decode_scnt, float * min_module) 
{
	int ret_val = 0;
	int i = 0, j = 0, k = 0, status = 0;

	int * pColumnArr = gpnDcdColumnscanArr;
	int * pGradArr = gpnDcdPartitionArr;

	int maxGray = 0, minGray = 255;

	int * pIntegTop = 0, * pIntegBtm = 0;
	int cnt = 0;
	int type = 0, isActive = 0;
	int val = 0, maxv = 0, minv = 0;

	const int gradThreshold = 30;
	const float fLumLrningRateFast = 0.15;
	const float fLumLrningRateSlow = 0.08;
	int lumL = 0, lumH = 0, diff = 0;

	float fTune = 0, fMin = (float)width, fVal = 0.0, fCnt = 0.0, fTmp = 0.0;

	DecodeDemarcateNode tmpDemarc;
	DecodeDemarcateNode * demarc_arr = gpDDNOrig_arr;
	int demarc_cnt = 0;

	DecodeDemarcateNode * demarc_effctv = gpDDNEffc_arr;
	int demarc_effcnt = 0;

	float * fDemarcCoord_basic = gpfDcdCoor_basic;
	float * fDecodeElements_basic = gpfDcdDecodeArr_basic;
	int decodeCnt_basic = 0;

	float * fDemarcCoord_strict = gpfDcdCoor_strict;
	float * fDecodeElements_strict = gpfDcdDecodeArr_strict;
	int decodeCnt_strict = 0;

	int * nDecodeArr_basic = gpnDcdDecodeArr;
	int * nDecodeArr_strict = gpnDcdDecodeArr2;
	int * nDecodeArrProc = gpnDcdDecodeArrProc;

	// 获取该slice的列灰度平均数组
	pIntegTop = integr + (slice_top-1) * width;
	pIntegBtm = integr + slice_bottom * width;
	for(j = 0; j < width; j++) {
		pColumnArr[j] = pIntegBtm[j] - (0 < slice_top ? pIntegTop[j] : 0);
		pColumnArr[j] /= slice_height;
		pColumnArr[j] = 255 - pColumnArr[j];

		minGray = pColumnArr[j] < minGray ? pColumnArr[j] : minGray;
		maxGray = pColumnArr[j] > maxGray ? pColumnArr[j] : maxGray;
	}

	// 求取曲线梯度
	pGradArr[0] = 0;
	for(j = 1; j < width; j++) {
		pGradArr[j] = pColumnArr[j] - pColumnArr[j-1];
	}

	// 分析曲线并写入分界结构体数组
	demarc_cnt = 0;
	tmpDemarc.type = tmpDemarc.idx_b = tmpDemarc.idx_e = tmpDemarc.acc = 0;
	tmpDemarc.max_v = tmpDemarc.count = 0;
	tmpDemarc.gravity = 0;
	isActive = 1;
	maxv = 0;
	for(j = 1; j < width; j++) {
		// 获取当前点状态
		type = (0 == pGradArr[j]) ? 0 : (0 < pGradArr[j] ? 1 : -1);
		val = abs(pGradArr[j]);
		// 当前点状态与当前检索区间状态对比
		if(type == tmpDemarc.type) {
			// 若为连续0区间，则跳过值验证
			if(0 == type)
				continue;
			// 若区间检索为活跃状态，则验证当前值是否满足失效条件，记录最值
			else if(isActive) {
				// 若满足失效条件，则记录最小值
				if(maxv>>1 >= val) {
					isActive = 0;
					minv = val;
				} else {
					// 若为连续区间，则记录最大值
					maxv = val > maxv ? val : maxv;
					tmpDemarc.max_v = maxv;
				}
				tmpDemarc.acc += val;
				tmpDemarc.gravity += val * j;

				// 若区间检索为失效状态，则验证当前值是否满足结束条件，记录最值
			} else {
				// 满足结束条件，区间检索结束
				if(val > minv) {
					tmpDemarc.idx_e = j - 2;
					if(tmpDemarc.acc > 0)
						tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
					else 
						tmpDemarc.gravity = 0;

					demarc_arr[demarc_cnt++] = tmpDemarc;

					// 最谷值处添加一个0区
					demarc_arr[demarc_cnt].type = 0;
					demarc_arr[demarc_cnt].idx_b = demarc_arr[demarc_cnt].idx_e = j - 1;
					tmpDemarc.max_v = 0;
					demarc_arr[demarc_cnt].acc = 0;
					demarc_arr[demarc_cnt].gravity = 0;
					demarc_cnt++;

					// 新区间初始化
					tmpDemarc.type = type;
					tmpDemarc.idx_b = j;
					tmpDemarc.max_v = val;
					tmpDemarc.acc = val;
					tmpDemarc.gravity = val * j;
					isActive = 1;
					maxv = val;
				} else {
					minv = val;
					tmpDemarc.acc += val;
					tmpDemarc.gravity += val * j;
				}
			}
		} else {
			// 状态结束，区间检索结束
			tmpDemarc.idx_e = j - 1;
			if(tmpDemarc.acc > 0)
				tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
			else 
				tmpDemarc.gravity = 0;
			demarc_arr[demarc_cnt++] = tmpDemarc;

			if(demarc_cnt >= gnDcdMaxLineSize-1)
				break;

			// 新区间初始化
			tmpDemarc.type = type;
			tmpDemarc.idx_b = j;
			tmpDemarc.max_v = val;
			tmpDemarc.acc = val;
			tmpDemarc.gravity = val * j;
			isActive = 1;
			maxv = val;
		}
	}

	// 末尾节点处理
	tmpDemarc.idx_e = width - 1;
	if(tmpDemarc.acc > 0)
		tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
	else 
		tmpDemarc.gravity = 0;
	demarc_arr[demarc_cnt++] = tmpDemarc;

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	IplImage * iplBinaImg = cvCreateImage( cvSize(width, height+32+256+128+16+8+4*4), 8, 1 );
	IplImage * iplBinaImg3C = cvCreateImage( cvGetSize(iplBinaImg), 8, 3 );

	int offsetY = 0;
	// 绘制图像
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int gloom = im[y*width+x];
			if(y < slice_top || y > slice_bottom)
				gloom = gloom / 2;
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*y))[x] = gloom;
		}
	}
	offsetY += height;
	// 绘制分割区
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
		}
	}
	offsetY += 4;
	// 绘制分块均值
	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 255-pColumnArr[x];
		}
	}
	offsetY += 32;
	// 绘制分割区
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
		}
	}
	offsetY += 4;
	// 绘制灰度曲线
	for (int y = 0; y < 256; y++) {
		unsigned char * pDbgIm = (unsigned char *)iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY);
		memset(pDbgIm, 0, iplBinaImg->widthStep * sizeof(unsigned char));
		for (int x = 0; x < width; x++) {
			if(y >= 255 - pColumnArr[x])
				pDbgIm[x] = 255;
		}
	}
	offsetY += 256;
	// 绘制分割区
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
		}
	}
	offsetY += 4;

	cvCvtColor(iplBinaImg, iplBinaImg3C, CV_GRAY2BGR);

	// 绘制梯度曲线
	for (int y = 0; y < 128; y++) {
		unsigned char * pDbgIm = (unsigned char *)iplBinaImg3C->imageData + iplBinaImg3C->widthStep*(y+offsetY);
		memset(pDbgIm, 0, iplBinaImg3C->widthStep * sizeof(unsigned char));
		for (int x = 0; x < width; x++) {
			if(y >= 128 - abs(pGradArr[x])) {
				pDbgIm[3*x] = 0;
				pDbgIm[3*x+1] = pGradArr[x] > 0 ? 255 : 0;
				pDbgIm[3*x+2] = pGradArr[x] < 0 ? 255 : 0;
			}
		}
	}
	offsetY += 128;

	// 绘制分割区
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x] = 0x80;
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x+1] = 0x80;
			((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x+2] = 0x80;
		}
	}
	offsetY += 4;

	memset(iplBinaImg3C->imageData + offsetY * iplBinaImg3C->widthStep, 0, 
		iplBinaImg3C->widthStep * 16 * sizeof(unsigned char));
	int offsetYMaxv = height + 32 + 256 + 128 + 3*4;
	// 绘制曲线划分结果
	for(int kkk = 0; kkk < demarc_cnt; kkk++) {
		CvScalar color;
		if(0 == demarc_arr[kkk].type)
			color = CV_RGB(0,0,255);
		else if(1 == demarc_arr[kkk].type)
			color = CV_RGB(0,255,0);
		else if(-1 == demarc_arr[kkk].type)
			color = CV_RGB(255,0,0);
		if(kkk % 2) {
			cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetY), cvPoint(demarc_arr[kkk].idx_e, offsetY+7), color, CV_FILLED);
		} else {
			cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetY+8), cvPoint(demarc_arr[kkk].idx_e, offsetY+15), color, CV_FILLED);
		}
		if(demarc_arr[kkk].max_v > 0)
			cvLine(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetYMaxv-demarc_arr[kkk].max_v), 
			cvPoint(demarc_arr[kkk].idx_e, offsetYMaxv-demarc_arr[kkk].max_v), CV_RGB(255,255,0));
	}
	offsetY += 16;

	memset(iplBinaImg3C->imageData + offsetY * iplBinaImg3C->widthStep, 0, 
		iplBinaImg3C->widthStep * 8 * sizeof(unsigned char));

	int dbgLumL = minGray, dbgLumH = maxGray;
#endif _DEBUG_DECODE
#endif _DEBUG_

	lumL = minGray;
	lumH = maxGray;
	// 筛选合并分界区域（紧约束）
	demarc_effcnt = isActive = 0;
	for(j = 0; j < demarc_cnt; j++) {
// 		minv = demarc_arr[j].type > 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
// 		maxv = demarc_arr[j].type < 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
// 		diff = (lumH - lumL) / 5;
		// 过阈值的有效域
		if(demarc_arr[j].acc >= gradThreshold) {
			minv = demarc_arr[j].type > 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
			maxv = demarc_arr[j].type < 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
			if(0 == demarc_effcnt) {
				lumL = (minGray + minv) >> 1;
				lumH = (maxGray + maxv) >> 1;
			}
			diff = (lumH - lumL) / 5;

			demarc_arr[j].idxex_b = demarc_arr[j].idx_b;
			demarc_arr[j].idxex_e = demarc_arr[j].idx_e;
			// 确定有效边界
			for(k = demarc_arr[j].idx_b; k <= demarc_arr[j].idx_e; k++) {
				if(abs(pGradArr[k]) > (demarc_arr[j].max_v >> 1)) {
					demarc_arr[j].idxex_b = (k > demarc_arr[j].idx_b) ? (k-1) : k;
					break;
				}
			}
			for(k = demarc_arr[j].idx_e; k >= demarc_arr[j].idx_b; k--) {
				if(abs(pGradArr[k]) > (demarc_arr[j].max_v >> 1)) {
					demarc_arr[j].idxex_e = (k < demarc_arr[j].idx_b) ? (k+1) : k;
					break;
				}
			}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
			cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[j].idx_b, offsetY), cvPoint(demarc_arr[j].idx_e, offsetY+7), CV_RGB(0,128,128), CV_FILLED);
			cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[j].idxex_b, offsetY), cvPoint(demarc_arr[j].idxex_e, offsetY+7), CV_RGB(0,255,255), CV_FILLED);
#endif _DEBUG_DECODE
#endif _DEBUG_
			// 之前有未知区间，则验证插入
			if(isActive) {
				// 前后有效域为同种上升/下降类型，中间插入一个待定域
				if(demarc_arr[j].type == demarc_effctv[demarc_effcnt-1].type) {
					tmpDemarc.type = 0;
					demarc_effctv[demarc_effcnt++] = tmpDemarc;
					if(demarc_effcnt >= gnDcdMaxLineSize-1)
						break;
					// 前后有效域为不同上升/下降类型，验证是否为待定域
				} else {
					val = (pColumnArr[tmpDemarc.idx_b] + pColumnArr[tmpDemarc.idx_e]) / 2;
					if(val >= lumL + diff && val <= lumH - diff
						/*&& tmpDemarc.acc >= gradThreshold*/) {
							tmpDemarc.type = 0;
							demarc_effctv[demarc_effcnt++] = tmpDemarc;
							if(demarc_effcnt >= gnDcdMaxLineSize-1)
								break;
					}
				}
			}
			// 根据与之对比结果微调重心，学习阈值
			// 小值未过阈值
			if(minv > lumL + diff && maxv > lumH - diff) {
				if(demarc_arr[j].type > 0) {
					fTune = (demarc_arr[j].gravity - demarc_arr[j].idxex_b) / 4;
					demarc_arr[j].gravity = demarc_arr[j].gravity - fTune;
				} else {
					fTune = (demarc_arr[j].idxex_e - demarc_arr[j].gravity) / 4;
					demarc_arr[j].gravity = demarc_arr[j].gravity + fTune;
				}
				// 灰度阈值学习
				lumL = lumL * (1 - fLumLrningRateSlow) + minv * fLumLrningRateSlow;
				lumH = lumH * (1 - fLumLrningRateFast) + maxv * fLumLrningRateFast;
			}
			// 大值未过阈值
			else if(minv < lumL + diff && maxv < lumH - diff) {
				if(demarc_arr[j].type > 0) {
					fTune = (demarc_arr[j].idxex_e - demarc_arr[j].gravity) / 3;
					demarc_arr[j].gravity = demarc_arr[j].gravity + fTune;
				} else {
					fTune = (demarc_arr[j].gravity - demarc_arr[j].idxex_b) / 3;
					demarc_arr[j].gravity = demarc_arr[j].gravity - fTune;
				}
				// 灰度阈值学习
				lumL = lumL * (1 - fLumLrningRateFast) + minv * fLumLrningRateFast;
				lumH = lumH * (1 - fLumLrningRateSlow) + maxv * fLumLrningRateSlow;
			}
			// 大小值皆未过阈值，慢速学习
			else if(minv > lumL + diff && maxv < lumH - diff) {
				// 灰度阈值学习
				lumL = lumL * (1 - fLumLrningRateSlow) + minv * fLumLrningRateSlow;
				lumH = lumH * (1 - fLumLrningRateSlow) + maxv * fLumLrningRateSlow;
			} 
			// 大小值皆过阈值，快速学习
			else {
				// 灰度阈值学习
				lumL = lumL * (1 - fLumLrningRateFast) + minv * fLumLrningRateFast;
				lumH = lumH * (1 - fLumLrningRateFast) + maxv * fLumLrningRateFast;
			}
			demarc_effctv[demarc_effcnt++] = demarc_arr[j];
			if(demarc_effcnt >= gnDcdMaxLineSize-1)
				break;
			isActive = 0;
		}
		// 未过阈值的待定域
		else if(demarc_effcnt) {
			// 之前有未知区间，则合并
			if(isActive) {
				tmpDemarc.idx_e = demarc_arr[j].idx_e;
				tmpDemarc.acc += demarc_arr[j].acc;
				tmpDemarc.count += 1;
			} 
			// 之前没有未知区域，则创建
			else {
				isActive = 1;
				tmpDemarc.idx_b = demarc_arr[j].idx_b;
				tmpDemarc.idx_e = demarc_arr[j].idx_e;
				tmpDemarc.acc = demarc_arr[j].acc;
				tmpDemarc.count = 1;
			}
		}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		int dbgOffsetYLum = height + 32 + 2*4 + 255;
		if(demarc_effcnt > 0) {
			int dbgDiff1 = (lumH - lumL) / 5;
			int dbgDiff0 = (dbgLumH - dbgLumL) / 5;
			cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumL),
				cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumL), CV_RGB(255, 0, 0));
			cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumH),
				cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumH), CV_RGB(0, 255, 0));

			cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumL-dbgDiff0),
				cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumL-dbgDiff1), CV_RGB(255, 96, 96));
			cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumH+dbgDiff0),
				cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumH+dbgDiff1), CV_RGB(96, 255, 96));
		}
		dbgLumL = lumL;
		dbgLumH = lumH;
#endif _DEBUG_
#endif _DEBUG_DECODE
	}

	// 找出最小宽度，以此为基准划分未知区域
	for(j = 1; j < demarc_effcnt; j++) {
		if(0 == demarc_effctv[j].type + demarc_effctv[j-1].type) {
			fVal = demarc_effctv[j].gravity - demarc_effctv[j-1].gravity;
			fMin = fVal < fMin ? fVal : fMin;
		}
	}

	fMin = (fMin > 0.5) ? fMin : 0.5;

	// 未知区域划分，宽度写入
	fDemarcCoord_basic[0] = demarc_effctv[0].gravity;
	fDemarcCoord_strict[0] = demarc_effctv[0].gravity;
	decodeCnt_basic = decodeCnt_strict = 0;
	for(j = 1; j < demarc_effcnt; j++) {
		// 未知区域，跳至下一区间
		if(0 == demarc_effctv[j].type)
			continue;
		// 当前和上一个区域皆为确定边界，计算宽度
		else if(0 != demarc_effctv[j-1].type) {
			fVal = demarc_effctv[j].gravity - demarc_effctv[j-1].gravity;
			// basic策略
			fDecodeElements_basic[decodeCnt_basic] = (demarc_effctv[j].type > 0) ? fVal : (0 - fVal);
			nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
			fDemarcCoord_basic[decodeCnt_basic+1] = demarc_effctv[j].gravity;
			decodeCnt_basic++;
			if(decodeCnt_basic >= gnDcdMaxLineSize-1)
				break;

			// strict策略
			fDecodeElements_strict[decodeCnt_strict] = (demarc_effctv[j].type > 0) ? fVal : (0 - fVal);
			nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
			fDemarcCoord_strict[decodeCnt_strict+1] = demarc_effctv[j].gravity;
			decodeCnt_strict++;
			if(decodeCnt_strict >= gnDcdMaxLineSize-1)
				break;
		} 
		// 上一个为未知区域
		else {
			// 计算宽度容许度
			fVal = demarc_effctv[j].gravity - demarc_effctv[j-2].gravity;
			fCnt = fVal / fMin;

			// 若未知区域前后确定边界为同一类型
			if(demarc_effctv[j].type == demarc_effctv[j-2].type) {
				// basic策略，则合并或插入两个宽度
				if(fCnt < 2) {
					if(decodeCnt_basic > 0) {
						fTmp = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
						fDecodeElements_basic[decodeCnt_basic-1] 
						= fDecodeElements_basic[decodeCnt_basic-1] + fVal;
						nDecodeArr_basic[decodeCnt_basic-1] = fDecodeElements_basic[decodeCnt_basic-1] * 100;
						fDemarcCoord_basic[decodeCnt_basic] = (demarc_effctv[j].gravity * demarc_effctv[j].acc
							+ demarc_effctv[j-2].gravity * demarc_effctv[j-2].acc)
							/ (demarc_effctv[j].acc + demarc_effctv[j-2].acc);
					}
				} else {
					fTmp = (0 > demarc_effctv[j].type) ? fVal : (0-fVal);
					fTmp = fTmp / 2;
					fDecodeElements_basic[decodeCnt_basic] = fTmp;
					nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
					fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fabs(fTmp);
					decodeCnt_basic++;
					fDecodeElements_basic[decodeCnt_basic] = -fTmp;
					nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
					fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fabs(fTmp);
					decodeCnt_basic++;
					if(decodeCnt_basic >= gnDcdMaxLineSize-1)
						break;
				}

				// strict策略，则插入偶数个宽度
				for(cnt = 2; cnt < 9; cnt += 2) {
//					if(fCnt + 0.5 < cnt + 2)
					if(fCnt < cnt + 1)
						break;
				}
				fVal = fVal / cnt;
				if(decodeCnt_strict >= gnDcdMaxLineSize-cnt)
					break;
				for(k = 0; k < cnt; k++) {
					fTmp = (0 > demarc_effctv[j].type) ? fVal : (0-fVal);
					fDecodeElements_strict[decodeCnt_strict] = (0 == k % 2) ? fTmp : (0-fTmp);
					nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
					fDemarcCoord_strict[decodeCnt_strict+1] = fDemarcCoord_strict[decodeCnt_strict] + fVal;
					decodeCnt_strict++;
				}
			}
			// 若未知区域前后确定边界为不同类型
			else {
				// basic策略，越过未知区域
				fDecodeElements_basic[decodeCnt_basic] = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
				nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
				fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fVal;
				decodeCnt_basic++;
				if(decodeCnt_basic >= gnDcdMaxLineSize-1)
					break;

				// strict策略，未知区域插入奇数个宽度
				for(cnt = 1; cnt < 8; cnt += 2) {
					if(fCnt < cnt + 1)
						break;
				}
				fVal = fVal / cnt;
				if(decodeCnt_strict >= gnDcdMaxLineSize-cnt)
					break;
				for(k = 0; k < cnt; k++) {
					fTmp = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
					fDecodeElements_strict[decodeCnt_strict] = (0 == k % 2) ? fTmp : (0-fTmp);
					nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
					fDemarcCoord_strict[decodeCnt_strict+1] = fDemarcCoord_strict[decodeCnt_strict] + fVal;
					decodeCnt_strict++;
				}
			}
		}
	}

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	if(16 > slice_height)
		goto nExit;
	printf("********basic fMin = %3.2f\n", fMin);
	int dbgOffsetYDem_basic = height - 4;
	for(int jjj = 0; jjj < decodeCnt_basic; jjj++) {
		if(0 != jjj && 0 == jjj % 6)
			printf("\n");
		if(0 == jjj % 2)
			printf("[%2d]=%4.2f  ", jjj, fDecodeElements_basic[jjj]);
		else
			printf("[%2d]=%4.2f  ", jjj, fDecodeElements_basic[jjj]);

		cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_basic[jjj]+0.5), dbgOffsetYDem_basic),
			cvPoint(int(fDemarcCoord_basic[jjj]+0.5), dbgOffsetYDem_basic+12), CV_RGB(0, 255, 0));
	}
	cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_basic[decodeCnt_basic]+0.5), dbgOffsetYDem_basic),
		cvPoint(int(fDemarcCoord_basic[decodeCnt_basic]+0.5), dbgOffsetYDem_basic+12), CV_RGB(0, 255, 0));

	printf("\n********strict fMin = %3.2f\n", fMin);
	int dbgOffsetYDem_strict = height + 32 + 4 - 8;
	for(int jjj = 0; jjj < decodeCnt_strict; jjj++) {
		if(0 != jjj && 0 == jjj % 6)
			printf("\n");
		if(0 == jjj % 2)
			printf("[%2d]=%4.2f  ", jjj, fDecodeElements_strict[jjj]);
		else
			printf("[%2d]=%4.2f  ", jjj, fDecodeElements_strict[jjj]);

		cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_strict[jjj]+0.5), dbgOffsetYDem_strict),
			cvPoint(int(fDemarcCoord_strict[jjj]+0.5), dbgOffsetYDem_strict+12), CV_RGB(255, 0, 0));
	}
	printf("\n");
	cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_strict[decodeCnt_strict]+0.5), dbgOffsetYDem_strict),
		cvPoint(int(fDemarcCoord_strict[decodeCnt_strict]+0.5), dbgOffsetYDem_strict+12), CV_RGB(255, 0, 0));
	cvNamedWindow("Decode");
	cvShowImage("Decode", iplBinaImg3C);
	cvWaitKey();
#endif _DEBUG_DECODE
#endif _DEBUG_

nExit:

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	if(iplBinaImg)
		cvReleaseImage(&iplBinaImg);
	if(iplBinaImg3C)
		cvReleaseImage(&iplBinaImg3C);
#endif _DEBUG_DECODE
#endif _DEBUG_

	*decode_bcnt = decodeCnt_basic;
	*decode_scnt = decodeCnt_strict;
	*min_module = fMin;

	return 1;
}




// v2.6版本开始使用的解码算法
int BarcodeDecoding_Integrogram( unsigned char * im, int * integr, int width, int height, int slice_height, 
				  char * code_result, int * code_type, int * char_num, int * module_num, 
				  int * code_direct, int * leftOffset, int * rightOffset)
{
	int ret_val = 0;
	int i = 0, j = 0, k = 0, status = 0;

	int nColCount = 0;

	int slice_cnt = 0, slice_remain = 0, slice_offset = 0;
	int slice_top = 0, slice_bottom = 0;

	char strResult[128];
	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nLeftIdx = 0, nRightIdx = 0, nCodeType = 0;
	int nCodeWidth = 0, nLeftOffset = 0, nRightOffset = 0;
	float fModuleWid = 0.0F;

	int * pColumnArr = gpnDcdColumnscanArr;
	int * pGradArr = gpnDcdPartitionArr;

	int maxGray = 0, minGray = 255;

	int * pIntegTop = 0, * pIntegBtm = 0;
	int cnt = 0;
	int type = 0, isActive = 0;
	int val = 0, maxv = 0, minv = 0;

	const int gradThreshold = 30;
	const float fLumLrningRateFast = 0.15;
	const float fLumLrningRateSlow = 0.08;
	int lumL = 0, lumH = 0;
	int diff = 0;

	DecodeDemarcateNode tmpDemarc;
	DecodeDemarcateNode * demarc_arr = gpDDNOrig_arr;
	int demarc_cnt = 0;

	DecodeDemarcateNode * demarc_effctv = gpDDNEffc_arr;
	int demarc_effcnt = 0;

	float * fDemarcCoord_basic = gpfDcdCoor_basic;
	float * fDecodeElements_basic = gpfDcdDecodeArr_basic;
	int decodeCnt_basic = 0;

	float * fDemarcCoord_strict = gpfDcdCoor_strict;
	float * fDecodeElements_strict = gpfDcdDecodeArr_strict;
	int decodeCnt_strict = 0;

	int * nDecodeArr_basic = gpnDcdDecodeArr;
	int * nDecodeArr_strict = gpnDcdDecodeArr2;
	int * nDecodeArrProc = gpnDcdDecodeArrProc;

	if( 1 != gnDcdInitFlag ) {
		ret_val = -1;
		goto nExit;
	}

	if( NULL == im || NULL == integr) {
		ret_val = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnDcdMaxWidth || height > gnDcdMaxHeight ) {
			ret_val = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	IplImage * iplBinaImg = cvCreateImage( cvSize(width, height+32+256+128+16+8+4*4), 8, 1 );
	IplImage * iplBinaImg3C = cvCreateImage( cvGetSize(iplBinaImg), 8, 3 );
#endif _DEBUG_DECODE
#endif _DEBUG_

	//slice_height = 8;
	slice_cnt = height / (slice_height >> 1) - 1;
	slice_top = 0;
	slice_bottom = slice_height - 1;
	for(i = 0; i < slice_cnt; i++) {
		// 获取该slice的列灰度平均数组
		pIntegTop = integr + (slice_top-1) * width;
		pIntegBtm = integr + slice_bottom * width;
		for(j = 0; j < width; j++) {
			pColumnArr[j] = pIntegBtm[j] - (0 < slice_top ? pIntegTop[j] : 0);
			pColumnArr[j] /= slice_height;
			pColumnArr[j] = 255 - pColumnArr[j];

			minGray = pColumnArr[j] < minGray ? pColumnArr[j] : minGray;
			maxGray = pColumnArr[j] > maxGray ? pColumnArr[j] : maxGray;
		}

		// 求取曲线梯度
		pGradArr[0] = 0;
		for(j = 1; j < width; j++) {
			pGradArr[j] = pColumnArr[j] - pColumnArr[j-1];
		}

		// 分析曲线并写入分界结构体数组
		demarc_cnt = 0;
		tmpDemarc.type = tmpDemarc.idx_b = tmpDemarc.idx_e = tmpDemarc.acc = tmpDemarc.gravity = 0;
		isActive = 1;
		maxv = 0;
		for(j = 1; j < width; j++) {
			// 获取当前点状态
			type = (0 == pGradArr[j]) ? 0 : (0 < pGradArr[j] ? 1 : -1);
			val = abs(pGradArr[j]);
			// 当前点状态与当前检索区间状态对比
			if(type == tmpDemarc.type) {
				// 若为连续0区间，则跳过值验证
				if(0 == type)
					continue;
				// 若区间检索为活跃状态，则验证当前值是否满足失效条件，记录最值
				else if(isActive) {
					// 若满足失效条件，则记录最小值
					if(maxv>>1 >= val) {
						isActive = 0;
						minv = val;
					} else {
						// 若为连续区间，则记录最大值
						maxv = val > maxv ? val : maxv;
						tmpDemarc.max_v = maxv;
					}
					tmpDemarc.acc += val;
					tmpDemarc.gravity += val * j;

				// 若区间检索为失效状态，则验证当前值是否满足结束条件，记录最值
				} else {
					// 满足结束条件，区间检索结束
					if(val > minv) {
						tmpDemarc.idx_e = j - 2;
						if(tmpDemarc.acc > 0)
							tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
						else 
							tmpDemarc.gravity = 0;

						demarc_arr[demarc_cnt++] = tmpDemarc;

						// 最谷值处添加一个0区
						demarc_arr[demarc_cnt].type = 0;
						demarc_arr[demarc_cnt].idx_b = demarc_arr[demarc_cnt].idx_e = j - 1;
						tmpDemarc.max_v = 0;
						demarc_arr[demarc_cnt].acc = 0;
						demarc_arr[demarc_cnt].gravity = 0;
						demarc_cnt++;

						// 新区间初始化
						tmpDemarc.type = type;
						tmpDemarc.idx_b = j;
						tmpDemarc.max_v = val;
						tmpDemarc.acc = val;
						tmpDemarc.gravity = val * j;
						isActive = 1;
						maxv = val;
					} else {
						minv = val;
						tmpDemarc.acc += val;
						tmpDemarc.gravity += val * j;
					}
				}
			} else {
				// 状态结束，区间检索结束
				tmpDemarc.idx_e = j - 1;
				if(tmpDemarc.acc > 0)
					tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
				else 
					tmpDemarc.gravity = 0;
				demarc_arr[demarc_cnt++] = tmpDemarc;

				// 新区间初始化
				tmpDemarc.type = type;
				tmpDemarc.idx_b = j;
				tmpDemarc.max_v = val;
				tmpDemarc.acc = val;
				tmpDemarc.gravity = val * j;
				isActive = 1;
				maxv = val;
			}
		}

		tmpDemarc.idx_e = width - 1;
		if(tmpDemarc.acc > 0)
			tmpDemarc.gravity = tmpDemarc.gravity / tmpDemarc.acc;
		else 
			tmpDemarc.gravity = 0;
		demarc_arr[demarc_cnt++] = tmpDemarc;

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		int offsetY = 0;
		// 绘制图像
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int gloom = im[y*width+x];
				if(y < slice_top || y > slice_bottom)
					gloom = gloom / 2;
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*y))[x] = gloom;
			}
		}
		offsetY += height;
		// 绘制分割区
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
			}
		}
		offsetY += 4;
		// 绘制分块均值
		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 255-pColumnArr[x];
			}
		}
		offsetY += 32;
		// 绘制分割区
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
			}
		}
		offsetY += 4;
		// 绘制灰度曲线
		for (int y = 0; y < 256; y++) {
			unsigned char * pDbgIm = (unsigned char *)iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY);
			memset(pDbgIm, 0, iplBinaImg->widthStep * sizeof(unsigned char));
			for (int x = 0; x < width; x++) {
				if(y >= 255 - pColumnArr[x])
					pDbgIm[x] = 255;
			}
		}
		offsetY += 256;
		// 绘制分割区
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[x] = 0x80;
			}
		}
		offsetY += 4;

		cvCvtColor(iplBinaImg, iplBinaImg3C, CV_GRAY2BGR);

		// 绘制梯度曲线
		for (int y = 0; y < 128; y++) {
			unsigned char * pDbgIm = (unsigned char *)iplBinaImg3C->imageData + iplBinaImg3C->widthStep*(y+offsetY);
			memset(pDbgIm, 0, iplBinaImg3C->widthStep * sizeof(unsigned char));
			for (int x = 0; x < width; x++) {
				if(y >= 128 - abs(pGradArr[x])) {
					pDbgIm[3*x] = 0;
					pDbgIm[3*x+1] = pGradArr[x] > 0 ? 255 : 0;
					pDbgIm[3*x+2] = pGradArr[x] < 0 ? 255 : 0;
				}
			}
		}
		offsetY += 128;

		// 绘制分割区
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < width; x++) {
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x] = 0x80;
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x+1] = 0x80;
				((unsigned char *)(iplBinaImg->imageData + iplBinaImg->widthStep*(y+offsetY)))[3*x+2] = 0x80;
			}
		}
		offsetY += 4;

		memset(iplBinaImg3C->imageData + offsetY * iplBinaImg3C->widthStep, 0, 
			iplBinaImg3C->widthStep * 16 * sizeof(unsigned char));
		int offsetYMaxv = height + 32 + 256 + 128 + 3*4;
		// 绘制曲线划分结果
		for(int kkk = 0; kkk < demarc_cnt; kkk++) {
			CvScalar color;
			if(0 == demarc_arr[kkk].type)
				color = CV_RGB(0,0,255);
			else if(1 == demarc_arr[kkk].type)
				color = CV_RGB(0,255,0);
			else if(-1 == demarc_arr[kkk].type)
				color = CV_RGB(255,0,0);
			if(kkk % 2) {
				cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetY), cvPoint(demarc_arr[kkk].idx_e, offsetY+7), color, CV_FILLED);
			} else {
				cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetY+8), cvPoint(demarc_arr[kkk].idx_e, offsetY+15), color, CV_FILLED);
			}
			if(demarc_arr[kkk].max_v > 0)
				cvLine(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetYMaxv-demarc_arr[kkk].max_v), 
					cvPoint(demarc_arr[kkk].idx_e, offsetYMaxv-demarc_arr[kkk].max_v), CV_RGB(255,255,0));
		}
		offsetY += 16;

		memset(iplBinaImg3C->imageData + offsetY * iplBinaImg3C->widthStep, 0, 
			iplBinaImg3C->widthStep * 8 * sizeof(unsigned char));
		// 绘制阈值筛选结果
// 		for(int kkk = 0; kkk < demarc_cnt; kkk++) {
// 			if(demarc_arr[kkk].acc >= gradThreshold) {
// 				cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[kkk].idx_b, offsetY), cvPoint(demarc_arr[kkk].idx_e, offsetY+7), CV_RGB(0,255,255), CV_FILLED);
// 			}
// 		}
//		offsetY += 8;

		int dbgLumL = minGray, dbgLumH = maxGray;
#endif _DEBUG_DECODE
#endif _DEBUG_


		float fTune = 0;
		lumL = minGray;
		lumH = maxGray;
		// 筛选合并分界区域（紧约束）
		demarc_effcnt = isActive = 0;
		for(j = 0; j < demarc_cnt; j++) {
// 			minv = demarc_arr[j].type > 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
// 			maxv = demarc_arr[j].type < 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
// 			diff = (lumH - lumL) / 5;
			// 过阈值的有效域
			if(demarc_arr[j].acc >= gradThreshold) {
				minv = demarc_arr[j].type > 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
				maxv = demarc_arr[j].type < 0 ? pColumnArr[demarc_arr[j].idx_b] : pColumnArr[demarc_arr[j].idx_e];
				if(0 == demarc_effcnt) {
					lumL = (minGray + minv) >> 1;
					lumH = (maxGray + maxv) >> 1;
				}
				diff = (lumH - lumL) / 5;

				demarc_arr[j].idxex_b = demarc_arr[j].idx_b;
				demarc_arr[j].idxex_e = demarc_arr[j].idx_e;
				// 确定有效边界
				for(k = demarc_arr[j].idx_b; k <= demarc_arr[j].idx_e; k++) {
					if(abs(pGradArr[k]) > (demarc_arr[j].max_v >> 1)) {
						demarc_arr[j].idxex_b = (k > demarc_arr[j].idx_b) ? (k-1) : k;
						break;
					}
				}
				for(k = demarc_arr[j].idx_e; k >= demarc_arr[j].idx_b; k--) {
					if(abs(pGradArr[k]) > (demarc_arr[j].max_v >> 1)) {
						demarc_arr[j].idxex_e = (k < demarc_arr[j].idx_b) ? (k+1) : k;
						break;
					}
				}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
				cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[j].idx_b, offsetY), cvPoint(demarc_arr[j].idx_e, offsetY+7), CV_RGB(0,128,128), CV_FILLED);
				cvRectangle(iplBinaImg3C, cvPoint(demarc_arr[j].idxex_b, offsetY), cvPoint(demarc_arr[j].idxex_e, offsetY+7), CV_RGB(0,255,255), CV_FILLED);
#endif _DEBUG_DECODE
#endif _DEBUG_
				// 之前有未知区间，则验证插入
				if(isActive) {
					// 前后有效域为同种上升/下降类型，中间插入一个待定域
					if(demarc_arr[j].type == demarc_effctv[demarc_effcnt-1].type) {
						tmpDemarc.type = 0;
						demarc_effctv[demarc_effcnt++] = tmpDemarc;
					// 前后有效域为不同上升/下降类型，验证是否为待定域
					} else {
						val = (pColumnArr[tmpDemarc.idx_b] + pColumnArr[tmpDemarc.idx_e]) / 2;
						if(val >= lumL + diff && val <= lumH - diff
							/*&& tmpDemarc.acc >= gradThreshold*/) {
							tmpDemarc.type = 0;
							demarc_effctv[demarc_effcnt++] = tmpDemarc;
						}
					}
				}
				// 根据与之对比结果微调重心，学习阈值
				// 小值未过阈值
				if(minv > lumL + diff && maxv > lumH - diff) {
					if(demarc_arr[j].type > 0) {
						fTune = (demarc_arr[j].gravity - demarc_arr[j].idxex_b) / 2;
						demarc_arr[j].gravity = demarc_arr[j].gravity - fTune;
					} else {
						fTune = (demarc_arr[j].idxex_e - demarc_arr[j].gravity) / 2;
						demarc_arr[j].gravity = demarc_arr[j].gravity + fTune;
					}
					// 灰度阈值学习
					lumL = lumL * (1 - fLumLrningRateSlow) + minv * fLumLrningRateSlow;
					lumH = lumH * (1 - fLumLrningRateFast) + maxv * fLumLrningRateFast;
				}
				// 大值未过阈值
				else if(minv < lumL + diff && maxv < lumH - diff) {
					if(demarc_arr[j].type > 0) {
						fTune = (demarc_arr[j].idxex_e - demarc_arr[j].gravity) / 2;
						demarc_arr[j].gravity = demarc_arr[j].gravity + fTune;
					} else {
						fTune = (demarc_arr[j].gravity - demarc_arr[j].idxex_b) / 2;
						demarc_arr[j].gravity = demarc_arr[j].gravity - fTune;
					}
					// 灰度阈值学习
					lumL = lumL * (1 - fLumLrningRateFast) + minv * fLumLrningRateFast;
					lumH = lumH * (1 - fLumLrningRateSlow) + maxv * fLumLrningRateSlow;
				}
				// 大小值皆未过阈值，慢速学习
				else if(minv > lumL + diff && maxv < lumH - diff) {
					// 灰度阈值学习
					lumL = lumL * (1 - fLumLrningRateSlow) + minv * fLumLrningRateSlow;
					lumH = lumH * (1 - fLumLrningRateSlow) + maxv * fLumLrningRateSlow;
				} 
				// 大小值皆过阈值，快速学习
				else {
					// 灰度阈值学习
					lumL = lumL * (1 - fLumLrningRateFast) + minv * fLumLrningRateFast;
					lumH = lumH * (1 - fLumLrningRateFast) + maxv * fLumLrningRateFast;
				}
				demarc_effctv[demarc_effcnt++] = demarc_arr[j];
				isActive = 0;
			}
			// 未过阈值的待定域
			else if(demarc_effcnt) {
				// 之前有未知区间，则合并
				if(isActive) {
					tmpDemarc.idx_e = demarc_arr[j].idx_e;
					tmpDemarc.acc += demarc_arr[j].acc;
					tmpDemarc.count += 1;
				} 
				// 之前没有未知区域，则创建
				else {
					isActive = 1;
					tmpDemarc.idx_b = demarc_arr[j].idx_b;
					tmpDemarc.idx_e = demarc_arr[j].idx_e;
					tmpDemarc.acc = demarc_arr[j].acc;
					tmpDemarc.count = 1;
				}
			}
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
			int dbgOffsetYLum = height + 32 + 2*4 + 255;
			if(demarc_effcnt > 0) {
				int dbgDiff1 = (lumH - lumL) / 5;
				int dbgDiff0 = (dbgLumH - dbgLumL) / 5;
				cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumL),
					cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumL), CV_RGB(255, 0, 0));
				cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumH),
					cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumH), CV_RGB(0, 255, 0));

				cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumL-dbgDiff0),
					cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumL-dbgDiff1), CV_RGB(255, 96, 96));
				cvLine(iplBinaImg3C, cvPoint((demarc_arr[j-1].idx_b+demarc_arr[j-1].idx_e)/2, dbgOffsetYLum-dbgLumH+dbgDiff0),
					cvPoint((demarc_arr[j].idx_b+demarc_arr[j].idx_e)/2, dbgOffsetYLum-lumH+dbgDiff1), CV_RGB(96, 255, 96));
			}
			dbgLumL = lumL;
			dbgLumH = lumH;
#endif _DEBUG_
#endif _DEBUG_DECODE
		}

		float fMin = (float)width, fVal = 0.0;
		// 找出最小宽度，以此为基准划分未知区域
		for(j = 1; j < demarc_effcnt; j++) {
			if(0 == demarc_effctv[j].type + demarc_effctv[j-1].type) {
				fVal = demarc_effctv[j].gravity - demarc_effctv[j-1].gravity;
				fMin = fVal < fMin ? fVal : fMin;
			}
		}

		// 未知区域划分，宽度写入
		float fCnt = 0.0, fTmp = 0.0;
		fDemarcCoord_basic[0] = demarc_effctv[0].gravity;
		fDemarcCoord_strict[0] = demarc_effctv[0].gravity;
		decodeCnt_basic = decodeCnt_strict = 0;
		for(j = 1; j < demarc_effcnt; j++) {
			// 未知区域，跳至下一区间
			if(0 == demarc_effctv[j].type)
				continue;
			// 当前和上一个区域皆为确定边界，计算宽度
			else if(0 != demarc_effctv[j-1].type) {
				fVal = demarc_effctv[j].gravity - demarc_effctv[j-1].gravity;
				// basic策略
				fDecodeElements_basic[decodeCnt_basic] = (demarc_effctv[j].type > 0) ? fVal : (0 - fVal);
				nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
				fDemarcCoord_basic[decodeCnt_basic+1] = demarc_effctv[j].gravity;
				decodeCnt_basic++;

				// strict策略
				fDecodeElements_strict[decodeCnt_strict] = (demarc_effctv[j].type > 0) ? fVal : (0 - fVal);
				nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
				fDemarcCoord_strict[decodeCnt_strict+1] = demarc_effctv[j].gravity;
				decodeCnt_strict++;
			} 
			// 上一个为未知区域
			else {
				// 计算宽度容许度
				fVal = demarc_effctv[j].gravity - demarc_effctv[j-2].gravity;
				fCnt = fVal / fMin;
				
				// 若未知区域前后确定边界为同一类型
				if(demarc_effctv[j].type == demarc_effctv[j-2].type) {
					// basic策略，则合并或插入两个宽度
					if(fCnt < 2) {
						fTmp = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
						fDecodeElements_basic[decodeCnt_basic-1] 
							= fDecodeElements_basic[decodeCnt_basic-1] + fVal;
						nDecodeArr_basic[decodeCnt_basic-1] = fDecodeElements_basic[decodeCnt_basic-1] * 100;
						fDemarcCoord_basic[decodeCnt_basic] = (demarc_effctv[j].gravity * demarc_effctv[j].acc
							+ demarc_effctv[j-2].gravity * demarc_effctv[j-2].acc)
							/ (demarc_effctv[j].acc + demarc_effctv[j-2].acc);
					} else {
						fTmp = (0 > demarc_effctv[j].type) ? fVal : (0-fVal);
						fTmp = fTmp / 2;
						fDecodeElements_basic[decodeCnt_basic] = fTmp;
						nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
						fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fabs(fTmp);
						decodeCnt_basic++;
						fDecodeElements_basic[decodeCnt_basic] = -fTmp;
						nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
						fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fabs(fTmp);
						decodeCnt_basic++;
					}

					// strict策略，则插入偶数个宽度
					for(cnt = 2; cnt < 9; cnt += 2) {
//						if(fCnt + 0.5 < cnt + 2)
						if(fCnt < cnt + 1)
							break;
					}
					fVal = fVal / cnt;
					for(k = 0; k < cnt; k++) {
						fTmp = (0 > demarc_effctv[j].type) ? fVal : (0-fVal);
						fDecodeElements_strict[decodeCnt_strict] = (0 == k % 2) ? fTmp : (0-fTmp);
						nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
						fDemarcCoord_strict[decodeCnt_strict+1] = fDemarcCoord_strict[decodeCnt_strict] + fVal;
						decodeCnt_strict++;
					}
				}
				// 若未知区域前后确定边界为不同类型
				else {
					// basic策略，越过未知区域
					fDecodeElements_basic[decodeCnt_basic] = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
					nDecodeArr_basic[decodeCnt_basic] = fDecodeElements_basic[decodeCnt_basic] * 100;
					fDemarcCoord_basic[decodeCnt_basic+1] = fDemarcCoord_basic[decodeCnt_basic] + fVal;
					decodeCnt_basic++;

					// strict策略，未知区域插入奇数个宽度
					for(cnt = 1; cnt < 8; cnt += 2) {
						if(fCnt < cnt + 1)
							break;
					}
					fVal = fVal / cnt;
					for(k = 0; k < cnt; k++) {
						fTmp = (0 < demarc_effctv[j].type) ? fVal : (0-fVal);
						fDecodeElements_strict[decodeCnt_strict] = (0 == k % 2) ? fTmp : (0-fTmp);
						nDecodeArr_strict[decodeCnt_strict] = fDecodeElements_strict[decodeCnt_strict] * 100;
						fDemarcCoord_strict[decodeCnt_strict+1] = fDemarcCoord_strict[decodeCnt_strict] + fVal;
						decodeCnt_strict++;
					}
				}
			}
		}

		// 试着识别basic
		status = 0;
		nColCount = decodeCnt_basic;
		memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
		memcpy( nDecodeArrProc, nDecodeArr_basic, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
		status = RecgCode128(nDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
			&nDirection, &nLeftIdx, &nRightIdx);
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		printf("********basic fMin = %3.2f\n", fMin);
		int dbgOffsetYDem_basic = height - 4;
		for(int jjj = 0; jjj < decodeCnt_basic; jjj++) {
			if(0 != jjj && 0 == jjj % 6)
				printf("\n");
			if(0 == jjj % 2)
				printf("[%2d]=%4.2f  ", jjj, fDecodeElements_basic[jjj]);
			else
				printf("[%2d]=%4.2f  ", jjj, fDecodeElements_basic[jjj]);

			cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_basic[jjj]+0.5), dbgOffsetYDem_basic),
				cvPoint(int(fDemarcCoord_basic[jjj]+0.5), dbgOffsetYDem_basic+12), CV_RGB(0, 255, 0));
		}
		cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_basic[decodeCnt_basic]+0.5), dbgOffsetYDem_basic),
			cvPoint(int(fDemarcCoord_basic[decodeCnt_basic]+0.5), dbgOffsetYDem_basic+12), CV_RGB(0, 255, 0));

		printf("\n********basic status=%d, strResult=%s\n", status, strResult);
		if(RYU_DECODERR_NULLPTR == status) {
			printf("Decode failed. Null ptr.\n");
		} else if(RYU_DECODERR_SHORTLEN == status) {
			printf("Decode failed. Too short decode arr length.\n");
		} else if(RYU_DECODERR_NOST == status) {
			printf("Decode failed. Cannot find ST.\n");
		} else if(RYU_DECODERR_SHORTLEN_ST == status) {
			printf("Decode failed. Too short decode arr length while find ST.\n");
		} else if(RYU_DECODERR_MATCHFAILED == status) {
			printf("Decode failed. Match failed.\n");
		} else if(RYU_DECODERR_VERIFYFAILED == status) {
			printf("Decode failed. Verify failed.\n");
		} else if(RYU_DECODERR_TRANSCFAILED == status) {
			printf("Decode failed. Transcode failed.\n");
		} else if(0 > status) {
			printf("Decode failed. Unknown error.\n");
		}
#endif _DEBUG_DECODE
#endif _DEBUG_

		// 试着识别strict
		status = 0;
		nColCount = decodeCnt_strict;
		memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
		memcpy( nDecodeArrProc, nDecodeArr_strict, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
		status = RecgCode128(nDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
			&nDirection, &nLeftIdx, &nRightIdx);
#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		printf("********strict fMin = %3.2f\n", fMin);
		int dbgOffsetYDem_strict = height + 32 + 4 - 8;
		for(int jjj = 0; jjj < decodeCnt_strict; jjj++) {
			if(0 != jjj && 0 == jjj % 6)
				printf("\n");
			if(0 == jjj % 2)
				printf("[%2d]=%4.2f  ", jjj, fDecodeElements_strict[jjj]);
			else
				printf("[%2d]=%4.2f  ", jjj, fDecodeElements_strict[jjj]);

			cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_strict[jjj]+0.5), dbgOffsetYDem_strict),
				cvPoint(int(fDemarcCoord_strict[jjj]+0.5), dbgOffsetYDem_strict+12), CV_RGB(255, 0, 0));
		}
		cvLine(iplBinaImg3C, cvPoint(int(fDemarcCoord_strict[decodeCnt_strict]+0.5), dbgOffsetYDem_strict),
			cvPoint(int(fDemarcCoord_strict[decodeCnt_strict]+0.5), dbgOffsetYDem_strict+12), CV_RGB(255, 0, 0));

		printf("\n********strict status=%d, strResult=%s\n", status, strResult);
		if(RYU_DECODERR_NULLPTR == status) {
			printf("Decode failed. Null ptr.\n");
		} else if(RYU_DECODERR_SHORTLEN == status) {
			printf("Decode failed. Too short decode arr length.\n");
		} else if(RYU_DECODERR_NOST == status) {
			printf("Decode failed. Cannot find ST.\n");
		} else if(RYU_DECODERR_SHORTLEN_ST == status) {
			printf("Decode failed. Too short decode arr length while find ST.\n");
		} else if(RYU_DECODERR_MATCHFAILED == status) {
			printf("Decode failed. Match failed.\n");
		} else if(RYU_DECODERR_VERIFYFAILED == status) {
			printf("Decode failed. Verify failed.\n");
		} else if(RYU_DECODERR_TRANSCFAILED == status) {
			printf("Decode failed. Transcode failed.\n");
		} else if(0 > status) {
			printf("Decode failed. Unknown error.\n");
		}
#endif _DEBUG_DECODE
#endif _DEBUG_


#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
		cvNamedWindow("Decode");
		cvShowImage("Decode", iplBinaImg3C);
		cvWaitKey();
#endif _DEBUG_DECODE
#endif _DEBUG_


		slice_top += (slice_height >> 1);
		slice_bottom = slice_top + slice_height - 1;
	}

nExit:

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	if(iplBinaImg)
		cvReleaseImage(&iplBinaImg);
	if(iplBinaImg3C)
		cvReleaseImage(&iplBinaImg3C);
#endif _DEBUG_DECODE
#endif _DEBUG_

	return ret_val;
}

int BarcodeDecoding_init( int max_width, int max_height )
{
	int nRet = 0, status = 0;

	gnDcdMaxWidth = max_width;
	gnDcdMaxHeight = max_height;
	gnDcdMaxLineSize = (int) sqrt( max_width*max_height*1.0 + max_width*max_height*1.0 + 0.5 );

	if(gnDcdMaxWidth <= 0 || gnDcdMaxHeight <= 0 || gnDcdMaxLineSize <= 0) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeDecoding_init, width=%d, height=%d\n",
			max_width, max_height);
#endif
		nRet = -1;
		goto nExit;
	}

	gpnDcdColumnscanArr = (int *) malloc( gnDcdMaxLineSize * sizeof(int) );
	if( !gpnDcdColumnscanArr ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdColumnscanArr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpnDcdPartitionArr = (int *) malloc( gnDcdMaxLineSize * sizeof(int) );
	if( !gpnDcdPartitionArr ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdPartitionArr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpnDcdDecodeArr = (int *) malloc( gnDcdMaxLineSize * sizeof(int) );
	if( !gpnDcdDecodeArr ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdDecodeArr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpnDcdDecodeArrProc =  (int *) malloc( gnDcdMaxLineSize * sizeof(int) );
	if( !gpnDcdDecodeArrProc ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdDecodeArr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gptDcdStartstop = (RyuPoint *) malloc( gnDcdMaxLineSize * sizeof(RyuPoint) );
	if( !gptDcdStartstop ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdDecodeIndex in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpDDNOrig_arr = (DecodeDemarcateNode *)malloc(gnDcdMaxLineSize * sizeof(DecodeDemarcateNode));
	gpDDNEffc_arr = (DecodeDemarcateNode *)malloc(gnDcdMaxLineSize * sizeof(DecodeDemarcateNode));
	if( !gpDDNOrig_arr || !gpDDNEffc_arr) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpDDNOrig_arr & gpDDNEffc_arr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpfDcdCoor_basic = (float *)malloc(gnDcdMaxLineSize * sizeof(float));
	gpfDcdDecodeArr_basic = (float *)malloc(gnDcdMaxLineSize * sizeof(float));
	if( !gpfDcdCoor_basic || !gpfDcdDecodeArr_basic) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpfDcdCoor_basic & gpfDcdDecodeArr_basic in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpfDcdCoor_strict = (float *)malloc(gnDcdMaxLineSize * sizeof(float));
	gpfDcdDecodeArr_strict = (float *)malloc(gnDcdMaxLineSize * sizeof(float));
	if( !gpfDcdCoor_strict || !gpfDcdDecodeArr_strict) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpfDcdCoor_strict & gpfDcdDecodeArr_strict in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpnDcdDecodeArr2 = (int *) malloc( gnDcdMaxLineSize * sizeof(int) );
	if( !gpnDcdDecodeArr2 ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpnDcdDecodeArr2 in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpDRNCandArr = (DecodeResultNode *)malloc(gnDRNCandMaxCnt *  sizeof(DecodeResultNode));
	if( !gpDRNCandArr ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpDRNCandArr in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	gpDcdDecoderMemSets = (unsigned char *)malloc(gnDcdDecoderMemSize *  sizeof(unsigned char));
	if( !gpDcdDecoderMemSets ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gpDcdDecoderMemSets in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	status = allocVariableMemStorage_code128(gpDcdDecoderMemSets, gnDcdDecoderMemSize);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for code128 decoder in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	status = allocVariableMemStorage_code39(gpDcdDecoderMemSets, gnDcdDecoderMemSize);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for code39 decoder in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	status = allocVariableMemStorage_code93(gpDcdDecoderMemSets, gnDcdDecoderMemSize);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for code93 decoder in BarcodeDecoding_init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	nRet = gnDcdInitFlag = 1;

nExit:
	return nRet;
}

void BarcodeDecoding_release()
{
	if(gpnDcdColumnscanArr) {
		free(gpnDcdColumnscanArr);
		gpnDcdColumnscanArr = 0;
	}

	if(gpnDcdPartitionArr) {
		free(gpnDcdPartitionArr);
		gpnDcdPartitionArr = 0;
	}

	if(gpnDcdDecodeArr) {
		free(gpnDcdDecodeArr);
		gpnDcdDecodeArr = 0;
	}

	if(gpnDcdDecodeArrProc) {
		free(gpnDcdDecodeArrProc);
		gpnDcdDecodeArrProc = 0;
	}

	if(gptDcdStartstop) {
		free(gptDcdStartstop);
		gptDcdStartstop = 0;
	}

	if(gpDDNOrig_arr) {
		free(gpDDNOrig_arr);
		gpDDNOrig_arr = 0;
	}

	if(gpDDNEffc_arr) {
		free(gpDDNEffc_arr);
		gpDDNEffc_arr = 0;
	}

	if(gpfDcdCoor_basic) {
		free(gpfDcdCoor_basic);
		gpfDcdCoor_basic = 0;
	}

	if(gpfDcdDecodeArr_basic) {
		free(gpfDcdDecodeArr_basic);
		gpfDcdDecodeArr_basic = 0;
	}

	if(gpfDcdCoor_strict) {
		free(gpfDcdCoor_strict);
		gpfDcdCoor_strict = 0;
	}

	if(gpfDcdDecodeArr_strict) {
		free(gpfDcdDecodeArr_strict);
		gpfDcdDecodeArr_strict = 0;
	}

	if(gpnDcdDecodeArr2) {
		free(gpnDcdDecodeArr2);
		gpnDcdDecodeArr2 = 0;
	}

	if(gpDRNCandArr) {
		free(gpDRNCandArr);
		gpDRNCandArr = 0;
	}

	if(gpDcdDecoderMemSets) {
		free(gpDcdDecoderMemSets);
		gpDcdDecoderMemSets = 0;
	}
	
	resetVariableMemStorage_code128();

	resetVariableMemStorage_code39();

	resetVariableMemStorage_code93();

	gnDcdInitFlag = 0;
}
// 统计图像每列白色像素数
void BarcodeColumnScan( unsigned char * bina, int width, int height, int * column )
{
	int x = 0, y = 0;

	unsigned char * pBina = 0, * pBinaL = bina;
	int * pCol = 0;

	memset( column, 0, width * sizeof(int) );

	for( y = 0; y < height; y++ ) {
		pBina = pBinaL;
		pCol = column;
		for( x = 0; x < width; x++ ) {
			*pCol += (*pBina == 0xff);
			pBina++;
			pCol++;
		}
		pBinaL += width;
	}
}

// 对黑白区间进行分界
void BarcodeBlackWhitePartition( int * column, int width, int height, int * partition )
{
	int i = 0, sum = 0;
	int thresh = height * 3 / 2;

	int sum2 = 0, sum3 = 0, thresh2 = height, thresh3 = height * 3 / 2;

	int sign = 0, start = 0, end = 0;

	memset( partition, 0, width * sizeof(int) );

 	//for( i = 1; i < width-1; i++ ) {
 	//	sum = column[i-1] + column[i] + column[i+1];
 	//	if( sum > thresh ) {
 	//		partition[i] = 1;
 	//	} else if( sum == thresh ) {
 	//		partition[i] = 0;
 	//	} else {
 	//		partition[i] = -1;
 	//	}
 	//}

	// 改用新的分界策略
	for( i = 1; i < width-1; i++ ) {
		sum2 = column[i-1] + column[i];
		sum3 = sum2 + column[i+1];
		if(sum2 > thresh2 && sum3 > thresh3) {
			partition[i] = 1;
		} else if(sum2 < thresh2 && sum3 < thresh3) {
			partition[i] = -1;
		} else {
			partition[i] = 0;
		}
	}

	partition[0] = partition[1];
	partition[width-1] = partition[width-2];
}


int FindPartitionStartStop( int * partition, int width, int height, RyuPoint * startstop )
{
	int nRet = 0;
	int count = 0, i = 0, j = 0;

	RyuPoint ptForm, ptCurr, ptRear;
	int nForm = 0, nCurr = 0, nRear = 0, nExtd = 0;

	int nSign = partition[0];

	// 第一次写入
	startstop[0].x = startstop[0].y = 0;
	for( i = 1; i < width; i++ ) {
		if( partition[i] != nSign) {
			startstop[count++].y = i - 1;
			startstop[count].x = i;
			nSign = partition[i];
		}
	}
	startstop[count++].y = width - 1;

	if( count < 7 ) {
		nRet = 0;
		goto nExit;
	}

	
	// 过滤筛选
	// 首元素处理
	ptCurr = startstop[0];
	nCurr = (ptCurr.x + ptCurr.y) >> 1;
	ptRear = startstop[1];
	nRear = (ptRear.x + ptRear.y) >> 1;
	if( 0 == partition[nCurr] ) {	// 0区直接忽略
		startstop[0].x = startstop[0].y = 0;
	} else {
		nExtd = ( nRear == ptRear.x) ? 1 : 2;
		//////////////////////////////////////////////////////////////////////////
		// 新加入规则
		if( 0 == partition[nRear] ) {
			nExtd = ptRear.y - startstop[0].y;
		} else {
			nExtd = ( nRear == ptRear.x ) ? 1 : 2;
		}
		//////////////////////////////////////////////////////////////////////////
		startstop[0].y += nExtd;
		startstop[0].y *= partition[nCurr];
	}
	ptForm = ptCurr;
	nForm = nCurr;
	ptCurr = ptRear;
	nCurr = nRear;
	// 中间元素
	for( i = 1; i < count - 1; i++ ) {
		ptRear = startstop[i+1];
		nRear = (ptRear.x + ptRear.y) >> 1;
		//  中间元素为0区
		if( 0 == partition[nCurr] ) {
			// 前后元素符号相同，则置该区域为相反符号
			if( partition[nForm] == partition[nRear] ) {
				for( j = ptCurr.x; j <= ptCurr.y; j++ ) {
					partition[j] = -partition[nForm];
				}
				nExtd = ( nForm == ptForm.x) ? 1 : 2;
				startstop[i].x -= nExtd;
				startstop[i].x *= partition[nCurr];
				nExtd = ( nRear == ptRear.x) ? 1 : 2;
				startstop[i].y += nExtd;
				startstop[i].y *= partition[nCurr];
			} else {
				startstop[i].x = startstop[i].y = 0;
			}
		} 
		// 中间元素非0区
		else {
			nExtd = ( nForm == ptForm.x) ? 1 : 2;
			//////////////////////////////////////////////////////////////////////////
			// 新加入规则
			if( 0 == partition[nForm] ) {
				nExtd = startstop[i].x - ptForm.x;
			} else {
				nExtd = ( nForm == ptForm.x ) ? 1 : 2;
			}
			//////////////////////////////////////////////////////////////////////////
			startstop[i].x -= nExtd;
			startstop[i].x *= partition[nCurr];
			nExtd = ( nRear == ptRear.x) ? 1 : 2;
			//////////////////////////////////////////////////////////////////////////
			// 新加入规则
			if( 0 == partition[nRear] ) {
				nExtd = ptRear.y - startstop[i].y;
			} else {
				nExtd = ( nRear == ptRear.x ) ? 1 : 2;
			}
			//////////////////////////////////////////////////////////////////////////
			startstop[i].y += nExtd;
			startstop[i].y *= partition[nCurr];
		}
		ptForm = ptCurr;
		nForm = nCurr;
		ptCurr = ptRear;
		nCurr = nRear;
	}
	// 尾元素处理
	if( 0 == partition[nCurr] ) {	// 0区直接忽略
		startstop[count-1].x = startstop[count-1].y = 0;
	} else {
		nExtd = ( nForm == ptForm.x) ? 1 : 2;
		//////////////////////////////////////////////////////////////////////////
		// 新加入规则
		if( 0 == partition[nForm] ) {
			nExtd = startstop[count-1].x - ptForm.x;
		} else {
			nExtd = ( nForm == ptForm.x ) ? 1 : 2;
		}
		//////////////////////////////////////////////////////////////////////////
		startstop[count-1].x -= nExtd;
		startstop[count-1].x *= partition[nCurr];
		startstop[count-1].y *= partition[nCurr];
	}

	
	// 整理，去除无效区域
	for( i = 0; i < count; i++ ) {
		if( 0 == startstop[i].x && 0 == startstop[i].y ) {
			for( j = i; j < count-1; j++ ) {
				startstop[j] = startstop[j+1];
			}
			count--;
			i--;
		}
	}

	if( count < 7 ) {
		nRet = 0;
		goto nExit;
	}

	// 校验符号正确性
	nSign = (startstop[0].y > 0) ? 1 : -1;
	for( i = 1; i < count; i++ ) {
		nForm = (startstop[i].x > 0) ? 1 : -1;
		nRear = (startstop[i].y > 0) ? 1 : -1;
		if( nForm != nRear || nForm == nSign) {
#ifdef	_PRINT_PROMPT
			printf("Warning! Unexpected sign of startstop in FindPartitionStartStop, \
				nForm=%d, nSign=%d, nRear=%d, exit\n", nForm, nSign, nRear);
#endif
			nRet = 0;
			goto nExit;
		}
		nSign = nForm;
	}
	

	nRet = count;

nExit:
	return nRet;
}


// 统计黑白各区间像素点数量，形成解码数组，并记录位置
void DecodeArrayAnalysis( int * column, int width, int height, RyuPoint * startstop, int count, int * decode_arr)
{
	int nRet = 0;
	int i = 0, j = 0;

	int nSum = 0, nTotal = 0;

	int nState = 0, nStart = 0, nStop = 0;

	for( i = 0; i < count; i++ ) {
		nState = (startstop[i].y > 0) ? 1 : 0;
		nStart = nState ? startstop[i].x : (-startstop[i].x);
		nStop = nState ? startstop[i].y : (-startstop[i].y);
		nTotal = (nStop - nStart + 1) * height;
		nSum = 0;
		for( j = nStart; j <= nStop; j++) {
			nSum += column[j];
		}
		decode_arr[i] = nState ? nSum : (nSum - nTotal);
	}
}


void resetDecodeResultCandidates()
{
	gnDRNCandCnt = 0;
}

int pushDecodeResult2Candidates(DecodeResultNode result)
{
	int ret_val = 0;
	int i = 0;

	DecodeResultNode * pCandArr = gpDRNCandArr;
	int nIsMatch = 0, nMatchIdx = 0;

	const int gnDRNCandMaxCnt = 32;

	// 逐点匹配结果，匹配后更新参数
	for(i = 0; i < gnDRNCandCnt; i++) {
		if(0 == strcmp(result.strCodeData, pCandArr[i].strCodeData)
			&& result.code_type == pCandArr[i].code_type) {
			pCandArr[i].left_idx = (result.left_idx < pCandArr[i].left_idx) 
				? result.left_idx : pCandArr[i].left_idx;
			pCandArr[i].right_idx = (result.right_idx > pCandArr[i].right_idx) 
				? result.right_idx : pCandArr[i].right_idx;
			pCandArr[i].code_unit_r = (result.code_unit_r > pCandArr[i].code_unit_r) 
				? result.code_unit_r : pCandArr[i].code_unit_r;
			pCandArr[i].reliability = pCandArr[i].reliability + result.code_unit_r * 1.0 / result.code_unit;
			pCandArr[i].count++;
			nIsMatch = 1;
			nMatchIdx = i;
			break;
		}
	}

	// 若有匹配节点，则返回该节点累计次数
	if(nIsMatch) {
		ret_val = pCandArr[nMatchIdx].count;
	} 
	// 若无匹配项，则写入新节点
	else if(gnDRNCandCnt < gnDRNCandMaxCnt) {
		pCandArr[gnDRNCandCnt] = result;
		pCandArr[gnDRNCandCnt].reliability = result.code_unit_r * 1.0 / result.code_unit;
		pCandArr[gnDRNCandCnt].count = 1;
		gnDRNCandCnt++;
		ret_val = 1;
	}

	return ret_val;
}

int acquireDecodeResults(DecodeResultNode * result)
{
	int ret_val = 0;
	int i = 0, j = 0;

	DecodeResultNode * pCandArr = gpDRNCandArr;
	DecodeResultNode tmpNode;

	// 根据累计次数、信度排序
	for(i = 0; i < gnDRNCandCnt; i++) {
		for(j = i + 1; j < gnDRNCandCnt; j++) {
			if(pCandArr[i].count < pCandArr[j].count) {
				tmpNode = pCandArr[j];
				pCandArr[j] = pCandArr[i];
				pCandArr[i] = tmpNode;
			} else if(pCandArr[i].count == pCandArr[j].count
				&& pCandArr[i].reliability < pCandArr[j].reliability) {
				tmpNode = pCandArr[j];
				pCandArr[j] = pCandArr[i];
				pCandArr[i] = tmpNode;
			}
		}
	}

#ifdef _DEBUG_
#ifdef _DEBUG_DECODE
	// 依次验证是否满足输出条件
	if(0 < gnDRNCandCnt)
		printf("+++++++++++++++++++++++++++\n");
	for(i = 0; i < gnDRNCandCnt; i++) {
		// 满足条码类型限制、合法字符限制以及字符位数限制
		printf("条码%d: %s, cnt=%d, relia=%.3f\n", 
			i, pCandArr[i].strCodeData, pCandArr[i].count, pCandArr[i].reliability);
	}
	if(0 < gnDRNCandCnt)
		printf("+++++++++++++++++++++++++++\n");
#endif
#endif

	if(0 < gnDRNCandCnt) {
		memcpy(result, &pCandArr[0], sizeof(DecodeResultNode));
		result->flag = 1;
		ret_val = 1;
	} else {
		memset(result, 0, sizeof(DecodeResultNode));
		ret_val = 0;
	}

	return ret_val;
}


int BarcodeDigitNumCheck(char * strCode, int cntCodeChar, int setDigit)
{
	int nRet = 0, i = 0;
	int digit = 0;

	if(0 == setDigit) {
		nRet = 1;
		goto nExit;
	}

	if(0 >= cntCodeChar || 32 < cntCodeChar) {
		nRet = 0;
		goto nExit;
	}

	if((1 << (cntCodeChar-1)) & setDigit)
		nRet = 1;
	else
		nRet = 0;

nExit:
	return nRet;
}

int CharValidityCheck(char codeChar, int setValid)
{
	if(0 == setValid)	return 1;

	if(0 == codeChar)	return 0;

	if(codeChar >= 48 && codeChar <= 57) {
		if(setValid & (1<<0))	return 1;
		else					return 0;
	} else if(codeChar >= 97 && codeChar <= 122) {
		if(setValid & (1<<1))	return 1;
		else					return 0;
	} else if(codeChar >= 65 && codeChar <= 90) {
		if(setValid & (1<<2))	return 1;
		else					return 0;
	} else if(codeChar == 32) {
		if(setValid & (1<<3))	return 1;
		else					return 0;
	} else if(codeChar == 33) {
		if(setValid & (1<<4))	return 1;
		else					return 0;
	} else if(codeChar == 34) {
		if(setValid & (1<<5))	return 1;
		else					return 0;
	} else if(codeChar == 35) {
		if(setValid & (1<<6))	return 1;
		else					return 0;
	} else if(codeChar == 36) {
		if(setValid & (1<<7))	return 1;
		else					return 0;
	} else if(codeChar == 37) {
		if(setValid & (1<<8))	return 1;
		else					return 0;
	} else if(codeChar == 38) {
		if(setValid & (1<<9))	return 1;
		else					return 0;
	} else if(codeChar == 39) {
		if(setValid & (1<<10))	return 1;
		else					return 0;
	} else if(codeChar == 40 || codeChar == 41) {
		if(setValid & (1<<11))	return 1;
		else					return 0;
	} else if(codeChar == 42) {
		if(setValid & (1<<12))	return 1;
		else					return 0;
	} else if(codeChar == 43) {
		if(setValid & (1<<13))	return 1;
		else					return 0;
	} else if(codeChar == 44) {
		if(setValid & (1<<14))	return 1;
		else					return 0;
	} else if(codeChar == 45) {
		if(setValid & (1<<15))	return 1;
		else					return 0;
	} else if(codeChar == 46) {
		if(setValid & (1<<16))	return 1;
		else					return 0;
	} else if(codeChar == 47) {
		if(setValid & (1<<17))	return 1;
		else					return 0;
	} else if(codeChar == 58) {
		if(setValid & (1<<18))	return 1;
		else					return 0;
	} else if(codeChar == 59) {
		if(setValid & (1<<19))	return 1;
		else					return 0;
	} else if(codeChar == 60 || codeChar == 62) {
		if(setValid & (1<<20))	return 1;
		else					return 0;
	} else if(codeChar == 61) {
		if(setValid & (1<<21))	return 1;
		else					return 0;
	} else if(codeChar == 63) {
		if(setValid & (1<<22))	return 1;
		else					return 0;
	} else if(codeChar == 64) {
		if(setValid & (1<<23))	return 1;
		else					return 0;
	} else if(codeChar == 91 || codeChar == 93) {
		if(setValid & (1<<24))	return 1;
		else					return 0;
	} else if(codeChar == 92) {
		if(setValid & (1<<25))	return 1;
		else					return 0;
	} else if(codeChar == 94) {
		if(setValid & (1<<26))	return 1;
		else					return 0;
	} else if(codeChar == 95) {
		if(setValid & (1<<27))	return 1;
		else					return 0;
	} else if(codeChar == 96) {
		if(setValid & (1<<28))	return 1;
		else					return 0;
	} else if(codeChar == 123 || codeChar == 125) {
		if(setValid & (1<<29))	return 1;
		else					return 0;
	} else if(codeChar == 124) {
		if(setValid & (1<<30))	return 1;
		else					return 0;
	} else if(codeChar == 126) {
		if(setValid & (1<<31))	return 1;
		else					return 0;
	} else
		return 0;
}

int BarcodeValidityCheck(char * strCode, int cntCodeChar, int setValid)
{
	int nRet = 0, i = 0, status = 0;

	if(0 == setValid) {
		nRet = 1;
		goto nExit;
	}

	for( i = 0; i < cntCodeChar; i++ ) {
		status = CharValidityCheck(strCode[i], setValid);
		if(1 != status) {
			nRet = 0;
			goto nExit;
		}
	}

	nRet = 1;

nExit:
	return nRet;
}

int BarcodeResultCheck(DecodeResultNode result, int setDigit, int setValid)
{
	int ret = 0;

	if(1 != result.flag) {
		ret = 0;
		goto nExit;
	}

	if(1 != BarcodeDigitNumCheck(result.strCodeData, result.rslt_digit, setDigit)) {
		ret = 0;
		goto nExit;
	}
	if(1 != BarcodeValidityCheck(result.strCodeData, result.rslt_digit, setValid)) {
		ret = 0;
		goto nExit;
	}

	ret = 1;

nExit:
	return ret;
}
