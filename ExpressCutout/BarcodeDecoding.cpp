#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "RyuCore.h"

#include "Decoder_code128.h"
#include "Decoder_code39.h"
#include "Decoder_code93.h"
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

int gnDcdInitFlag = 0;

void BarcodeColumnScan( unsigned char * bina, int width, int height, int * column );

void BarcodeBlackWhitePartition( int * column, int width, int height, int * partition );

int FindPartitionStartStop( int * partition, int width, int height, RyuPoint * startstop );

void DecodeArrayAnalysis( int * column, int width, int height, RyuPoint * startstop, int count, int * decode_arr);

//int BarcodeValidityCheck(char * strCode, int cntCodeChar);

int BarcodeValidityCheck(char * strCode, int cntCodeChar, int setValid);

int BarcodeDigitNumCheck(char * strCode, int cntCodeChar, int setDigit);


int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH, 
		int * code_type, int * char_num, int * char_valid, int * module_num,
		int * code_direct, int * leftOffset, int * rightOffset, char * code_result)
{
	int nRet = 0, i = 0, status = 0;
	int nIsCodeState = 0;

	int nColCount = 0;

	int slice_cnt = 0, slice_remain = 0, slice_offset = 0;
	int slice_top = 0, slice_bottom = 0, slice_height = 0;

	unsigned char * pBina = 0;

	char strResult[128];
	int setDigit = 0, setType = 0, setValid = 0;
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
	setType = *code_type;
	setDigit = *char_num;
	setValid = *char_valid;

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
			if((setType & 0x1) || 0 == setType) {
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode128(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 0x1;
				break;
			} 
			}

			if((setType & 0x2) || 0 == setType) {
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode39(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<1;
				break;
			} 
			}

			if((setType & 0x4) || 0 == setType) {
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCode93(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<2;
				break;
				}
			}

			if((setType & 0x8) || 0 == setType) {
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCodeI2of5(gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<3;
				break;
				}
			}

			if((setType & 0x10) || 0 == setType) {
			status = 0;
			memset( strResult, 0, sizeof(char) * 128 );		// 写入前置零是必要的!!!
			memcpy( gpnDcdDecodeArrProc, gpnDcdDecodeArr, nColCount * sizeof(int));		// 使用副本进行识别，保留原数组不变
			status = RecgCodeEAN13(gpnDcdDecodeArr, gpnDcdDecodeArrProc, nColCount, strResult, &nDigitCnt, &nModuleCnt, 
				&nDirection, &nLeftIdx, &nRightIdx);
			if( 1 == status ) {
				nCodeType = 1<<4;
				break;
				}
			}

			break;
		}

		if( 1 == status ) {
#if    0
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
			//if( 1 == BarcodeValidityCheck(strResult, nDigitCnt) )
			//	break;
			//else
			//	status = 0;
			if(1 != BarcodeDigitNumCheck(strResult, nDigitCnt, setDigit)) {
				status = 0;
				continue;
			}
			if(1 != BarcodeValidityCheck(strResult, nDigitCnt, setValid)) {
				status = 0;
				continue;
			}
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

int BarcodeDecoding_init( int max_width, int max_height )
{
	int nRet = 0;

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

/*
int BarcodeValidityCheck(char * strCode, int cntCodeChar)
{
	int nRet = 0, i = 0;

	for( i = 0; i < cntCodeChar; i++ ) {
		if( strCode[i] >= 48 && strCode[i] <= 57 )
			continue;
		else if( strCode[i] >= 65 && strCode[i] <= 90 )
			continue;
		else if( strCode[i] >= 97 && strCode[i] <= 122 )
			continue;
		else {
			nRet = -1;
			goto nExit;
		}
	}

	nRet = 1;

nExit:
	return nRet;
}
*/

int BarcodeDigitNumCheck(char * strCode, int cntCodeChar, int setDigit)
{
	int nRet = 0, i = 0;
	int digit = 0;

	if(0 == setDigit) {
		nRet = 1;
		goto nExit;
	}

	for( i = 0; i < 4; i++ ) {
		digit = (setDigit>>(i*8)) & 0xff;
		if(digit == cntCodeChar) {
			nRet = 1;
			break;
		}
	}

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

