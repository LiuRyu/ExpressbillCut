#include "stdafx.h"

#include "RyuCore.h"
#include "Decoder_EAN13.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
#include "OpenCv_debugTools.h"
#endif
#endif

#define CODEEAN13_MODULE_COUNT	(59)

#define CODEEAN13_SEQARR_COUNT	(12)

#define CODEEAN_MAX_MODULE_DEVIATION	(2<<FLOAT2FIXED_SHIFT_DIGIT)

const int gnDecoderModuleCodeEAN[20] = {
	0x3211, 0x2221, 0x2122, 0x1411, 0x1132, 0x1231, 0x1114, 0x1312, 0x1213, 0x3112,
	0x1123, 0x1222, 0x2212, 0x1141, 0x2311, 0x1321, 0x4111, 0x2131, 0x3121, 0x2113,
};

const int gnDecoderModuleSetCodeEAN[10] = {
	0x000000, 0x001011, 0x001101, 0x001110, 0x010011, 
	0x011001, 0x011100, 0x010101, 0x010110, 0x011010
};

static int gnDecoderLeftSSArrEAN13[32] = {0};
static int gnDecoderRightSSArrEAN13[32] = {0};

static int gnDecoderStartArrEAN13[32] = {0};
static int gnDecoderStopArrEAN13[32] = {0};
static int gnDecoderSSArrCntEAN13 = 0;


static int gnDecoderSeqArrEAN13[32] = {0};
static int gnDecoderFaithArrEAN13[32] = {0};

int CodeStartStopMatch_EAN13(int * decode_arr, int arr_count,  int * start_idx, int * stop_idx);
int CodeModelMatch_EAN13(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr);
int Transcode_EAN13(int * seq_arr, char * strResult);
int InvertSeqArr_EAN13( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx );


int RecgCodeEAN13(int * decode_arr, int * decode_arrproc, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR)
{
	int nRet = 0, status = 0;
	int i = 0;

	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nStartIdx = 0, nStopIdx = 0;
	int nArrCount = 0;

	int isFindCode = 0;

	if(!decode_arr || !code_result || !code_digit || !code_module
		|| !code_direct || !code_idxL || !code_idxR) {
#ifdef	_PRINT_PROMPT
			printf( "ERROR! Invalid input of RecgCodeEAN13, decode_arr=0x%x, code_result=0x%x\
					, code_digit=0x%x, code_module=0x%x, code_direct=0x%x, code_idxL=0x%x\
					, code_idxR=0x%x\n", decode_arr, code_result, code_digit, code_module, \
					code_direct, code_idxL, code_idxR );
#endif
			nRet = -1;
			goto nExit;
	}

	// 重置初始状态
	code_result[0] = 0;
	*code_digit = *code_module = *code_direct = *code_idxL = 0;
	*code_idxR = arr_count - 1;

	if( arr_count < CODEEAN13_MODULE_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot find EAN-13 code, too small arr_count: %d\n", arr_count);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeStartStopMatch_EAN13( decode_arr, arr_count, &nStartIdx, &nStopIdx );
	if(0 >= status) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf( "Unexpected return of CodeStartStopMatch_EAN13, cannot find code start/stop,\
				return=%d\n", status );
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	for(i = 0; i < gnDecoderSSArrCntEAN13; i++) {
		// 重置数组
		memcpy(decode_arrproc, decode_arr, arr_count * sizeof(int));
		nStartIdx = gnDecoderStartArrEAN13[i];
		nStopIdx = gnDecoderStopArrEAN13[i];
		if(nStartIdx > nStopIdx) {
			InvertSeqArr_EAN13(decode_arrproc, arr_count, &nStartIdx, &nStopIdx);
			nDirection = -1;
		} else {
			nDirection = 1;
		}

		nArrCount = nStopIdx - nStartIdx + 1;	// 黑白数
		if( nArrCount != CODEEAN13_MODULE_COUNT) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot find EAN-13 code, bad nColCount: %d, i = %d\n", nArrCount, i);
#endif
#endif
			continue;
		}

		status = CodeModelMatch_EAN13( decode_arrproc+nStartIdx, nArrCount, 
			gnDecoderSeqArrEAN13, gnDecoderFaithArrEAN13 );
		if( status != CODEEAN13_SEQARR_COUNT) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot find EAN/UPC code, bad nSeqCount: %d, i = %d\n\n", status, i);
#endif
#endif
			continue;
		}

		status = Transcode_EAN13( gnDecoderSeqArrEAN13, code_result );
		if( status != 13 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot find EAN/UPC code, bad return of Transcode_code128, return = %d, i = %d\n", status, i);
#endif
#endif
			continue;
		} else {
			isFindCode = 1;
			break;
		}
	}

	if(1 == isFindCode) {
		nModuleCnt = 95;
		nDigitCnt = 13;
	} else {
		code_result[0] = 0;
		nDirection = 0;
		nRet = 0;
		goto nExit;
	}

	*code_digit = nDigitCnt;
	*code_module = nModuleCnt;
	*code_direct = nDirection;
	*code_idxL = (nDirection > 0) ? nStartIdx : (arr_count-1-nStopIdx);
	*code_idxR = (nDirection > 0) ? nStopIdx : (arr_count-1-nStartIdx);
	nRet = 1;

nExit:
	return nRet;
}

int CodeStartStopChecking_EAN13(int left_idx, int right_idx)
{
	int arrCnt = right_idx - left_idx + 1;

	if(arrCnt != CODEEAN13_MODULE_COUNT)
		return 0;

	return 1;
}

int CodeStartStopMatch_EAN13( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = 0;

	int s = 0, f[7] = {0}, r[7] = {0};
	int mask = 0, nTmp = 0;

	int * LeftSSArr = gnDecoderLeftSSArrEAN13;
	int * RightSSArr = gnDecoderRightSSArrEAN13;
	int leftCnt = 0, rightCnt = 0;

	int * StartArr = gnDecoderStartArrEAN13;
	int * StopArr = gnDecoderStopArrEAN13;

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	gnDecoderSSArrCntEAN13 = 0;

	//////////////////////////////////////////////////////////////////////////
	// 从中间向左寻找条码起始位或反向终止符号位
	pCodeCol = decode_arr + arr_count / 2;
	for( i = arr_count/2; i >= 0; i-- ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol--;
			continue;
		}

		// 取开始/结束位和相邻的一个数据位
		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5]- pCodeCol[6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_codeEAN\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		// 对比开始位
		mask = nTmp = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			nTmp = (r[j] > 6) ? (nTmp + 1) : nTmp;
			mask |= (r[j] << (24-4*j));
		}
		// 不符合条码基本规格，跳过
		if(nTmp > 0) {
			pCodeCol--;
			continue;
		}
		// 匹配开始结束位
		if(0x111 == ((mask & 0xfff0000) >> 16)) {
			LeftSSArr[leftCnt++] = i;
			if(leftCnt >= 32)
				break;
		}
		pCodeCol--;
	}

	// 左侧找不到开始或终止位
	if(leftCnt <= 0) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot find EAN/UPC code, cannot find start/~end in CodeStartStopMatch_codeEAN\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	//////////////////////////////////////////////////////////////////////////
	// 从中间向右寻找条码终止位或反向起始符号位
	pCodeCol = decode_arr + arr_count / 2;
	for( i = arr_count/2; i < arr_count; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol++;
			continue;
		}

		// 取开始/结束位和相邻的一个数据位
		s = -pCodeCol[0] + pCodeCol[-1] - pCodeCol[-2] + pCodeCol[-3] - pCodeCol[-4] + pCodeCol[-5]- pCodeCol[-6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_codeEAN\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		// 对比开始位
		mask = nTmp = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			nTmp = (r[j] > 6) ? (nTmp + 1) : nTmp;
			mask |= (r[j] << (24-4*j));
		}
		// 不符合条码基本规格，跳过
		if(nTmp > 0) {
			pCodeCol++;
			continue;
		}
		// 匹配开始结束位
		if(0x111 == ((mask & 0xfff0000) >> 16)) {
			RightSSArr[rightCnt++] = i;
			if(rightCnt >= 32)
				break;
		}
		pCodeCol++;
	}

	// 右侧找不到开始或终止位
	if(rightCnt <= 0) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot find EAN/UPC code, cannot find start/~end in CodeStartStopMatch_codeEAN\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 开始结束位匹配
	for(i = 0; i < leftCnt; i++) {
		for(j = 0; j < rightCnt; j++) {
			nTmp = CodeStartStopChecking_EAN13(LeftSSArr[i], RightSSArr[j]);
			if(1 == nTmp) {
				// EAN13码正反向在识别之前无法判定
				StartArr[gnDecoderSSArrCntEAN13] = LeftSSArr[i];
				StopArr[gnDecoderSSArrCntEAN13++] = RightSSArr[j];
				if(gnDecoderSSArrCntEAN13 >= 32)
					break;
				StartArr[gnDecoderSSArrCntEAN13] = RightSSArr[j];
				StopArr[gnDecoderSSArrCntEAN13++] = LeftSSArr[i];
				if(gnDecoderSSArrCntEAN13 >= 32)
					break;
			}
		}
		if(gnDecoderSSArrCntEAN13 >= 32)
			break;
	}

	nRet = gnDecoderSSArrCntEAN13;

nExit:
	return nRet;
}

/*
int CodeStartStopMatch_EAN13( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = 0;

	int nStartIdx = 0, nStopIdx = 0;
	int nIsFindL = 0, nIsFindR = 0;
	int s = 0, f[7] = {0}, r[7] = {0};
	int mask = 0, loop = 0;

	int nVar = 0, nMin = 0, nMinSeq = 0;
	int nTmp = 0;

	int * LeftSSArr = gnDecoderLeftSSArrEAN13;
	int * RightSSArr = gnDecoderRightSSArrEAN13;
	int startCnt = 0, stopCnt = 0;

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	*start_idx = *stop_idx = 0;

	//////////////////////////////////////////////////////////////////////////
	// 从中间向左寻找条码起始位或反向终止符号位
	pCodeCol = decode_arr;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.2版本优化，限制左右检索边界，减少乱码
	for( i = 0; i < arr_count/2; i++ ) {
	//for( i = 0; i < 6; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol++;
			continue;
		}

		// 取开始/结束位和相邻的一个数据位
		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5]- pCodeCol[6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_codeEAN\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		// 对比开始位
		mask = nTmp = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			nTmp = (r[j] > 6) ? (nTmp + 1) : nTmp;
			mask |= (r[j] << (24-4*j));
		}
		// 不符合条码基本规格，跳过
		if(nTmp > 0) {
			pCodeCol++;
			continue;
		}
		// 匹配开始结束位
		if(0x111 == ((mask & 0xfff0000) >> 16)) {
			nStartIdx = i;
			nIsFindL = 1;
		}
		if(1 == nIsFindL) 
			break;
		pCodeCol++;
	}

	// 找不到开始或终止位
	if(nIsFindL != 1) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN
		printf("Cannot find EAN/UPC code, cannot find start/~end in CodeStartStopMatch_codeEAN\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	//////////////////////////////////////////////////////////////////////////
	// 反向搜索条码终止位或反向开始符号位
	pCodeCol = decode_arr + arr_count - 1;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.2版本优化，限制左右检索边界，减少乱码
	for(i = 0; i < arr_count/2; i++) {
	//for(i = 0; i < 6; i++) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol--;
			continue;
		}
		s = -pCodeCol[0] + pCodeCol[-1] - pCodeCol[-2] + pCodeCol[-3] - pCodeCol[-4] + pCodeCol[-5] - pCodeCol[-6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_codeEAN\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		mask = nTmp = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			nTmp = (r[j] > 6) ? (nTmp + 1) : nTmp;
			mask |= (r[j] << (24-4*j));
		}
		// 不符合条码基本规格，跳过
		if(nTmp > 0) {
			pCodeCol--;
			continue;
		}
		// 匹配开始结束位
		if(0x111 == ((mask & 0xfff0000) >> 16)) {
			nStopIdx = arr_count - 1 - i;
			nIsFindR = 1;
			break;
		}
		if(1 == nIsFindR) 
			break;
		pCodeCol--;
	}

	// 找不到开始或终止位
	if(nIsFindR != 1) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find EAN/UPC code, cannot find end/~start in CodeStartStopMatch_codeEAN\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	*start_idx = nStartIdx;
	*stop_idx = nStopIdx;
	nRet = 1;

nExit:
	return nRet;
}*/

int CodeModelMatch_EAN13(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr)
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = decode_arr + 3;

	int s = 0, f[5] = {0}, r[5] = {0};
	int mask = 0, loop = 0;

	int nVar = 0, nMin = 0, nMinCnt = 0, nMinIdx[8] = {0};

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	if(arr_count == CODEEAN13_MODULE_COUNT)
		loop = 6;
	else {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot match EAN/UPC code, unexpected value of arr_count in CodeModelMatch_codeEAN, \
			   arr_count=%d\n", arr_count);
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	for( i = 0; i < loop; i++ ) {
		if( pCodeCol[0] < 0 ) {	// 规定从白色开始
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot match EAN/UPC code, unexpected value of pCodeCol[0] in CodeModelMatch_codeEAN, \
				   pCodeCol[0]=%d (should be >0)\n", pCodeCol[0]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		s = pCodeCol[0] - pCodeCol[1] + pCodeCol[2] - pCodeCol[3];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot match EAN-13 code, unexpected value of s in CodeModelMatch_codeEAN, \
				   s=%d (should be >0)\n", s);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		mask = 0;
		for( j = 0; j < 4; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 7 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (12-4*j));
		}
		// 直接对比
		for( n = 0; n < 20; n++ ) {
			if( mask == gnDecoderModuleCodeEAN[n] )
				break;
		}

		if( n < 20) {
			seq_arr[i] = n;
			faith_arr[i] = 3;	// 可靠系数3：直接匹配模板
		} else {
			// 距离最小
			nMinCnt = 0;
			nMin = 0x7fffffff;
			for( n = 0; n < 20; n++ ) {
				mask = gnDecoderModuleCodeEAN[n];
				nVar = abs(f[0]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
					+ abs(f[1]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[2]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[3]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));

				if( nVar < nMin ) {
					nMin = nVar;
					nMinCnt = 0;
					nMinIdx[nMinCnt] = n;
				} else if( nVar == nMin && nMinCnt < 7) {
					nMinIdx[++nMinCnt] = n;
				}
			}
			nMinCnt++;
			if( 1 == nMinCnt && nMin < CODEEAN_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 2;
			} else if( 1 < nMinCnt && nMin < CODEEAN_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 1;
			} else {
				faith_arr[i] = 0;
			}
			seq_arr[i] = nMinIdx[0];
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
#if		0
			printf("Cannot match code128 directly in CodeModelMatch_code128, seq=%d\n", i+1);
			printf("s:%5d  %5d  %5d  %5d  %5d  %5d\n",\
				pCodeCol[0], pCodeCol[1], pCodeCol[2], pCodeCol[3], pCodeCol[4], pCodeCol[5]);
			printf("f:%.3f  %.3f  %.3f  %.3f  %.3f  %.3f\n",\
				-pCodeCol[0]*11.0/s, pCodeCol[1]*11.0/s, -pCodeCol[2]*11.0/s, \
				pCodeCol[3]*11.0/s, -pCodeCol[4]*11.0/s, pCodeCol[5]*11.0/s);
			printf("r:%5d %5d %5d %5d %5d %5d\n", r[0], r[1], r[2], r[3], r[4], r[5]);

			printf("距离:nMin=%d, nMinCnt=%d, nMinIdx=%d, module=%x\n", nMin, nMinCnt, nMinIdx[0], \
				gnDecoderModuleCode128[nMinIdx[0]]);
#endif
#endif
#endif
		}
		pCodeCol += 4;
	}

	// 检查中间分隔符
	if( pCodeCol[0] < 0 ) {	// 规定从白色开始
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot match EAN/UPC code, unexpected value of pCodeCol[0] in CodeModelMatch_codeEAN, \
			   pCodeCol[0]=%d (should be >0)\n", pCodeCol[0]);
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	s = pCodeCol[0] - pCodeCol[1] + pCodeCol[2] - pCodeCol[3] + pCodeCol[4];
	if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot match EAN/UPC code, unexpected value of s in CodeModelMatch_codeEAN, \
			   s=%d (should be >0)\n", s);
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	mask = 0;
	for( j = 0; j < 5; j++ ) {
		f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 5 / s;
		r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
		mask |= (r[j] << (16-4*j));
	}
	if(mask != 0x11111) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot match EAN/UPC code, unexpected Fengefu in CodeModelMatch_codeEAN, \
			   mask=0x%x (should be 0x11111)\n", mask);
#endif
#endif
		nRet = -2;
		goto nExit;
	}
	pCodeCol += 5;

	// 读取后半部分
	for( i = loop; i < (loop<<1); i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot match EAN/UPC code, unexpected value of pCodeCol[0] in CodeModelMatch_codeEAN, \
				   pCodeCol[0]=%d (should be >0)\n", pCodeCol[0]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot match EAN/UPC code, unexpected value of s in CodeModelMatch_codeEAN, \
				   s=%d (should be >0)\n", s);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		mask = 0;
		for( j = 0; j < 4; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 7 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (12-4*j));
		}
		// 直接对比
		for( n = 0; n < 20; n++ ) {
			if( mask == gnDecoderModuleCodeEAN[n] )
				break;
		}

		if( n < 20) {
			seq_arr[i] = n;
			faith_arr[i] = 3;	// 可靠系数3：直接匹配模板
		} else {
			// 距离最小
			nMinCnt = 0;
			nMin = 0x7fffffff;
			for( n = 0; n < 20; n++ ) {
				mask = gnDecoderModuleCodeEAN[n];
				nVar = abs(f[0]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
					+ abs(f[1]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[2]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[3]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));

				if( nVar < nMin ) {
					nMin = nVar;
					nMinCnt = 0;
					nMinIdx[nMinCnt] = n;
				} else if( nVar == nMin && nMinCnt < 7) {
					nMinIdx[++nMinCnt] = n;
				}
			}
			nMinCnt++;
			if( 1 == nMinCnt && nMin < CODEEAN_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 2;
			} else if( 1 < nMinCnt && nMin < CODEEAN_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 1;
			} else {
				faith_arr[i] = 0;
			}
			seq_arr[i] = nMinIdx[0];
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
#if		0
			printf("Cannot match code128 directly in CodeModelMatch_code128, seq=%d\n", i+1);
			printf("s:%5d  %5d  %5d  %5d  %5d  %5d\n",\
				pCodeCol[0], pCodeCol[1], pCodeCol[2], pCodeCol[3], pCodeCol[4], pCodeCol[5]);
			printf("f:%.3f  %.3f  %.3f  %.3f  %.3f  %.3f\n",\
				-pCodeCol[0]*11.0/s, pCodeCol[1]*11.0/s, -pCodeCol[2]*11.0/s, \
				pCodeCol[3]*11.0/s, -pCodeCol[4]*11.0/s, pCodeCol[5]*11.0/s);
			printf("r:%5d %5d %5d %5d %5d %5d\n", r[0], r[1], r[2], r[3], r[4], r[5]);

			printf("距离:nMin=%d, nMinCnt=%d, nMinIdx=%d, module=%x\n", nMin, nMinCnt, nMinIdx[0], \
				gnDecoderModuleCode128[nMinIdx[0]]);
#endif
#endif
#endif
		}
		pCodeCol += 4;
	}

	nRet = (loop << 1);

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
	printf("Decode seq_arr: ");
	for(i = 0; i < (loop<<1); i++)
		printf("%5d",seq_arr[i]);
	printf("\n");
#endif
#endif

nExit:
	return nRet;
}

int Transcode_EAN13( int * seq_arr, char * strResult )
{
	int nRet = 0, i = 0;

	int mask = 0, startDigit = -1;
	int oddsum = 0, evensum = 0;
	int transArr[13] = {0};

	for(i = 0; i < 6; i++) {
		if(seq_arr[i] >= 10) {
			mask |= (1 << (20-4*i));
		}
		if(seq_arr[i+6] >= 10) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
			printf("Cannot verify EAN13 code, unexpected seq_arr of s in CodeModelMatch_codeEAN, \
				   seq_arr[%d]=%d\n", i+6, seq_arr[i+6]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
	}

	for(i = 0; i < 10; i++) {
		if(mask == gnDecoderModuleSetCodeEAN[i]) {
			startDigit = i;
			break;
		}
	}

	if(startDigit < 0) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot verify EAN13 code, unexpected startDigit in CodeModelMatch_codeEAN, \
			   mask=0x%x\n", mask);
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	transArr[0] = startDigit;
	for(i = 0; i < 12; i++) {
		transArr[i+1] = (seq_arr[i] >= 10) ? (seq_arr[i]-10) : seq_arr[i];
	}

	for(i = 0; i < 6; i++) {
		oddsum += transArr[i*2];
		evensum += transArr[i*2+1];
	}
	oddsum += (evensum * 3);
	oddsum = 10 - (oddsum % 10);
	oddsum = (oddsum >= 10) ? (oddsum-10) : oddsum;

	if(transArr[12] != oddsum) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_EAN13
		printf("Cannot verify EAN13 code, unexpected checkDigit in CodeModelMatch_codeEAN, \
			   checkDigit=%d(should be %d)\n", transArr[12], oddsum);
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	for(i = 0; i < 13; i++) {
		strResult[i] = transArr[i] + 48;
	}

	nRet = 13;

nExit:
	return nRet;
}

int InvertSeqArr_EAN13( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
{
	int i = 0, loop = 0, mask = 0;
	int nStartIdx = *start_idx, nStopIdx = *stop_idx;

	loop = arr_count >> 1;
	for (i = 0; i < loop; i++) {
		mask = decode_arr[i];
		decode_arr[i] = decode_arr[arr_count-i-1];
		decode_arr[arr_count-i-1] = mask; 
	}
	*start_idx = arr_count - 1 - nStartIdx;
	*stop_idx = arr_count - 1 - nStopIdx;

	return 1;
}
