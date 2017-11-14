#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "RyuCore.h"

#include "Decoder_code128.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
#include "OpenCv_debugTools.h"
#endif
#endif

#define CODE128_MIN_DECODEARR_COUNT		(25)
#define CODE128_MIN_SEQARR_COUNT		(3)
#define CODE128_MAX_MODULE_DEVIATION	(4<<FLOAT2FIXED_SHIFT_DIGIT)

const int gnDecoderModuleCode128[107] = {
	0x212222, 0x222122, 0x222221, 0x121223, 0x121322, 0x131222, 0x122213, 0x122312, 0x132212, 0x221213,
	0x221312, 0x231212, 0x112232, 0x122132, 0x122231, 0x113222, 0x123122, 0x123221, 0x223211, 0x221132,
	0x221231, 0x213212, 0x223112, 0x312131, 0x311222, 0x321122, 0x321221, 0x312212, 0x322112, 0x322211,
	0x212123, 0x212321, 0x232121, 0x111323, 0x131123, 0x131321, 0x112313, 0x132113, 0x132311, 0x211313,
	0x231113, 0x231311, 0x112133, 0x112331, 0x132131, 0x113123, 0x113321, 0x133121, 0x313121, 0x211331,
	0x231131, 0x213113, 0x213311, 0x213131, 0x311123, 0x311321, 0x331121, 0x312113, 0x312311, 0x332111,
	0x314111, 0x221411, 0x431111, 0x111224, 0x111422, 0x121124, 0x121421, 0x141122, 0x141221, 0x112214,
	0x112412, 0x122114, 0x122411, 0x142112, 0x142211, 0x241211, 0x221114, 0x413111, 0x241112, 0x134111,
	0x111242, 0x121142, 0x121241, 0x114212, 0x124112, 0x124211, 0x411212, 0x421112, 0x421211, 0x212141,
	0x214121, 0x412121, 0x111143, 0x111341, 0x131141, 0x114113, 0x114311, 0x411113, 0x411311, 0x113141,
	0x114131, 0x311141, 0x411131, 0x211412, 0x211214, 0x211232, 0x2331112/*stop*/
};

static int gnDecoderSeqArrCode128[128] = {0};
static int gnDecoderFaithArrCode128[128] = {0};

int CodeStartStopMatch_code128( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx );

int CodeModelMatch_code128(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr);

int CheckDigitVerify_code128(int * seq_arr, int * faith_arr, int seq_cnt);

int Transcode_code128( int * seq_arr, int seq_cnt, char * strResult );


int RecgCode128(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
		int * code_module, int * code_direct, int * code_idxL, int * code_idxR)
{
	int nRet = 0, status = 0;

	int nDigitCnt = 0, nModuleCnt = 0, nDirection = 0;
	int nStartIdx = 0, nStopIdx = 0;
	int nArrCount = 0, nSeqCount = 0;

	if(!decode_arr || !code_result || !code_digit || !code_module
		|| !code_direct || !code_idxL || !code_idxR) {
#ifdef	_PRINT_PROMPT
			printf( "ERROR! Invalid input of RecgCode128, decode_arr=0x%x, code_result=0x%x\
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

	if( arr_count < CODE128_MIN_DECODEARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, too small arr_count: %d\n", arr_count);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeStartStopMatch_code128( decode_arr, arr_count, &nStartIdx, &nStopIdx );
	if(1 != status && 2 != status) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf( "Unexpected return of CodeStartStopMatch_code128, cannot find code start/stop,\
				return=%d\n", status );
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nDirection = (1 == status) ? 1 : -1;		// 条码方向
	
	nArrCount = nStopIdx - nStartIdx + 1;	// 黑白数
	if( nArrCount < CODE128_MIN_DECODEARR_COUNT || 0 != (nArrCount-1) % 6) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, bad nColCount: %d\n", nArrCount);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = CodeModelMatch_code128( decode_arr+nStartIdx, nArrCount, 
		gnDecoderSeqArrCode128, gnDecoderFaithArrCode128 );
	if( status < CODE128_MIN_SEQARR_COUNT ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, bad nSeqCount: %d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	nSeqCount = status;
	nModuleCnt = nSeqCount * 11 + 13;

	status = CheckDigitVerify_code128( gnDecoderSeqArrCode128, gnDecoderFaithArrCode128, nSeqCount );
	if(1 != status ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, check digit verification failed, return=%d\n", status);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	status = Transcode_code128( gnDecoderSeqArrCode128, nSeqCount, code_result );
	if( status <= 0 ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, bad return of Transcode_code128, return=%d\n", status);
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


int CodeStartStopMatch_code128( int * decode_arr, int arr_count,  int * start_idx, int * stop_idx )
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

	//////////////////////////////////////////////////////////////////////////
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
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code128\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比开始位
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 11 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}
		for( j = 103; j < 106; j++) {
			if( mask == gnDecoderModuleCode128[j] ) {
				nStartIdx = i;
				nIsFindL = 1;
			}
		}
		if( nIsFindL ) 
			break;

		//寻找距离最近者
		nMin = 0x7fffffff;
		for( n = 0; n < 106; n++ ) {
			mask = gnDecoderModuleCode128[n];
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
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code128\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比反向结束位
		mask = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 13 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (j*4));
		}
		if(mask == gnDecoderModuleCode128[106]) {
			nStopIdx = i;
			nIsFindL = 2;
			break;
		}

		// 计算距离
		mask = gnDecoderModuleCode128[106];
		nVar = abs(f[6]-((mask>>24&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[5]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[4]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[3]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[2]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[1]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[0]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
		if( nVar < CODE128_MAX_MODULE_DEVIATION || nMin < CODE128_MAX_MODULE_DEVIATION ) {
			if( nVar < nMin ) {
				nStopIdx = i;
				nIsFindL = 2;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on left, replaced with min deviation one(stop)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 103 ) {
				nStartIdx = i;
				nIsFindL = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on left, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 104 ) {
				nStartIdx = i;
				nIsFindL = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on left, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 105 ) {
				nStartIdx = i;
				nIsFindL = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on left, replaced with min deviation one(start)\n" );
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
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, cannot find start/~end in CodeStartStopMatch_code128\n");
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
	//for(i = 0; i < arr_count - 6; i++) {
	for(i = 0; i < 6; i++) {
		if( pCodeCol[0] > 0 ) {	// 规定从黑色开始
			pCodeCol--;
			continue;
		}
		s = -pCodeCol[0] + pCodeCol[-1] - pCodeCol[-2] + pCodeCol[-3] - pCodeCol[-4] + pCodeCol[-5];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code128\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比反向开始位
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 11 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}
		for( j = 103; j < 106; j++) {
			if( mask == gnDecoderModuleCode128[j] ) {
				nStartIdx = arr_count - 1 - i;
				nIsFindR = 1;
			}
		}
		if( nIsFindR ) 
			break;

		//寻找距离最近者
		nMin = 0x7fffffff;
		for( n = 0; n < 106; n++ ) {
			mask = gnDecoderModuleCode128[n];
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
#ifdef  _DEBUG_DECODER_CODE128
			printf( "Unexpected format of decode_arr in CodeStartStopMatch_code128\n" );
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		// 对比结束位
		mask = 0;
		for( j = 0; j < 7; j++ ) {
			f[j] = (abs(pCodeCol[-j]) << FLOAT2FIXED_SHIFT_DIGIT) * 13 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (j*4));
		}
		if(mask == gnDecoderModuleCode128[106]) {
			nStopIdx = arr_count - 1 - i;
			nIsFindR = 2;
			break;
		}

		// 计算距离
		mask = gnDecoderModuleCode128[106];
		nVar = abs(f[0]-((mask>>24&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[1]-((mask>>20&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[2]-((mask>>16&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[3]-((mask>>12&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[4]-((mask>>8&0xf)<<FLOAT2FIXED_SHIFT_DIGIT)) 
			+ abs(f[5]-((mask>>4&0xf)<<FLOAT2FIXED_SHIFT_DIGIT))
			+ abs(f[6]-((mask&0xf)<<FLOAT2FIXED_SHIFT_DIGIT));
		if( nVar < CODE128_MAX_MODULE_DEVIATION || nMin < CODE128_MAX_MODULE_DEVIATION ) {
			if( nVar < nMin ) {
				nStopIdx = arr_count - 1 - i;
				nIsFindR = 2;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on right, replaced with min deviation one(stop)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 103 ) {
				nStartIdx = arr_count - 1 - i;
				nIsFindR = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on right, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 104 ) {
				nStartIdx = arr_count - 1 - i;
				nIsFindR = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on right, replaced with min deviation one(start)\n" );
#endif
#endif
				break;
			} else if( nMinSeq == 105 ) {
				nStartIdx = arr_count - 1 - i;
				nIsFindR = 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf( "Cannot find Code128 start/end directly on right, replaced with min deviation one(start)\n" );
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
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, cannot find end/~start in CodeStartStopMatch_code128\n");
#endif
#endif
		nRet = 0;
		goto nExit;
	}

	// 两端找到相同的开始终止位
	if(3 != nIsFindL + nIsFindR) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("Cannot find code128, start&end do not match in CodeStartStopMatch_code128\n");
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


int CodeModelMatch_code128(int * decode_arr, int arr_count, int * seq_arr, int * faith_arr)
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
#ifdef  _DEBUG_DECODER_CODE128
			printf("Cannot match code128, unexpected value of pCodeCol[0] in CodeModelMatch_code128, \
				   pCodeCol[0]=%d (should be <0)\n", pCodeCol[0]);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		s = -pCodeCol[0] + pCodeCol[1] - pCodeCol[2] + pCodeCol[3] - pCodeCol[4] + pCodeCol[5];
		if( 0 >= s ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
			printf("Cannot match code128, unexpected value of s in CodeModelMatch_code128, \
				   s=%d (should be >0)\n", s);
#endif
#endif
			nRet = -1;
			goto nExit;
		}
		mask = 0;
		for( j = 0; j < 6; j++ ) {
			f[j] = (abs(pCodeCol[j]) << FLOAT2FIXED_SHIFT_DIGIT) * 11 / s;
			r[j] = (f[j] + half_fixed) >> FLOAT2FIXED_SHIFT_DIGIT;
			mask |= (r[j] << (20-4*j));
		}
		// 直接对比
		for( n = 0; n < 106; n++ ) {
			if( mask == gnDecoderModuleCode128[n] )
				break;
		}
		if( n < 106) {
			seq_arr[i] = n;
			faith_arr[i] = 3;	// 可靠系数3：直接匹配模板
		} else {
			// 距离最小
			nMinCnt = 0;
			nMin = 0x7fffffff;
			for( n = 0; n < 106; n++ ) {
				mask = gnDecoderModuleCode128[n];
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
			if( 0 == nMinCnt && nMin < CODE128_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 2;
			} else if( 0 <= nMinCnt && nMin < CODE128_MAX_MODULE_DEVIATION ) {
				faith_arr[i] = 1;
			} else {
				faith_arr[i] = 0;
			}
			seq_arr[i] = nMinIdx[0];
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
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
		}
		pCodeCol += 6;
	}

	nRet = loop;

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
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
int CheckDigitVerify_code128(int * seq_arr, int * faith_arr, int seq_cnt)
{
	int nRet = 0;
	int i = 0, sum = 0, tmp = 0;
	int failIdx = 0, failCnt = 0;

	sum = seq_arr[0];
	for( i = 1; i < seq_cnt - 1; i++ ) {
		sum += i * seq_arr[i];
	}
	i = sum % 103;

	if( i == seq_arr[seq_cnt-1] ) {
		nRet = 1;
		goto nExit;
	}

#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
	printf("#CheckDigitVerify128# Cannot pass CheckDigit directly, ALGO try to infer the failed one\n");
#endif
#endif

	sum = seq_arr[0];
	for( i = 1; i < seq_cnt - 1; i++ ) {
		if( 3 != faith_arr[i] ) {
			failCnt++;
			failIdx = i;
		} else {
			sum += i * seq_arr[i];
		}
	}

	if( 0 == failCnt && 3 != faith_arr[seq_cnt-1]) {	// 校验位为唯一不确定位
		i = sum % 103;
		seq_arr[seq_cnt-1] = i;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("#CheckDigitVerify128# Inference index=%d(vertify), Inference value seq_arr[index]=%d\n",\
			seq_cnt-1, seq_arr[seq_cnt-1]);
#endif
#endif
		nRet = 1;
		goto nExit;
	} else if( 3 == faith_arr[seq_cnt-1] && 1 == failCnt ) {
		// 如果只有一个不能确定，则可以直接推算出该值
		for( i = 0; i < 106; i++ ) {
			tmp = sum + i * failIdx;
			if( seq_arr[seq_cnt-1] == tmp % 103 ) {
				seq_arr[failIdx] = i;
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
				printf("#CheckDigitVerify128# Inference index=%d, Inference value seq_arr[%d]=%d\n", \
					failIdx, failIdx, seq_arr[failIdx]);
#endif
#endif
				nRet = 1;
				goto nExit;
			}
		}
	} else {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf("#CheckDigitVerify128# ALGO can not infer the failed ones, failCnt=%d\n", failCnt);
#endif
#endif
		nRet = 0;
		goto nExit;
	}

nExit:
	return nRet;
}


int Transcode_code128( int * seq_arr, int seq_cnt, char * strResult )
{
	int i = 0, nRet = 0;
	int codeType = 0;
	int nDigit = 0;

	if( 103 == seq_arr[0] )
		codeType = 1;
	else if( 104 == seq_arr[0] )
		codeType = 2;
	else if( 105 == seq_arr[0] )
		codeType = 3;
	else {
#ifdef	_DEBUG_
#ifdef  _DEBUG_DECODER_CODE128
		printf( "Unexpected seq_arr[0]:%d", seq_arr[0] );
#endif
#endif
		nRet = -1;
		goto nExit;
	}

	for (i = 1; i < seq_cnt - 1; i++) {
		if (codeType == 1) {
			if(seq_arr[i] < 99)  {
				strResult[nDigit++] = seq_arr[i] + 32;
			}
			else if(seq_arr[i] == 99) codeType=3;
			else if(seq_arr[i] == 100) codeType=2;
			continue;
		}

		if (codeType == 2) {
			if(seq_arr[i] < 99) {
				strResult[nDigit++] = seq_arr[i] + 32;
			}
			else if(seq_arr[i] == 99) codeType=3;
			else if(seq_arr[i] == 101) codeType=1;
			continue;
		}

		if(codeType == 3) {
			if(seq_arr[i] < 10) {
				strResult[nDigit++] = 48;
				strResult[nDigit++] = seq_arr[i] + 48;

			}
			else if(seq_arr[i]<100) {
				strResult[nDigit++] =(seq_arr[i] - seq_arr[i] % 10) / 10 + 48;
				strResult[nDigit++] = seq_arr[i] % 10 + 48;
			}
			else if(seq_arr[i]==100) codeType = 2;
			else if(seq_arr[i]==101) codeType = 1;
			continue;
		}
	}

	seq_arr[nDigit] = 0;

	nRet = nDigit;

nExit:
	return nRet;
}

