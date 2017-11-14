#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "RyuCore.h"

#include "Decoder_code93.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
#include "OpenCv_debugTools.h"
#endif
#endif

#define CODE93_MIN_DECODEARR_COUNT		(31)
#define CODE93_MIN_SEQARR_COUNT			(3)
#define CODE93_MAX_MODULE_DEVIATION		(4<<FLOAT2FIXED_SHIFT_DIGIT)

const int gnDecoderModuleCode93[49] = {
	0x131112, 0x111213, 0x111312, 0x111411, 0x121113, 
	0x121212, 0x121311, 0x111114, 0x131211, 0x141111,
	0x211113, 0x211212, 0x211311, 0x221112, 0x221211, 
	0x231111, 0x112113, 0x112212, 0x112311, 0x122112,
	0x132111, 0x111123, 0x111222, 0x111321, 0x121122, 
	0x131121, 0x212112, 0x212211, 0x211122, 0x211221,
	0x221121, 0x222111, 0x112122, 0x112221, 0x122121, 
	0x123111, 0x121131, 0x311112, 0x311211, 0x321111,
	0x112131, 0x113121, 0x211131, 0x121221, 0x312111, 
	0x311121, 0x122211, 0x111141, 0x1111411/*stop*/
};

static int gnDecoderSeqArrCode93[128] = {0};
static int gnDecoderFaithArrCode93[128] = {0};

int CodeStartStopMatch_code93( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx );

int CodeModelMatch_code93(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr);

int CheckDigitVerify_code93(int * seq_arr, int * faith_arr, int seq_cnt);

int Transcode_code93( int * seq_arr, int seq_cnt, char * strResult );


int RecgCode93(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR)
{
	int nRet = 0, status = 0;

	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nStartIdx = 0, nStopIdx = 0;
	int nArrCount = 0, nSeqCount = 0;

	if(!decode_arr || !code_result || !code_digit || !code_module
		|| !code_direct || !code_idxL || !code_idxR) {
#ifdef	_PRINT_PROMPT
			printf( "ERROR! Invalid input of RecgCode93, decode_arr=0x%x, code_result=0x%x\
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

	if( arr_count < CODE93_MIN_DECODEARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, too small arr_count: %d\n", arr_count);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeStartStopMatch_code93( decode_arr, arr_count, &nStartIdx, &nStopIdx );
	if(1 != status && 2 != status) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf( "Unexpected return of CodeStartStopMatch_code93, cannot find code start/stop,\
				return=%d\n", status );
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nDirection = (1 == status) ? 1 : -1;		// 条码方向

	nArrCount = nStopIdx - nStartIdx + 1;	// 黑白数
	if( nArrCount < CODE93_MIN_DECODEARR_COUNT || 0 != (nArrCount-1) % 6) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, bad nColCount: %d\n", nArrCount);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeModelMatch_code93( decode_arr+nStartIdx, nArrCount, 
		gnDecoderSeqArrCode93, gnDecoderFaithArrCode93 );
	if( status < CODE93_MIN_SEQARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, bad nSeqCount: %d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nSeqCount = status;
	nModuleCnt = nSeqCount * 9 + 10;

	status = CheckDigitVerify_code93( gnDecoderSeqArrCode93, gnDecoderFaithArrCode93, nSeqCount );
	if(1 != status ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, check digit verification failed, return=%d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = Transcode_code93( gnDecoderSeqArrCode93, nSeqCount, code_result );
	if( status <= 0 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, bad return of Transcode_code93, return=%d\n", status);
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


int CodeStartStopMatch_code93( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = 0;

	int nStartIdx = 0, nStopIdx = 0;
	int nIsFindL = 0, nIsFindR = 0;
	int s = 0, f[7] = {0}, r[7] = {0};
	int mask = 0, loop = 0;

	int nVar = 0, nMin = 0, nMinSeq = 0;

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	*start_idx = *stop_idx = 0;

	// 寻找条码起始位或反向终止符号位
	pCodeCol = decode_arr;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.2版本优化，限制左右检索边界，减少乱码
	//for( i = 0; i < arr_count - 6; i++ ) {
	for( i = 0; i < 6; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol++;
			continue;
		}

		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code93\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比开始位
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 9 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}

		if( mask == gnDecoderModuleCode93[47] ) {
			nStartIdx = i;
			nIsFindL = 1;
		}

		if( nIsFindL ) 
			break;

		//寻找距离最近者
		nMin = 0x7fffffff;
		for( n = 0; n < 48; n++ ) {
			mask = gnDecoderModuleCode93[n];
			nVar = abs(f[0]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
				+ abs(f[1]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[2]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[3]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[4]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[5]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
			if( nVar < nMin ) {
				nMin = nVar;
				nMinSeq = n;
			}
		}

		s -= pCodeCol[6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code93\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比反向结束位
		mask = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (j*4));
		}
		if(mask == gnDecoderModuleCode93[48]) {
			nStopIdx = i;
			nIsFindL = 2;
			break;
		}

		// 计算距离
		mask = gnDecoderModuleCode93[48];
		nVar = abs(f[6]-((mask>>24&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[5]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[4]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[3]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[2]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[1]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[0]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
		if( nVar < CODE93_MAX_MODULE_DEVIATION || nMin < CODE93_MAX_MODULE_DEVIATION ) {
			if( nVar < nMin ) {
				nStopIdx = i;
				nIsFindL = 2;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
				printf( "Cannot find Code93 start/end directly on left, replaced with min deviation one(stop)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 47 ) {
				nStartIdx = i;
				nIsFindL = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
				printf( "Cannot find Code93 start/end directly on left, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			}
		}
		pCodeCol++;
	}

	// 找不到开始或终止位
	if( nIsFindL != 1 && nIsFindL != 2 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, cannot find start/~end in CodeStartStopMatch_code93\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 反向搜索条码终止位或反向开始符号位
	pCodeCol = decode_arr + arr_count - 1;
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.2版本优化，限制左右检索边界，减少乱码
	//for(i = 0; i < arr_count - 6; i++) {
	for(i = 0; i < 6; i++) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol--;
			continue;
		}
		s = -pCodeCol[0] + pCodeCol[-1] - pCodeCol[-2] + pCodeCol[-3] - pCodeCol[-4] + pCodeCol[-5];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code93\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比反向开始位
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 9 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}
		if( mask == gnDecoderModuleCode93[47] ) {
			nStartIdx = arr_count - 1 - i;
			nIsFindR = 1;
		}

		if( nIsFindR ) 
			break;

		//寻找距离最近者
		nMin = 0x7fffffff;
		for( n = 0; n < 48; n++ ) {
			mask = gnDecoderModuleCode93[n];
			nVar = abs(f[0]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
				+ abs(f[1]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[2]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[3]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[4]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
				+ abs(f[5]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
			if( nVar < nMin ) {
				nMin = nVar;
				nMinSeq = n;
			}
		}

		s -= pCodeCol[-6];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code93\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比结束位
		mask = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 10 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (j*4));
		}
		if(mask == gnDecoderModuleCode93[48]) {
			nStopIdx = arr_count - 1 - i;
			nIsFindR = 2;
			break;
		}

		// 计算距离
		mask = gnDecoderModuleCode93[48];
		nVar = abs(f[0]-((mask>>24&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[1]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[2]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[3]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[4]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[5]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[6]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
		if( nVar < CODE93_MAX_MODULE_DEVIATION || nMin < CODE93_MAX_MODULE_DEVIATION ) {
			if( nVar < nMin ) {
				nStopIdx = arr_count - 1 - i;
				nIsFindR = 2;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
				printf( "Cannot find Code93 start/end directly on right, replaced with min deviation one(stop)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 47 ) {
				nStartIdx = arr_count - 1 - i;
				nIsFindR = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
				printf( "Cannot find Code93 start/end directly on right, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			}
		}
		pCodeCol--;
	}

	// 找不到开始或终止位
	if(nIsFindR != 1 && nIsFindR != 2) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, cannot find end/~start in CodeStartStopMatch_code93\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 两端找到相同的开始终止位
	if(3 != nIsFindL + nIsFindR) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("Cannot find code93, start&end do not match in CodeStartStopMatch_code93\n");
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


int CodeModelMatch_code93(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr)
{
	int nRet = 0;
	int i = 0, j = 0, n = 0;
	int * pCodeCol = decode_arr;

	int s = 0, f[6] = {0}, r[6] = {0};
	int mask = 0, loop = arr_count / 6 - 1;

	int nVar = 0, nMin = 0, nMinCnt = 0, nMinIdx[8] = {0};

	const int half_fixed = 1 << (FLOAT2FIXED_SHIFT_DIGIT - 1);

	for( i = 0; i < loop; i++ ) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf("Cannot match code93, unexpected value of pCodeCol[0] in CodeModelMatch_code93, \
				   pCodeCol[0]=%d (should be <0)\n", pCodeCol[0]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf("Cannot match code93, unexpected value of s in CodeModelMatch_code93, \
				   s=%d (should be >0)\n", s);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 9 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}
		// 直接对比
		for( n = 0; n < 48; n++ ) {
			if( mask == gnDecoderModuleCode93[n] )
				break;
		}
		if( n < 48) {
			seq_arr[i] = n;
			faith_arr[i] = 3;	// 可靠系数3：直接匹配模板
		} else {
			// 距离最小
			nMinCnt = 0;
			nMin = 0x7fffffff;
			for( n = 0; n < 48; n++ ) {
				mask = gnDecoderModuleCode93[n];
				nVar = abs(f[0]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
					+ abs(f[1]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[2]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[3]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[4]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
					+ abs(f[5]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
				if( nVar < nMin ) {
					nMin = nVar;
					nMinCnt = 0;
					nMinIdx[nMinCnt] = n;
				} else if( nVar == nMin && nMinCnt < 7) {
					nMinIdx[++nMinCnt] = n;
				}
			}
			nMinCnt++;
			if( 0 == nMinCnt && nMin < CODE93_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 2;
			} else if( 0 <= nMinCnt && nMin < CODE93_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 1;
			} else {
				faith_arr[i] = 0;
			}
			seq_arr[i] = nMinIdx[0];
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
			printf("Cannot match code93 directly in CodeModelMatch_code93, seq=%d\n", i+1);
			printf("s:%5d  %5d  %5d  %5d  %5d  %5d\n",\
				pCodeCol[0], pCodeCol[1], pCodeCol[2], pCodeCol[3], pCodeCol[4], pCodeCol[5]);
			printf("f:%.3f  %.3f  %.3f  %.3f  %.3f  %.3f\n",\
				-pCodeCol[0]*9.0/s, pCodeCol[1]*9.0/s, -pCodeCol[2]*9.0/s, \
				pCodeCol[3]*9.0/s, -pCodeCol[4]*9.0/s, pCodeCol[5]*9.0/s);
			printf("r:%5d %5d %5d %5d %5d %5d\n", r[0], r[1], r[2], r[3], r[4], r[5]);

			printf("距离:nMin=%d, nMinCnt=%d, nMinIdx=%d, module=%x\n", nMin, nMinCnt, nMinIdx[0], \
				gnDecoderModuleCode93[nMinIdx[0]]);
#endif
#endif
		}
		pCodeCol += 6;
	}

	nRet = loop;

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
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
int CheckDigitVerify_code93(int * seq_arr, int * faith_arr, int seq_cnt)
{
	int nRet = 0;
	int nSum, i, nRemainder;
	int factor = 0, multiple = 0, multiplier = 0;

	int nloop = 0;
	int tLoopCnt = 0;

	//判断第一验证位是否正确
	nSum = 0;
	factor = tLoopCnt = seq_cnt - 3;
	for (i = 1; i <= tLoopCnt; i++) {
		multiple = (factor - 1) / 20;
		multiplier = factor - multiple * 20;
		nSum += multiplier * seq_arr[i];
		factor--;
	}

	nRemainder = nSum % 47;

	if(nRemainder != seq_arr[tLoopCnt+1]) {
		nRet = 0;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("#CheckDigitVerify93# Cannot pass CheckDigit\n");
#endif
#endif

		goto nExit;
	}

	//判断第二验证位是否正确
	nSum = 0;
	factor = tLoopCnt = seq_cnt - 2;
	for (i = 1; i <= tLoopCnt; i++) {
		multiple = (factor - 1) / 15;
		multiplier = factor - multiple * 15;
		nSum += multiplier * seq_arr[i];
		factor--;
	}

	nRemainder = nSum % 47;

	if(nRemainder == seq_arr[tLoopCnt+1]) {
		nRet = 1;
		goto nExit;
	} else {
		seq_arr[tLoopCnt] = nRemainder;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE93
		printf("#CheckDigitVerify93# Cannot pass CheckDigit, algorithm try to infer the failed one\n");
#endif
#endif
		nRet = 1;
		goto nExit;
	}

	nRet = 1;

nExit:
	return nRet;
}


int Transcode_code93( int * seq_arr, int seq_cnt, char * strResult )
{
	int i = 0, nRet = 0;
	int nDigit = 0;

	//对正确的条码进行解码
	for (i = 1; i < seq_cnt - 2; i++) {
		if(seq_arr[i] < 10) {
			strResult[nDigit++] = seq_arr[i] + 48;
		} else if(seq_arr[i] < 36) {
			strResult[nDigit++] = seq_arr[i] + 55;
		} else if(seq_arr[i] < 38) {
			strResult[nDigit++] = seq_arr[i] + 9;
		} else if(seq_arr[i] == 38) {
			strResult[nDigit++] = 32;
		} else if(seq_arr[i] == 39) {
			strResult[nDigit++] = 36;
		} else if(seq_arr[i] == 40) {
			strResult[nDigit++] = 47;
		} else if(seq_arr[i] == 41) {
			strResult[nDigit++] = 43;
		} else if(seq_arr[i] == 42) {
			strResult[nDigit++] = 37;
		} else if(seq_arr[i] == 43) {
			if(seq_arr[i+1] >= 10 && seq_arr[i+1] <= 35) {
				strResult[nDigit++] = seq_arr[i+1] - 9;
			} else {
				return -3;
			}
			i++;
		} else if(seq_arr[i] == 44) {
			if(seq_arr[i+1] >= 10 && seq_arr[i+1] <= 14) {
				strResult[nDigit++] = seq_arr[i+1] + 17;
			} else if(seq_arr[i+1] >= 15 && seq_arr[i+1] <= 19) {
				strResult[nDigit++] = seq_arr[i+1] + 44;
			} else if(seq_arr[i+1] >= 20 && seq_arr[i+1] <= 24) {
				strResult[nDigit++] = seq_arr[i+1] + 71;
			} else if(seq_arr[i+1] >= 25 && seq_arr[i+1] <= 29) {
				strResult[nDigit++] = seq_arr[i+1] + 98;
			} else if(seq_arr[i+1] == 30) {
				strResult[nDigit++] = 0;
			} else if(seq_arr[i+1] == 31) {
				strResult[nDigit++] = 64;
			} else if(seq_arr[i+1] == 32) {
				strResult[nDigit++] = 96;
			} else {
				return -3;
			}
			i++;
		} else if(seq_arr[i] == 45) {
			if(seq_arr[i+1] >= 10 && seq_arr[i+1] <= 35) {
				strResult[nDigit++] = seq_arr[i+1] + 23;
			} else {
				return -3;
			}
			i++;
		} else if(seq_arr[i] == 46) {
			if(seq_arr[i+1] >= 10 && seq_arr[i+1] <= 35) {
				strResult[nDigit++] = seq_arr[i+1] + 87;
			} else {
				return -3;
			}
			i++;
		} else {
			return -3;
		}
	}

	strResult[nDigit] = 0;
	nRet = nDigit;

nExit:
	return nRet;
}

