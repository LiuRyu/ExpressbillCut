#ifndef _RYU_CORE_H
#define _RYU_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include <time.h>
#include "RyuType.h"

/************************************************************************/
/* ģ�鿪��                                                             */
/************************************************************************/
//#define _OCR				// �ַ�ʶ��ģ�鿪��
//#define _RECOG_CHAR_		// �ַ�ʶ��ģ�鿪��
#define _MULTI_PKG			// ��������ģ�鿪��
//#define _CUT_CHAR_			// �и����ֲ���ͼ��(��ʶ������)
//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* ��¼�㷨������־                                                      */
/************************************************************************/
//#define _PRINT_LOG	// ʵʱ��ӡ�㷨��־
#define _WRITE_LOG		// �㷨��־д���ļ�
#ifdef  _WRITE_LOG
#include "write_log.h"
#endif

//#define _LOG_TRACE
#define _LOG_COSTTIME
//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* ���Կ���                                                             */
/************************************************************************/
// �����ܿ���
//#define _DEBUG_

// ��ģ����Կ���
//#define _DEBUG_MAIN		// ������ģ��
//#define _DEBUG_LOCATE		// ��λģ��
//#define _DEBUG_SEGMENT	// �и�ģ��
//#define _DEBUG_ROTATE		// ��תģ��
//#define _DEBUG_IMGPROC	// ͼ����ģ��
//#define _DEBUG_DECODE		// ����ģ��
//#define _DEBUG_DECODER_CODE128
//#define _DEBUG_DECODER_CODE39
//#define _DEBUG_DECODER_CODE93
//#define _DEBUG_DECODER_I2OF5
//#define _DEBUG_DECODER_EAN13
//#define _DEBUG_FLOODFILL

//#define _DEBUG_EXBILL
//#define _DEBUG_EXBILL_TIME
//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* ���Կ���                                                             */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////


#define TRIGONOMETRIC_SHIFT_DIGIT		(10)
#define FLOAT2FIXED_SHIFT_DIGIT			(10)
#define CODE_RESULT_ARR_LENGTH			(128)
#define CODE_IMAGE_MIN_WIDTH			(36)

#ifndef RYUMAX
#define RYUMAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef RYUMIN
#define RYUMIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

const int cnRyuAtanLUTHalf[90] = {
	9, 27, 45, 63, 81, 99, 117, 135, 153, 171, 190, 208, 227, 246, 265, 284, 303, 
	323, 343, 363, 383, 403, 424, 445, 467, 488, 511, 533, 556, 579, 603, 628, 652, 
	678, 704, 730, 758, 786, 815, 844, 875, 906, 938, 972, 1006, 1042, 1079, 1117, 
	1157, 1199, 1242, 1287, 1335, 1384, 1436, 1490, 1547, 1607, 1671, 1738, 1810, 
	1886, 1967, 2054, 2147, 2247, 2355, 2472, 2600, 2739, 2892, 3060, 3248, 3457, 
	3692, 3960, 4265, 4619, 5033, 5525, 6119, 6852, 7778, 8988, 10635, 13011, 16742, 
	23453, 39105, 117339
};

int	ryuSinShift(int angel);

int	ryuCosShift(int angel);

int	ryuArctan180Shift(int dy, int dx);

int ryuCycle(int a, int range);

int	ryuCycleDistance(int a, int b, int range);

RyuPoint ryuDivideIntPoint(int point);

int ryuDistanceBtPoints(RyuPoint pt1, RyuPoint pt2);

void ryuMakeGaussianKernal();


RyuROI * ryuCreateROI(int xOffset, int yOffset, int width, int height);

void ryuSetImageROI(RyuImage* image, RyuRect rect);

RyuRect ryuGetImageROI(const RyuImage * image);

void ryuResetImageROI(RyuImage * image);

RyuImage * ryuCreateImageHeader(RyuSize size, int depth, int channels);

void * ryuInitImageHeader(RyuImage * image, RyuSize size, int depth, int channels);

RyuImage * ryuCreateImage(RyuSize size, int depth, int channels);

void ryuReleaseImageHeader( RyuImage ** image );

void ryuReleaseImage( RyuImage ** image );

void * ryuSetImage( RyuImage * image, RyuSize size, int depth, int channels );

void ryuZero( RyuImage * image );

int ryuGetPixel(RyuImage * image, RyuPoint pt);

int ryuSetPixel(RyuImage * image, RyuPoint pt, unsigned char val);

int ryuResizeImage(RyuImage * img_in, RyuImage * img_out);

int ryuDilate(RyuImage * src, RyuImage * dst);

#endif
