#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "RyuCore.h"

#include "Decoder_code39.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
#include "OpenCv_debugTools.h"
#endif
#endif

#define CODE39_MIN_DECODEARR_COUNT		(29)
#define CODE39_MIN_SEQARR_COUNT		(4)

const short gnDecoderModuleCode39[45] = {
	0x034,     0x121,     0x061,     0x160,     0x031, 
	//	000110100, 100100001, 001100001, 101100000, 000110001, 
	0x130,     0x070,     0x025,      0x124,     0x064, 
	//	100110000, 001110000, 000100101, 100100100, 001100100, 
	0x109,     0x049,     0x148,     0x019,     0x118,
	//	100001001, 001001001, 101001000, 000011001, 100011000, 
	0x058,     0x00d,     0x10d,     0x04c,     0x01c,
	//	001011000, 000001101, 100001101, 001001100, 000011100, 
	0x103,     0x043,     0x142,     0x013,     0x112,
	//	100000011, 001000011, 101000010, 000010011, 100010010, 
	0x052,     0x007,     0x106,     0x046,     0x016,
	//	001010010, 000000111, 100000110, 001000110, 000010110, 
	0x181,     0x0c1,     0x1c0,     0x091,     0x190,
	//	110000001, 011000001, 111000000, 010010001, 110010000, 
	0x0d0,     0x085,     0x184,     0x0c4,     0x0a8,
	//	011010000, 010000101, 110000100, 011000100, 010101000, 
	0x0a2,     0x08a,     0x02a,     0x094,     0x052
	//	010100010, 010001010, 000101010, 010010100, 001010010(反向标识位)
};

static int gnDecoderSeqArrCode39[128] = {0};
static int gnDecoderFaithArrCode39[128] = {0};

int CodeStartStopMatch_code39( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx );

int CodeModelMatch_code39(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr);

int CheckDigitVerify_code39(int * seq_arr, int * faith_arr, int seq_cnt, int isCheckDigit);

int Transcode_code39( int * seq_arr, int seq_cnt, char * strResult );

int RecgCode39(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR)
{
	int nRet = 0, status = 0;

	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nStartIdx = 0, nStopIdx = 0;
	int nArrCount = 0, nSeqCount = 0;

	if(!decode_arr || !code_result || !code_digit || !code_module
		|| !code_direct || !code_idxL || !code_idxR) {
#ifdef	_PRINT_PROMPT
			printf( "ERROR! Invalid input of RecgCode39, decode_arr=0x%x, code_result=0x%x\
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

	if( arr_count < CODE39_MIN_DECODEARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, too small arr_count: %d\n", arr_count);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeStartStopMatch_code39( decode_arr, arr_count, &nStartIdx, &nStopIdx );
	if(1 != status && 2 != status) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf( "Unexpected return of CodeStartStopMatch_code39, cannot find code start/stop,\
				return=%d\n", status );
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nDirection = (1 == status) ? 1 : -1;		// 条码方向

	nArrCount = nStopIdx - nStartIdx + 1;	// 黑白数
	if( nArrCount < CODE39_MIN_DECODEARR_COUNT || 0 != (nArrCount+1) % 10) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, bad nColCount: %d\n", nArrCount);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeModelMatch_code39( decode_arr+nStartIdx, nArrCount, 
		gnDecoderSeqArrCode39, gnDecoderFaithArrCode39 );
	if( status < CODE39_MIN_SEQARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, bad nSeqCount: %d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nSeqCount = status;
	nModuleCnt = nSeqCount * 16;

	// 校验位检验，仅限于有校验位的39条码
	status = CheckDigitVerify_code39( gnDecoderSeqArrCode39, gnDecoderFaithArrCode39, nSeqCount, 0 );
	if(1 != status ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, check digit verification failed, return=%d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = Transcode_code39( gnDecoderSeqArrCode39, nSeqCount, code_result );
	if( status <= 0 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, bad return of Transcode_code39, return=%d\n", status);
#endif
#endif
		code_result[0] = 0;
		nRet = 0;
		goto nExit;
	}

	nDigitCnt = status;

	*code_digit = nDigitCnt;
	*code_module = nModuleCnt;
	*code_direct = nDirection;
	*code_idxL = (nDirection > 0) ? nStartIdx : (arr_count-1-nStopIdx);
	*code_idxR = (nDirection > 0) ? nStopIdx : (arr_count-1-nStartIdx);
	nRet = 1;

nExit:
	return nRet;
}

int CodeStartStopMatch_code39( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
{
	int nRet = 0;
	int i = 0, j = 0;
	int * pCodeCol = 0;

	int nStartIdx = 0, nStopIdx = 0;
	int nIsFindL = 0, nIsFindR = 0;
	int s = 0, t = 0, f[9] = {0}, r[9] = {0}; 
	int mask = 0, loop = 0;

	int k = 0, tmp = 0, sort[9] = {0};

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	*start_idx = *stop_idx = 0;

	//////////////////////////////////////////////////////////////////////////
	// 寻找条码起始位或反向终止符号位
	pCodeCol = decode_arr;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.5.2版本优化，限制左右检索边界，减少乱码
	//for( i = 0; i < arr_count - 9; i++ ) {
	for( i = 0; i < 6; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol++;
			continue;
		}

		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5]
			-pCodeCol[6] + pCodeCol[7] - pCodeCol[8];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code39, s=%d\n", s );
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		//////////////////////////////////////////////////////////////////////////
		// 2.0.4.2版本改进
		// 旧版代码，注释
		// 对比开始位和反向结束位
// 		mask = 0;
// 		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;
// 		for( j = 0; j < 9; j++ ) {
// 			f[j] = abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT;
// 			r[j] = (f[j] > t) ? 1 : 0;
// 			mask  |= (r[j] << (8-j));
// 		}

		//////////////////////////////////////////////////////////////////////////
		// 新版代码，针对EMS不规则条码进行改进，对长度进行排序得到阈值
		// 计算通用阈值
		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;

		// 对输入元素宽度进行排序
		for( j = 0; j < 9; j++ ) {
			sort[j] = abs(pCodeCol[j]);
		}
		for( j = 0; j < 9; j++ ) {
			for( k = j+1; k < 9; k++ ) {
				if( sort[j] < sort[k] ) {
					tmp = sort[j];
					sort[j] = sort[k];
					sort[k] = tmp;
				}
			}
		}
		// 对边界宽度进行阈值检验
		if( (sort[2]<<FLOAT2FIXED_SHIFT_DIGIT) < t * 2/3 ||
			(sort[3]<<FLOAT2FIXED_SHIFT_DIGIT) > t * 4/3 ||
			sort[0] > sort[8] * 8 ) {
				pCodeCol++;
				continue;
		}
		// 计算阈值
		tmp = (sort[2] + sort[3]) >> 1;
		// 获取编码
		mask = 0;
		for( j = 0; j < 9; j++ ) {
			r[j] = (abs(pCodeCol[j]) > tmp) ? 1 : 0;
			mask  |= (r[j] << (8-j));
		}
		// 2.0.4.2版本改进结束
		//////////////////////////////////////////////////////////////////////////

		// 对比开始结束位
		if( mask == gnDecoderModuleCode39[43] ) {
			nStartIdx = i;
			nIsFindL = 1;
			break;
		} else if( mask == gnDecoderModuleCode39[44]) {	// 发现反向终止符号
			nStopIdx = i;
			nIsFindL = 2;
			break;
		}
		pCodeCol++;
	}

	// 找不到开始或终止位
	if( nIsFindL != 1 && nIsFindL != 2 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, cannot find start/~end in CodeStartStopMatch_code39\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 反向搜索条码终止位或反向开始符号位
	pCodeCol = decode_arr + arr_count - 1;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.2版本优化，限制左右检索边界，减少乱码
	//for(i = 0; i < arr_count - 9; i++) {
	for(i = 0; i < 6; i++) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol--;
			continue;
		}
		s = -pCodeCol[0] + pCodeCol[-1] - pCodeCol[-2] + pCodeCol[-3] - pCodeCol[-4] + pCodeCol[-5]
			- pCodeCol[-6] + pCodeCol[-7] - pCodeCol[-8];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
				printf( "Unexpected format of decode_arr in CodeStartStopMatch_code39, s=%d\n", s );
#endif
#endif
				nRet = -1;
				goto nExit;
		}

		//////////////////////////////////////////////////////////////////////////
		// 2.0.4.2版本改进
		// 旧版代码，注释
		// 对比反向开始位和终止位
// 		mask = 0;
// 		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;
// 		for( j = 0; j < 9; j++ ) {
// 			f[j] = abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT;
// 			r[j] = (f[j] > t) ? 1 : 0;
// 			mask |= (r[j] << (8-j));
// 		}

		//////////////////////////////////////////////////////////////////////////
		// 新版代码，针对EMS不规则条码进行改进，对长度进行排序得到阈值
		// 计算通用阈值
		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;

		// 对输入元素宽度进行排序
		for( j = 0; j < 9; j++ ) {
			sort[j] = abs(pCodeCol[-j]);
		}
		for( j = 0; j < 9; j++ ) {
			for( k = j+1; k < 9; k++ ) {
				if( sort[j] < sort[k] ) {
					tmp = sort[j];
					sort[j] = sort[k];
					sort[k] = tmp;
				}
			}
		}
		// 对边界宽度进行阈值检验
		if( (sort[2]<<FLOAT2FIXED_SHIFT_DIGIT) < t * 2/3 ||
			(sort[3]<<FLOAT2FIXED_SHIFT_DIGIT) > t * 4/3 ||
			sort[0] > sort[8] * 8 ) {
				pCodeCol--;
				continue;
		}
		// 计算阈值
		tmp = (sort[2] + sort[3]) >> 1;
		// 获取编码
		mask = 0;
		for( j = 0; j < 9; j++ ) {
			r[j] = (abs(pCodeCol[-j]) > tmp) ? 1 : 0;
			mask  |= (r[j] << (8-j));
		}
		// 2.0.4.2版本改进结束
		//////////////////////////////////////////////////////////////////////////

		if( mask == gnDecoderModuleCode39[43] ) {	// 发现反向开始符号
			nStartIdx = arr_count - i - 1;
			nIsFindR = 1;
			break;
		} else if( mask == gnDecoderModuleCode39[44]) {	// 发现终止符号
			nStopIdx = arr_count - i - 1;
			nIsFindR = 2;
			break;
		}
		pCodeCol--;
	}

	// 找不到开始或终止位
	if(nIsFindR != 1 && nIsFindR != 2) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, cannot find end/~start in CodeStartStopMatch_code39\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 两端找到相同的开始终止位
	if(3 != nIsFindL + nIsFindR) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("Cannot find code39, start&end do not match in CodeStartStopMatch_code39\n");
#endif
#endif
		nRet = -3;
		goto nExit;
	}

	// 顺序正确
	if(1 == nIsFindL) {
		*start_idx = nStartIdx;
		*stop_idx = nStopIdx;
		nRet = 1;
	} else {	// 顺序颠倒
		loop = arr_count >> 1;
		for (i = 0; i < loop; i++) {
			mask = decode_arr[i];
			decode_arr[i] = decode_arr[arr_count-i-1];
			decode_arr[arr_count-i-1] = mask; 
		}
		*start_idx = arr_count - 1 - nStartIdx;
		*stop_idx = arr_count - 1 - nStopIdx;
		nRet = 2;
	}

nExit:
	return nRet;
}

int CodeModelMatch_code39(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr)
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = decode_arr;

	int s = 0, t = 0, f[9] = {0}, r[9] = {0};
	int mask = 0, loop = (arr_count+1) / 10;

	int k = 0, tmp = 0, sort[9] = {0};

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	for( i = 0; i < loop; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf("Cannot match code39, unexpected value of pCodeCol[0] in CodeModelMatch_code39, \
				   pCodeCol[0]=%d (should be <0)\n", pCodeCol[0]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5]
			-pCodeCol[6] + pCodeCol[7] - pCodeCol[8];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf("Cannot match code39, unexpected value of s in CodeModelMatch_code39, \
				   s=%d (should be >0)\n", s);
#endif
#endif
			nRet = -1;
			goto nExit;
		}

		//////////////////////////////////////////////////////////////////////////
		// 2.0.4.2版本改进
		// 旧版代码，注释
		// 对比开始位和反向结束位
// 		mask = 0;
// 		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;
// 		for( j = 0; j < 9; j++ ) {
// 			f[j] = abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT;
// 			r[j] = (f[j] > t) ? 1 : 0;
// 			mask  |= (r[j] << (8-j));
// 		}

		//////////////////////////////////////////////////////////////////////////
		// 新版代码，针对EMS不规则条码进行改进，对长度进行排序得到阈值
		// 计算通用阈值
		t = (s << FLOAT2FIXED_SHIFT_DIGIT) >> 3;

		// 对输入元素宽度进行排序
		for( j = 0; j < 9; j++ ) {
			sort[j] = abs(pCodeCol[j]);
		}
		for( j = 0; j < 9; j++ ) {
			for( k = j+1; k < 9; k++ ) {
				if( sort[j] < sort[k] ) {
					tmp = sort[j];
					sort[j] = sort[k];
					sort[k] = tmp;
				}
			}
		}
		// 对边界宽度进行阈值检验
		if( (sort[2]<<FLOAT2FIXED_SHIFT_DIGIT) < t * 2/3 ||
			(sort[3]<<FLOAT2FIXED_SHIFT_DIGIT) > t * 4/3 ||
			sort[0] > sort[8] * 8 ) {
				nRet = -1;
				goto nExit;
		}
		// 计算阈值
		tmp = (sort[2] + sort[3]) >> 1;
		// 获取编码
		mask = 0;
		for( j = 0; j < 9; j++ ) {
			r[j] = (abs(pCodeCol[j]) > tmp) ? 1 : 0;
			mask  |= (r[j] << (8-j));
		}
		// 2.0.4.2版本改进结束
		//////////////////////////////////////////////////////////////////////////

		// 直接对比
		for( n = 0; n < 44; n++ ) {
			if( mask == gnDecoderModuleCode39[n] )
				break;
		}
		if( n < 44) {
			seq_arr[i] = n;
			faith_arr[i] = 1;	// 可靠系数1：直接匹配模板
		} else {
			seq_arr[i] = -1;
			faith_arr[i] = 0;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf("Cannot match code39 directly in CodeModelMatch_code39, seq=%d\n", i+1);
			printf("s:%5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d\n",\
				pCodeCol[0], pCodeCol[1], pCodeCol[2], pCodeCol[3], pCodeCol[4], pCodeCol[5],\
				pCodeCol[6], pCodeCol[7], pCodeCol[8]);
			printf("f:%5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d\n",\
				f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
			printf("r:%5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d  %5d\n",\
				r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8]);
#endif
#endif
		}
		pCodeCol += 10;
	}

	nRet = loop;

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
	printf("Decode seq_arr: ");
	for(i = 0; i < loop; i++)
		printf("%5d",seq_arr[i]);
	printf("\n");
#endif
#endif

nExit:
	return nRet;
}


// 校验位检验
int CheckDigitVerify_code39(int * seq_arr, int * faith_arr, int seq_cnt, int isCheckDigit)
{
	int nRet = 0;
	int i = 0, sum = 0, tmp = 0;
	int failIdx = 0, failCnt = 0;

	// 开始结束符号错误
	if(seq_arr[0] != 43 || seq_arr[seq_cnt-1] != 43) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("#CheckDigitVerify39# Unexpected Start-stop, seq_arr[0]=%d, seq_arr[seq_cnt-1]=%d",\
			seq_arr[0], seq_arr[seq_cnt-1]);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 无校验位校验
	if( !isCheckDigit ) {
		tmp = 0;
		for( i = 1; i < seq_cnt - 1; i++ ) {
			if( 1 != faith_arr[i] ) {
				tmp = i;
				break;
			}
		}
		if( 0 == tmp ) {
			nRet = 1;
			goto nExit;
		} 
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
		printf("#CheckDigitVerify39# Cannot pass CheckDigit directly, CheckDigit off and seq_arr[%d]=%d, faith_arr[%d]=%d\n",\
			tmp, seq_arr[tmp], tmp, faith_arr[tmp]);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 有校验位校验
	for( i = 1; i < seq_cnt - 2; i++ ) {
		if( 1 == faith_arr[i] ) {
			sum += seq_arr[i];
		} else {
			failCnt++;
			failIdx = i;
		}
	}

	if( 0 == failCnt) {
		i = sum % 43;
		if( i == seq_arr[seq_cnt-2] ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf("#CheckDigitVerify39# CheckDigitVerify succeed\n");
#endif
#endif
			nRet = 1;
			goto nExit;
		} else if( 1 != seq_arr[seq_cnt-2] ) {	// 校验位不可靠
			seq_arr[seq_cnt-2] = i;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
			printf("#CheckDigitVerify39# Cannot pass CheckDigit directly, change seq_arr[%d](vertify)=%d\n",\
				seq_cnt-2, seq_arr[seq_cnt-2]);
#endif
#endif
			nRet = 1;
			goto nExit;
		}
	} else if( 1 == failCnt && 1 == faith_arr[seq_cnt-2]) {
		for( i = 0; i < 43; i++ ) {
			tmp = sum + i;
			if( seq_arr[seq_cnt-2] == tmp % 43 ) {
				seq_arr[failIdx] = i;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE39
				printf("#CheckDigitVerify39# Cannot pass CheckDigit directly, change seq_arr[%d]=%d\n",\
					failIdx, seq_arr[failIdx]);
#endif
#endif
				nRet = 1;
				goto nExit;
			}
		}
	} else {
		nRet = 0;
		goto nExit;
	}

nExit:
	return nRet;
}

int Transcode_code39( int * seq_arr, int seq_cnt, char * strResult )
{
	int i = 0, nStrNum = 0;

	for (i = 1; i < seq_cnt-1; i++)
	{
		if(seq_arr[i] < 10) {
			strResult[nStrNum++] = seq_arr[i] + 48;
		}
		else if(seq_arr[i] < 36) {
			strResult[nStrNum++] = seq_arr[i] + 55;
		}
		else if(seq_arr[i] < 38) {
			strResult[nStrNum++] = seq_arr[i] + 9;
		}
		else if(seq_arr[i] == 38) {
			strResult[nStrNum++] = 32;
		}
		else if(seq_arr[i] == 39) {
			strResult[nStrNum++] = 36;
		}
		else if(seq_arr[i] == 40) {
			strResult[nStrNum++] = 47;
		}
		else if(seq_arr[i] == 41) {
			strResult[nStrNum++] = 43;
		}
		else if(seq_arr[i] == 42) {
			strResult[nStrNum++] = 37;
		}
	}

	strResult[nStrNum] = 0;

	return nStrNum;
}


