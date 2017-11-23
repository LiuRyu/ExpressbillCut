// ExpressBarcodeDetection.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RyuCore.h"
#include "BarcodeLocation.h"
#include "BarcodeSegmentation.h"
#include "ImageRotation.h"
#include "BarcodeImageproc.h"
#include "BarcodeDecoding.h"
#include "WaybillSegmentation.h"
#include "ExpressBarcodeDetection.h"

#ifdef _OCR
// TODO: 插入字符识别头文件引用
#include "LocateCodeNumber.h"
#endif

#ifdef  _DEBUG_
#include "OpenCv_debugTools.h"
#endif

#ifdef	_PRINT_TIME
#define _PRINT_TIME_MAIN
#ifdef	_PRINT_TIME_MAIN
#include "OpenCv_debugTools.h"
#include <time.h>
#endif
#endif
#include "PerspectiveTrans.h"

#define BARCODE_RESULT_MAX_COUNT	(128)
#define LOCATE_SCAN_WINDOW_SIZE		(32)
#define OCR_INTERCEPT_EXTEND		(36) 

#define DECODE_AVGMIN_MODULEW_THRESHOLD	(1.8)
#define DECODE_IM_WIDTH_THRESHOLD		(400)

using namespace std;

int gnImageMaxWidth = 0, gnImageMaxHeight = 0, gnImageMaxSize = 0;

unsigned char * gpucBarcodeResults = 0;

int gnExprBrcdDtctInitFlag = 0;

/************************************************************************/
/* 设置参数										*/
/************************************************************************/
int gnCodeSymbology = 0;
int gnRglCodeCnt = 0;
int gnCodeDgtNum = 0;
int gnCodeDgtNumExt = 0;
int gnCodeValidity = 0;
int gnCodeValidityExt = 0;


/************************************************************************/
/* 运行识别																*/
/* 文件编号01，函数编号01													*/
/************************************************************************/
int algorithm_run(int lrning_flag, unsigned char * in_data, int width, int height, unsigned char ** results)
{
	// 错误号
	const int error_ret = -1010100;

	int i = 0, j = 0, k = 0, status = 0;
	int ret_val = -1000000;

	// 结果
	AlgorithmResult result_node;
	int code_count = 0;
	int * first_4char = (int *)gpucBarcodeResults;

	// 定位
	LocateClusArea * pLocateClus = 0;
	int nLocateRet = 0;

	// 切割
	SegmentBarcodeArea * pSegmentBarcode = 0;
	int nSegmentCnt = 0;
	SegmentBarcodeArea tmpSegment;
	int nTmp1 = 0, nTmp2 = 0;

	// 旋转
	int rtt_width = 0, rtt_height = 0;
	unsigned char * ucBarcodeImage = 0;
	unsigned char * ucDecodeImage = 0;
	
	// 读码
	int sliceH = 0, nDecodeFlag = 0, sliceHs[32] = {0}, sliceCnt = 0;
	int codeDigit = 0, codeType = 0, codeValid = 0, codeDirect = 0, codeModule = 0;
	char code_result[CODE_RESULT_ARR_LENGTH];
	int codeOffsetL = 0, codeOffsetR = 0;
	float minModuleW = 0.0, accModuleW = 0.0;

	// 临时变量
	RyuPoint ptCenter;
	int nTmp = 0, nLctClusMaxGrad = 0;

	// OCR字符识别
	int IsNumberRecg = 0;
	RyuPoint ptIncptOCR, ptOncptOCR, ptCornerOCR[4];

	// 包裹检测
	RyuPoint pkgCentre = ryuPoint(-1, -1);
	RyuRect  pkgBoundBox = ryuRect(-1, -1, 0, 0);

#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_MAIN
	long TimeCost = 0;
	LARGE_INTEGER m_frequency = {0}, m_time1 = {0}, m_time2 = {0};
	int support = QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_time2);
#endif
#endif

	if(1 != gnExprBrcdDtctInitFlag) {
#ifdef	_PRINT_PROMPT
		printf("Error! Algorithm runs without initialization, DO init before run. --algorithm_run\n");
#endif
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. gnExprBrcdDtctInitFlag=%d", ret_val, gnExprBrcdDtctInitFlag);
#endif
		return error_ret-1;
	}

	memset(gpucBarcodeResults, 0, sizeof(int) + BARCODE_RESULT_MAX_COUNT * sizeof(AlgorithmResult));
	memset(&result_node, 0, sizeof(AlgorithmResult));
	memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);
	*results = 0;

	if(!in_data) {
#ifdef	_PRINT_PROMPT
		printf("Error! Invalid input of in-data, in_data=0x%x. --algorithm_run\n", in_data);
#endif
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. in_data=0x%x", ret_val, in_data);
#endif
		code_count = ret_val = error_ret - 2;
		goto nExit;
	}

	WaybillSegm_resetCodeAreaStack();

	// 条码区域定位
	nLocateRet = LocateBarcodeAreas(in_data, width, height, LOCATE_SCAN_WINDOW_SIZE);

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- LocateBarcodeAreas proc done. ret=%d", nLocateRet);
#endif
#endif

	pLocateClus = getLocateFeatureAreas();
	if(0 == nLocateRet) {
		// 未找到条码区域定位，判定为图像中无条码，返回nofind标识
		code_count = ret_val = -2;
		goto nExit;
	} else if(nLocateRet < 0 || !pLocateClus) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of LocateBarcodeAreas, nLocateRet=%d, pLocateClus=0x%x. --algorithm_run\n",
				nLocateRet, pLocateClus);
#endif
		code_count = ret_val = nLocateRet;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. nLocateRet=%d", ret_val, nLocateRet);
#endif
		goto nExit;
	}

	// 于定位区域检测并分割条形码
	pSegmentBarcode = GetSegmentBarcodeAreaPtr();

	for(i = 1; i <= nLocateRet; i++) {
		if(nSegmentCnt >= SGM_BARCODE_AREA_MAX_COUNT)
			break;

#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_MAIN
		TimeCost = 0;
		QueryPerformanceFrequency(&m_frequency);
		QueryPerformanceCounter(&m_time2);
#endif
#endif

		// 条码分割
		status = SegmentBarcode(in_data, width, height, pLocateClus[i].corner, (RyuPoint*)&pLocateClus[i].min_intcpt, 
			(RyuPoint*)&pLocateClus[i].min_ontcpt, &pLocateClus[i].angle, pLocateClus[i].grad, &pSegmentBarcode[nSegmentCnt], nSegmentCnt);

#if 0
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "TRACE- %dth SegmentBarcode proc done. ret=%d", i, status);
#endif
#endif
#endif

#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_MAIN
		QueryPerformanceCounter((LARGE_INTEGER*) &m_time1);
		TimeCost = 1000.0*1000.0*(m_time1.QuadPart-m_time2.QuadPart)/m_frequency.QuadPart;
		printf("\n-SegmentBarcode 耗时: %ldus\n", TimeCost);
#endif
#endif
		if( status < 0 ) {
#ifdef	_PRINT_PROMPT
			printf( "Warning! Unexpected return of SegmentBarcode, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
#ifdef _WRITE_LOG
			Write_Log(LOG_TYPE_ERROR, "错误号: %d. seq=%d", status, i);
#endif
			continue;
		} else {
			for(j = nSegmentCnt; j < nSegmentCnt + status; j++) {
				pSegmentBarcode[j].index = i;
			}
			nSegmentCnt += status;
			nLctClusMaxGrad = (pLocateClus[i].grad > nLctClusMaxGrad) ? pLocateClus[i].grad : nLctClusMaxGrad;
		}
	}

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- All SegmentBarcode proc done. nSegmentCnt=%d", nSegmentCnt);
#endif
#endif

	// 没有找到条形码
	if(nSegmentCnt == 0) {
		// 无有效分割，判定为图像中无条码，返回nofind标识
		code_count = ret_val = -2;
		goto nExit;
	} else if(nSegmentCnt < 0) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected value of nSegmentCnt, nSegmentCnt=%d. --algorithm_run\n", nSegmentCnt);
#endif
		code_count = ret_val = error_ret - 4;
#ifdef _WRITE_LOG
		Write_Log(LOG_TYPE_ERROR, "错误号: %d. nSegmentCnt=%d", ret_val, nSegmentCnt);
#endif
		goto nExit;
	}

// 	// 对条码区域进行排序（得分-面积）
// 	for(i = 0; i < nSegmentCnt; i++) {
// 		for(j = i+1; j < nSegmentCnt; j++) {
// 			nTmp1 = (pSegmentBarcode[i].max_ontcpt-pSegmentBarcode[i].min_ontcpt) 
// 				* (pSegmentBarcode[i].max_intcpt-pSegmentBarcode[i].min_intcpt);
// 			nTmp1 *= pSegmentBarcode[i].score;
// 			nTmp2 = (pSegmentBarcode[j].max_ontcpt-pSegmentBarcode[j].min_ontcpt) 
// 				* (pSegmentBarcode[j].max_intcpt-pSegmentBarcode[j].min_intcpt);
// 			nTmp2 *= pSegmentBarcode[j].score;
// 			if(nTmp1 < nTmp2) {
// 				tmpSegment = pSegmentBarcode[i];
// 				pSegmentBarcode[i] = pSegmentBarcode[j];
// 				pSegmentBarcode[j] = tmpSegment;
// 			}
// 		}
// 	}

	// 依次对分割出来的条形码区域图像进行缩放、旋转和识别
	for(i = 0; i < nSegmentCnt; i++) {
		// 对于每个分割域，最多尝试识别两次，第一次为原图大小，第二次放大
		for(j = 0; j < 2; j++) {
			// 根据标志flag对条码区域图像进行缩放、旋转矫正
			status = RotateImage(in_data, width, height, pSegmentBarcode[i].corner_ext, pSegmentBarcode[i].angle, 
				j+1, (short *) &rtt_width, (short *) &rtt_height);

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth RotateImage proc done. ret=%d", i, status);
#endif
#endif
		// 这里需要加入对于宽度的限制
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of RotateImage, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
#ifdef _WRITE_LOG
				Write_Log(LOG_TYPE_ERROR, "错误号: %d. RotateImage seq=%d", status, i);
#endif
				continue;
			} else if(rtt_width <= 5 || rtt_height <= 5) {
				continue;
			}

			ucBarcodeImage = GetRotateImage();
			if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of GetRotateImage, slice%d ucBarcodeImage=0x%x. --algorithm_run\n", i, ucBarcodeImage );
#endif
#ifdef _WRITE_LOG
				Write_Log(LOG_TYPE_ERROR, "错误号: -11809001. RotateImage seq=%d, ret=%d, ucBarcodeImage=0x%x", 
					status, i, ucBarcodeImage);
#endif
				continue;
			}

			// 条码区域矫正图像预处理(调整对比度、锐化、二值化)
			status = BarcodeImgProcessIntegrogram(ucBarcodeImage, rtt_width, rtt_height);

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth BarcodeImgProcess proc done. ret=%d", i, status);
#endif
#endif
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of BarcodeImageProcessing, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
#ifdef _WRITE_LOG
				Write_Log(LOG_TYPE_ERROR, "错误号: %d. BarcodeImgProcess seq=%d", status, i);
#endif
				continue;
			}

			ucDecodeImage = getBarcodeImgProcOutput();

			memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);
			codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
			codeType = gnCodeSymbology;
			codeDigit = gnCodeDgtNum;
			codeValid = gnCodeValidity;
			minModuleW = 2.0;
			nDecodeFlag = BarcodeDecoding_run(ucBarcodeImage, (int *)ucDecodeImage, rtt_width, rtt_height, code_result, 
				&codeType, &codeDigit, &codeValid, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR, &minModuleW);

			if(1 == nDecodeFlag) {
				codeOffsetL /= (j+1);
				codeOffsetR /= (j+1);
				break;
			}
			else if(1 != nDecodeFlag 
				&& minModuleW < DECODE_AVGMIN_MODULEW_THRESHOLD 
				&& rtt_width < DECODE_IM_WIDTH_THRESHOLD) {
				continue;
			} else {
				break;
			}
		}

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth DecodeBarcode proc done. nDecodeFlag=%d", i, nDecodeFlag);
#endif
#endif

#ifdef _CUT_CHAR_
		//////////////////////////////////////////////////////////////////////////
		// 字符识别图像截取
		while(1 == nDecodeFlag) {
			// Step1
			int char_angle = (codeDirect > 0) ? pSegmentBarcode[i].angle : pSegmentBarcode[i].angle+180;

			if(char_angle < 180) {
				ptIncptOCR.x = pSegmentBarcode[i].max_intcpt;
				ptIncptOCR.y = pSegmentBarcode[i].max_intcpt + OCR_INTERCEPT_EXTEND;
				ptOncptOCR.x = pSegmentBarcode[i].min_ontcpt;
				ptOncptOCR.y = pSegmentBarcode[i].max_ontcpt;
				InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, char_angle, ptCornerOCR);
				status = RotateImage(in_data, width, height, ptCornerOCR, char_angle, 
					1, (short *) &rtt_width, (short *) &rtt_height);
				if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
					printf( "Warning! Unexpected return of RotateImageOCR, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
					break;
				}
				ucBarcodeImage = GetRotateImage();
				if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
					printf( "Error! Unexpected return of GetRotateImageOCR, slice%d ucBarcodeImage=0x%x. --algorithm_run\n", i, ucBarcodeImage );
#endif
					break;
				}
			} else {
				ptIncptOCR.x = pSegmentBarcode[i].min_intcpt - OCR_INTERCEPT_EXTEND;
				ptIncptOCR.y = pSegmentBarcode[i].min_intcpt;
				ptOncptOCR.x = pSegmentBarcode[i].min_ontcpt;
				ptOncptOCR.y = pSegmentBarcode[i].max_ontcpt;
				InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, char_angle-180, ptCornerOCR);
				status = RotateImage(in_data, width, height, ptCornerOCR, char_angle-180, 
					1, (short *) &rtt_width, (short *) &rtt_height);
				if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
					printf( "Warning! Unexpected return of RotateImageOCR, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
					break;
				}
				ucBarcodeImage = GetRotateImage();
				if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
					printf( "Error! Unexpected return of GetRotateImageOCR, slice%d ucBarcodeImage=0x%x. --algorithm_run\n", i, ucBarcodeImage );
#endif
					break;
				}
				int rtt_size = rtt_width * rtt_height;
				for(j = 0; j < (rtt_size>>1); j++) {
					unsigned char ucTmp = ucBarcodeImage[j];
					ucBarcodeImage[j] = ucBarcodeImage[rtt_size-j-1];
					ucBarcodeImage[rtt_size-j-1] = ucTmp;
				}
			}

			printf("codeDirect=%d, pSegmentBarcode[i].angle=%d, char_angle=%d\n", codeDirect, pSegmentBarcode[i].angle, char_angle);
			IplImage * dbgCutCharIm = cvCreateImage(cvSize(rtt_width, rtt_height), 8, 1);
			uc2IplImageGray(ucBarcodeImage, dbgCutCharIm);
			cvNamedWindow("dbgCutCharIm");
			cvShowImage("dbgCutCharIm", dbgCutCharIm);
			cvWaitKey();
			cvReleaseImage(&dbgCutCharIm);

			break;
		}

		// OCR字符识别模块 END
		//////////////////////////////////////////////////////////////////////////
#endif _CUT_CHAR_

		if( 1 == nDecodeFlag ) {
			UpdateCodeCorner(&pSegmentBarcode[i], codeOffsetL, codeOffsetR);
			memcpy(result_node.strCodeData, code_result, CODE_RESULT_ARR_LENGTH);
			if(IsNumberRecg == 1)
				result_node.nFlag = 2;
			else
				result_node.nFlag = 1;
			result_node.nCodeSymbology = codeType;
			result_node.nCodeCharNum = codeDigit;
			result_node.nCodeOrient = (codeDirect > 0) ? pSegmentBarcode[i].angle : (pSegmentBarcode[i].angle + 180);
			result_node.nCodeWidth = abs(pSegmentBarcode[i].max_ontcpt - pSegmentBarcode[i].min_ontcpt + 1);
			result_node.nCodeHeight = abs(pSegmentBarcode[i].max_intcpt - pSegmentBarcode[i].min_intcpt + 1);
			result_node.nCodeModuleWid = result_node.nCodeWidth * 1000 / (codeModule+1);	// 分母加一避免除数为0
			result_node.ptCodeBound1 = (pSegmentBarcode[i].corner[0].x << 16) | (pSegmentBarcode[i].corner[0].y & 0xffff);
			result_node.ptCodeBound2 = (pSegmentBarcode[i].corner[1].x << 16) | (pSegmentBarcode[i].corner[1].y & 0xffff);
			result_node.ptCodeBound3 = (pSegmentBarcode[i].corner[2].x << 16) | (pSegmentBarcode[i].corner[2].y & 0xffff);
			result_node.ptCodeBound4 = (pSegmentBarcode[i].corner[3].x << 16) | (pSegmentBarcode[i].corner[3].y & 0xffff);
			ptCenter.x = ptCenter.y = 0;
			for( j = 0; j < 4; j++) {
				ptCenter.x += pSegmentBarcode[i].corner[j].x;
				ptCenter.y += pSegmentBarcode[i].corner[j].y;
			}
			ptCenter.x >>= 2;
			ptCenter.y >>= 2;
			result_node.ptCodeCenter = (ptCenter.x << 16) | (ptCenter.y & 0xffff);
			result_node.nCodeSeqNum = code_count + 1;
			memcpy(gpucBarcodeResults+code_count*sizeof(AlgorithmResult)+4, &result_node, sizeof(AlgorithmResult));
			code_count++;

			WaybillSegm_pushCodeAreaStack(pSegmentBarcode[i].corner, pSegmentBarcode[i].angle, nDecodeFlag, result_node.strCodeData);

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth CodeResults writing done.", i);
#endif
#endif
		}
		if(code_count >= gnRglCodeCnt && gnRglCodeCnt > 0)
			break;
	}

	if(0 >= code_count) {
		status = WaybillSegment(in_data, width, height, &pkgBoundBox);
		if(0 >= status) {
			ret_val = code_count = -2;
		}
		// 若无法识别，则给出最大可能条码的位置和形状信息，以供切割算法参考
		else if(1 == pSegmentBarcode[0].index) {
			result_node.nCodeOrient = pSegmentBarcode[0].angle;
			result_node.nCodeWidth = abs(pSegmentBarcode[0].max_ontcpt - pSegmentBarcode[0].min_ontcpt + 1);
			result_node.nCodeHeight = abs(pSegmentBarcode[0].max_intcpt - pSegmentBarcode[0].min_intcpt + 1);
			result_node.ptCodeBound1 = (pSegmentBarcode[0].corner[0].x << 16) | (pSegmentBarcode[0].corner[0].y & 0xffff);
			result_node.ptCodeBound2 = (pSegmentBarcode[0].corner[1].x << 16) | (pSegmentBarcode[0].corner[1].y & 0xffff);
			result_node.ptCodeBound3 = (pSegmentBarcode[0].corner[2].x << 16) | (pSegmentBarcode[0].corner[2].y & 0xffff);
			result_node.ptCodeBound4 = (pSegmentBarcode[0].corner[3].x << 16) | (pSegmentBarcode[0].corner[3].y & 0xffff);
			memcpy(gpucBarcodeResults+4, &result_node, sizeof(AlgorithmResult));
			code_count = ret_val = 0;
		} 
		// 无法识别，无法给出切割区域
		else {
			code_count = ret_val = -1;
		}
	}

	ret_val = code_count;

nExit:
#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	if(0 == status)			printf("********Cannot find express package!\n");
	else if(1 == status)	printf("********The frame has only 1 express package!\n");
	else if(2 == status)	printf("********Warning, the frame MAY has multiple express packages!\n");
	else if(3 == status)	printf("********Warning, the frame has multiple express packages!\n");
	else					printf("********Error, WaybillSegment runs into some exceptions!\n");
#endif
#endif
	first_4char[0] = code_count;
	*results = gpucBarcodeResults;
	return ret_val;
}


/************************************************************************/
/* 算法初始化函数														*/
/************************************************************************/
int algorithm_init(int max_width, int max_height, unsigned char ** results)
{
	// 错误号
	const int error_ret = -1010200;

	int i = 0, j = 0;
	int status= 0, ret_val = 0;

	AlgorithmResult result_node;
	int * first_4char = 0;

	if(gnExprBrcdDtctInitFlag) {
#ifdef	_PRINT_PROMPT
		printf("Error! Algorithm is already initialized, DO NOT init again. --algorithm_init\n");
#endif
		*results = 0;
		ret_val = error_ret - 1;
		return ret_val;
	}

	if(max_width <= 0 || max_height <= 0) {
#ifdef	_PRINT_PROMPT
		printf("Error! Invalid input of width/height, max_width=%d, max_height=%d. --algorithm_init\n", max_width, max_height);
#endif
		ret_val = error_ret - 2;
		goto nExit;
	}

	gnImageMaxWidth = max_width;
	gnImageMaxHeight = max_height;
	gnImageMaxSize = max_width * max_height;

	gpucBarcodeResults = (unsigned char *) malloc(sizeof(int) + BARCODE_RESULT_MAX_COUNT * sizeof(AlgorithmResult));
	if(!gpucBarcodeResults) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Memory allocation failed, gpucBarcodeResults=0x%x. --algorithm_init\n");
#endif
		return error_ret - 3;
	}
	memset(gpucBarcodeResults, 0, sizeof(int) + BARCODE_RESULT_MAX_COUNT * sizeof(AlgorithmResult));

	first_4char = (int *)gpucBarcodeResults;
	first_4char[0] = 0;
	memset(&result_node, 0, sizeof(AlgorithmResult));

	status = BarcodeLocation_init(max_width, max_height);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of BarcodeLocation_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}

	status = BarcodeSegmentation_init(max_width, max_height, 128000);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of BarcodeSegmentation_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}

	status = ImageRotation_init(max_width, max_height);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of ImageRotation_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}

	status = BarcodeImageproc_init(max_width, max_height);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of BarcodeImageproc_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}

	status = BarcodeDecoding_init(max_width, max_height);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of BarcodeDecoding_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}
#ifdef _OCR
	status = LocateCodeNumber_init(max_width, max_height);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of LocateCodeNumber_init, ret_val=%d. --algorithm_init\n", status);
#endif
		ret_val = status;
		goto nExit;
	}
#endif

#ifdef _RECOG_CHAR_
	status = Init_RecogChar();
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of Init_RecogChar, ret_val=%d. --Init_RecogChar\n", status);
#endif
		ret_val = status;
		goto nExit;
	}
#endif _RECOG_CHAR_

	status = ryuInitFloodFill(ryuSize(400, 400), 1024);

	// 预设参数，切割现场用
	gnCodeSymbology = (1 << 0) | (1 << 1);
	gnRglCodeCnt = 1;
	gnCodeDgtNum = (1 << 11) | (1 << 12) | (1 << 13);
	gnCodeDgtNumExt = 0;
	gnCodeValidity = (1 << 0);
	gnCodeValidityExt = 0;

	gnExprBrcdDtctInitFlag = 1;
	ret_val = 1;

nExit:
	result_node.nFlag = ret_val;
	first_4char[0] = 1;
	memcpy(gpucBarcodeResults+4, &result_node, sizeof(AlgorithmResult));

	*results = gpucBarcodeResults;

	return ret_val;
}


void algorithm_release()
{
	if(gpucBarcodeResults) {
		free(gpucBarcodeResults);
		gpucBarcodeResults = 0;
	}

	BarcodeLocation_release();

	BarcodeSegmentation_release();

	ImageRotation_release();

	BarcodeImageproc_release();

	BarcodeDecoding_release();

#ifdef _OCR
	LocateCodeNumber_release();
#endif

#ifdef _RECOG_CHAR_
	Release_RecogChar();
#endif _RECOG_CHAR_

	ryuReleaseFloodFill();

	gnExprBrcdDtctInitFlag = 0;
}


int algorithm_setparams(AlgorithmParamSet * paramset)
{
	int ret_val = 0;

	if(!paramset) {
		ret_val = -10112011;
		printf("\n--Ryu--Invalid input parameters in Algorithm Getparams operation, %d\n", ret_val);
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "ERROR- Algorithm params setting failed[%d]. paramset=0x%x", ret_val, paramset);
#endif
		Write_Log(LOG_TYPE_ERROR, "ERROR- Algorithm params setting failed[%d]. paramset=0x%x", ret_val, paramset);
#endif
		goto nExit;
	}

	gnRglCodeCnt = paramset->nCodeCount;
	if(gnRglCodeCnt < 0 || gnRglCodeCnt > BARCODE_RESULT_MAX_COUNT) {
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "WARN- Invalid algorithm param: code_count=%d. Reset this param to default: 0", gnRglCodeCnt);
#endif
		Write_Log(LOG_TYPE_ERROR, "WARN- Invalid algorithm param: code_count=%d. Reset this param to default: 0", gnRglCodeCnt);
#endif
		gnRglCodeCnt = 0;	// 还原初始值
	}
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_count <== %d", gnRglCodeCnt);
#endif
#endif


	gnCodeSymbology = paramset->nCodeSymbology;
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_symbology <== 0x%x", gnCodeSymbology);
#endif
#endif

	gnCodeDgtNum = paramset->nCodeDgtNum;
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_digit <== 0x%x", gnCodeSymbology);
#endif
#endif

	gnCodeDgtNumExt = paramset->nCodeDgtNumExt;
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_digit <== 0x%x", gnCodeSymbology);
#endif
#endif

	gnCodeValidity = paramset->nCodeValidity;
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_validity <== 0x%x", gnCodeValidity);
#endif
#endif

	gnCodeValidityExt = paramset->nCodeValidityExt;
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "SET- Algorithm param setting. code_validityEx <== 0x%x", gnCodeValidityExt);
#endif
#endif

	ret_val = 1;

nExit:
	return ret_val;
}

void algorithm_resetparams()
{
	gnRglCodeCnt = 0;
	gnCodeSymbology = 0;
	gnCodeDgtNum = 0;
	gnCodeDgtNumExt = 0;
	gnCodeValidity = 0;
	gnCodeValidityExt = 0;
	return;
}


void CoorRelative2Absolute(int centAbs_x, int centAbs_y, int coorRltv_x, int coorRltv_y, unsigned char rttAngle, int * coorAbs_x, int * coorAbs_y)
{
	int s = 0, alpha = 0;

	*coorAbs_x = *coorAbs_y = 0;

	alpha = ryuArctan180Shift(coorRltv_y, coorRltv_x);

	if(alpha == 0 && coorRltv_x < 0) {
		alpha = 180;
	} 
	else if(alpha == 90 && coorRltv_y < 0) {
		alpha = 270;
	}
	else if(coorRltv_x < 0 && coorRltv_y < 0) {
		alpha += 180;
	}
	else if(coorRltv_x > 0 && coorRltv_y < 0) {
		alpha += 180;
	}

	s = coorRltv_x * coorRltv_x + coorRltv_y * coorRltv_y;
	s = (int)sqrt((double)s);

	alpha = alpha + rttAngle;

	alpha = (alpha >= 360) ? (alpha - 360) : alpha;

	*coorAbs_x = ((s * ryuCosShift(alpha))>>FLOAT2FIXED_SHIFT_DIGIT) + centAbs_x;
	*coorAbs_y = ((s * ryuSinShift(alpha))>>FLOAT2FIXED_SHIFT_DIGIT) + centAbs_y;

	return;
}

void CoorAbsolute2Relative(int centAbs_x, int centAbs_y, int coorAbs_x, int coorAbs_y, unsigned char rttAngle, int * coorRltv_x, int * coorRltv_y)
{
	int s = 0, alpha = 0;

	int nrttRltv_x = 0, nrttRltv_y = 0;

	*coorRltv_x = *coorRltv_y = 0;

	nrttRltv_x = coorAbs_x - centAbs_x;
	nrttRltv_y = coorAbs_y - centAbs_y;

	alpha = ryuArctan180Shift(nrttRltv_y, nrttRltv_x); // 0-360

	if(alpha == 0 && nrttRltv_x < 0) {
		alpha = 180;
	} 
	else if(alpha == 90 && nrttRltv_y < 0) {
		alpha = 270;
	}
	else if(nrttRltv_x < 0 && nrttRltv_y < 0) {
		alpha += 180;
	}
	else if(nrttRltv_x > 0 && nrttRltv_y < 0) {
		alpha += 180;
	}

	s = nrttRltv_x * nrttRltv_x + nrttRltv_y * nrttRltv_y;
	s = (int)sqrt((double)s);

	alpha = alpha - rttAngle;

	alpha = (alpha < 0) ? (alpha + 360) : alpha;

	*coorRltv_x = ((s * ryuCosShift(alpha))>>FLOAT2FIXED_SHIFT_DIGIT);
	*coorRltv_y = ((s * ryuSinShift(alpha))>>FLOAT2FIXED_SHIFT_DIGIT);

	return;
}

