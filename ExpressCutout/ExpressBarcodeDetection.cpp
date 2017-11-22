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
#define OCR_INTERCEPT_EXTEND	(36) 
#endif

#ifdef  _DEBUG_
#ifdef  _DEBUG_MAIN
#include "OpenCv_debugTools.h"
#endif
#endif

#ifdef	_PRINT_TIME
#define _PRINT_TIME_MAIN
#ifdef	_PRINT_TIME_MAIN
#include "OpenCv_debugTools.h"
#include <time.h>
#endif
#endif

#define BARCODE_RESULT_MAX_COUNT	(128)
#define LOCATE_SCAN_WINDOW_SIZE		(32)

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

	int i = 0, j = 0, status = 0;
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
	int sliceH = 0, nDecodeFlag = 0;
	int codeDigit = 0, codeType = 0, codeValid = 0, codeDirect = 0, codeModule = 0;
	char code_result[CODE_RESULT_ARR_LENGTH];
	int codeOffsetL = 0, codeOffsetR = 0;

	// 临时变量
	RyuPoint ptCenter;
	int nTmp = 0, nLctClusMaxGrad = 0;

	// OCR字符识别
	int IsNumberRecg = 0;
#ifdef _OCR
	RyuPoint ptIncptOCR, ptOncptOCR, ptCornerOCR[4];
#endif

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

#ifdef _MULTI_PKG
	WaybillSegm_resetCodeAreaStack();
#endif

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

	// 对条码区域进行排序（面积）
	for(i = 0; i < nSegmentCnt; i++) {
		for(j = i+1; j < nSegmentCnt; j++) {
			nTmp1 = (pSegmentBarcode[i].max_ontcpt-pSegmentBarcode[i].min_ontcpt) 
				* (pSegmentBarcode[i].max_intcpt-pSegmentBarcode[i].min_intcpt);
			nTmp2 = (pSegmentBarcode[j].max_ontcpt-pSegmentBarcode[j].min_ontcpt) 
				* (pSegmentBarcode[j].max_intcpt-pSegmentBarcode[j].min_intcpt);
			if(nTmp1 < nTmp2) {
				tmpSegment = pSegmentBarcode[i];
				pSegmentBarcode[i] = pSegmentBarcode[j];
				pSegmentBarcode[j] = tmpSegment;
			}
		}
	}

	// 限制nSegmentCnt循环次数
	nSegmentCnt = RYUMIN(nSegmentCnt, 5);

	// 依次对分割出来的条形码区域图像进行缩放、旋转和识别
	for(i = 0; i < nSegmentCnt; i++) {
		// 根据标志flag对条码区域图像进行缩放、旋转矫正
		status = RotateImage(in_data, width, height, pSegmentBarcode[i].corner_ext, pSegmentBarcode[i].angle, 
			pSegmentBarcode[i].flag+1, (short *) &rtt_width, (short *) &rtt_height);
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
//			continue;
			break;
		} else if(rtt_width <= 5 || rtt_height <= 5) {
//			continue;
			break;
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
//			continue;
			break;
		}
		
		// 条码区域矫正图像预处理(调整对比度、锐化、二值化)
		status = BarcodeImgProcess(ucBarcodeImage, rtt_width, rtt_height);
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
//			continue;
			break;
		}

		ucDecodeImage = getBarcodeImgProcOutput();

		//////////////////////////////////////////////////////////////////////////
		// 注释部分
		//sliceH = (1 == pSegmentBarcode[i].flag) ? 16 : 10;
		//// 条码识别
		//status = DecodeBarcode(ucBarcodeImage, rtt_width, rtt_height, sliceH, code_result, 
		//	&codeType, &codeDigit, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR);
		//////////////////////////////////////////////////////////////////////////
		// 条码识别，添加修改部分
		nDecodeFlag = 0;
		sliceH = 16;
		IsNumberRecg = 0;//数字识别标识符清零 2016-5-12 wcc
		while(1) {
 			memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);
			codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
// 			codeType = codeDigit = codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
// 			nDecodeFlag = DecodeBarcode(ucDecodeImage, rtt_width, rtt_height, sliceH, code_result, 
// 				&codeType, &codeDigit, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR);
			codeType = gnCodeSymbology;
			codeDigit = gnCodeDgtNum;
			codeValid = gnCodeValidity;
			nDecodeFlag = DecodeBarcode(ucDecodeImage, rtt_width, rtt_height, sliceH,
				&codeType, &codeDigit, &codeValid, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR, code_result);
			if( 1 == nDecodeFlag )
				break;
			sliceH -= 4;
			if( 4 >= sliceH )
				break;
		}

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
		Write_Log(LOG_TYPE_INFO, "TRACE- %dth DecodeBarcode proc done. nDecodeFlag=%d", i, nDecodeFlag);
#endif
#endif

#if 1
		// 拼图重识别
		if( 1 != nDecodeFlag ) {
			status = BarcodeImageproc_recombination(ucDecodeImage, rtt_width, rtt_height);
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth BarcodeRecombination proc done. status=%d", i, nDecodeFlag);
#endif
#endif
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of BarcodeImageproc_recombination, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			}
			sliceH = 16;
			while(1) {
				memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);
				codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
// 				codeType = codeDigit = codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
// 				nDecodeFlag = DecodeBarcode(ucDecodeImage, rtt_width, rtt_height, sliceH, code_result, 
// 					&codeType, &codeDigit, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR);
				codeType = gnCodeSymbology;
				codeDigit = gnCodeDgtNum;
				codeValid = gnCodeValidity;
				nDecodeFlag = DecodeBarcode(ucDecodeImage, rtt_width, rtt_height, sliceH,
					&codeType, &codeDigit, &codeValid, &codeModule, &codeDirect, &codeOffsetL, &codeOffsetR, code_result);
				if( 1 == nDecodeFlag )
					break;
				sliceH -= 4;
				if( 4 >= sliceH )
					break;
			}

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth BarcodeRecombination DecodeBarcode proc done. nDecodeFlag=%d", i, nDecodeFlag);
#endif
#endif
		}
#endif
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// OCR字符识别模块
		// 若无法识别，则进入OCR字符识别
#ifdef _OCR
		if( 1 != nDecodeFlag && i <= 1) {
			// Step1
			ptIncptOCR.x = pSegmentBarcode[i].min_intcpt - OCR_INTERCEPT_EXTEND;
			ptIncptOCR.y = pSegmentBarcode[i].min_intcpt;
			ptOncptOCR.x = pSegmentBarcode[i].min_ontcpt;
			ptOncptOCR.y = pSegmentBarcode[i].max_ontcpt;
			InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, pSegmentBarcode[i].angle, ptCornerOCR);
			status = RotateImage(in_data, width, height, ptCornerOCR, pSegmentBarcode[i].angle, 
				1, (short *) &rtt_width, (short *) &rtt_height);
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of RotateImageOCR, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			}
			ucBarcodeImage = GetRotateImage();
			if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
				printf( "Error! Unexpected return of GetRotateImageOCR, slice%d ucBarcodeImage=0x%x. --algorithm_run\n", i, ucBarcodeImage );
#endif
				continue;
			}

			// TODO: 此时ucBarcodeImage中存储的是条形码上方的图像，用作字符识别
			status =  SetCodeNumberImage(2,ucBarcodeImage,rtt_width,rtt_height);
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of SetCodeNumberTopImage, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			}

			// Step2
			ptIncptOCR.x = pSegmentBarcode[i].max_intcpt;
			ptIncptOCR.y = pSegmentBarcode[i].max_intcpt + OCR_INTERCEPT_EXTEND;
			ptOncptOCR.x = pSegmentBarcode[i].min_ontcpt;
			ptOncptOCR.y = pSegmentBarcode[i].max_ontcpt;
			InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, pSegmentBarcode[i].angle, ptCornerOCR);
			status = RotateImage(in_data, width, height, ptCornerOCR, pSegmentBarcode[i].angle, 
				1, (short *) &rtt_width, (short *) &rtt_height);
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of RotateImageOCR2, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			}
			ucBarcodeImage = GetRotateImage();
			if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
				printf( "Error! Unexpected return of GetRotateImageOCR2, slice%d ucBarcodeImage=0x%x. --algorithm_run\n", i, ucBarcodeImage );
#endif
				continue;
			}

			// TODO: 此时ucBarcodeImage中存储的是条形码下方的图像，用作字符识别
			status =  SetCodeNumberImage(1,ucBarcodeImage,rtt_width,rtt_height);
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of SetCodeNumberBottomImage, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			}
			

			// Step3
			// TODO: 若找到结果，置nDecodeFlag为1，将结果写入code_result[]中，
			// TODO: 置nCodeCharNum为条码结果位数，置codeDirect为1(条码正向)或-1(条码反向)

			//获取数字图像
			int codeHeight = abs(pSegmentBarcode[i].max_intcpt - pSegmentBarcode[i].min_intcpt + 1);
			nDecodeFlag = NumberRecoge(codeHeight,code_result,&codeDigit,&codeDirect);
			if(nDecodeFlag == 1)
			{
				codeModule = abs(pSegmentBarcode[i].max_ontcpt - pSegmentBarcode[i].min_ontcpt + 1);
				IsNumberRecg = 1;
			}

		}
#endif
		//////////////////////////////////////////////////////////////////////////

		if (1 == nDecodeFlag) {
			UpdateCodeCorner(&pSegmentBarcode[i], codeOffsetL, codeOffsetR);
			memcpy(result_node.strCodeData, code_result, CODE_RESULT_ARR_LENGTH);
			if (IsNumberRecg == 1){
				result_node.nFlag = 2;
			}
			else{
				result_node.nFlag = 1;
			}
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

#ifdef _MULTI_PKG
			WaybillSegm_pushCodeAreaStack(pSegmentBarcode[i].corner, pSegmentBarcode[i].angle, nDecodeFlag, result_node.strCodeData);
#endif

#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- %dth CodeResults writing done.", i);
#endif
#endif
		}

		if(code_count >= gnRglCodeCnt && gnRglCodeCnt > 0)
			break;
	}

	if(0 == code_count) {
		if(10 > nLctClusMaxGrad) {
			// 若条码特征区域梯度幅值特征较弱，则也判定为无法找到条码
			code_count = ret_val = -2;
			goto nExit;
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
		} else {
			// 无法识别
			code_count = ret_val = -1;
		}
	}

	ret_val = code_count;

nExit:
#ifdef _MULTI_PKG
	/************************************************************************/
	/* 测试多包裹检测算法                                                    */
	/************************************************************************/
	status = WaybillSegment(in_data, width, height);
#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	if(0 == status)			printf("********Cannot find express package!\n");
	else if(1 == status)	printf("********The frame has only 1 express package!\n");
	else if(2 == status)	printf("********Warning, the frame MAY has multiple express packages!\n");
	else if(3 == status)	printf("********Warning, the frame has multiple express packages!\n");
	else					printf("********Error, WaybillSegment runs into some exceptions!\n");
#endif
#endif
	////////////////////////////////////////////////////////////////////////*/
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

#ifdef _MULTI_PKG
	status = ryuInitFloodFill(ryuSize(400, 400), 1024);
#endif

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

#ifdef _MULTI_PKG
	ryuReleaseFloodFill();
#endif

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



