#ifndef BARCODE_DECODING_H
#define BARCODE_DECODING_H

#include "RyuCore.h"

/************************************************************************/
/* 识别错误返回标志                                                      */
/************************************************************************/
#define RYU_DECODERR_NULLPTR		(-1)
#define RYU_DECODERR_SHORTLEN		(-2)
#define RYU_DECODERR_NOST			(-3)
#define RYU_DECODERR_SHORTLEN_ST	(-4)
#define RYU_DECODERR_MATCHFAILED	(-5)
#define RYU_DECODERR_VERIFYFAILED	(-6)
#define RYU_DECODERR_TRANSCFAILED	(-7)

/************************************************************************/
/* 解码主模块	                                                        */
/************************************************************************/
typedef struct DecodeDemarcateNode
{
	int type;
	int idx_b;
	int idx_e;
	int idxex_b;
	int idxex_e;
	int acc;
	int max_v;
	int count;
	float gravity;
} DecodeDemarcateNode;

typedef struct DecodeResultNode
{
	int		flag;				// 标志
	int		count;				// 计数
	int		code_type;			// 条码类型
	int		rslt_digit;		// 字符位数
	int		code_module;		// 条码模块数
	int		code_unit;			// 条码单元数
	int		code_unit_r;		// 直接匹配单元数
	int		code_direct;		// 条码方向
	int		left_idx;			// 条码左侧偏移(像素)
	int		right_idx;			// 条码右侧偏移(像素)
	float	reliability;		// 信度
	char	strCodeData[CODE_RESULT_ARR_LENGTH];	// 解码结果
} DecodeResultNode;

/************************************************************************/
/* 解码模块调用接口                                                      */
/************************************************************************/
int BarcodeDecoding_init( int max_width, int max_height );

int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH, 
	char * code_result, int * code_type, int * char_num, int * module_num, 
	int * code_direct, int * leftOffset, int * rightOffset, float * minModule);

// v2.6版本开始使用的解码算法
int BarcodeDecoding_run( unsigned char * im, int * integr, int width, int height, 
						char * code_result, int * code_type, int * char_num, int * char_valid, int * module_num, 
						int * code_direct, int * leftOffset, int * rightOffset, float * minModule);

// int BarcodeDecoding_Integrogram( unsigned char * im, int * integr, int width, int height, int slice_height, 
// 								char * code_result, int * code_type, int * char_num, int * module_num, 
// 								int * code_direct, int * leftOffset, int * rightOffset);

void BarcodeDecoding_release();

/************************************************************************/
/* 解码模块验证功能														*/
/************************************************************************/

/************************************************************************/
/* Code128解码模块                                                      */
/************************************************************************/
int allocVariableMemStorage_code128(unsigned char * heapPtr, int heapSize);

void resetVariableMemStorage_code128();

int RecgCode128(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
				int * code_module, int * code_direct, int * code_idxL, int * code_idxR);

int Decoder_code128(int * decode_arr, int arr_count, DecodeResultNode * result);

/************************************************************************/
/* Code39解码模块                                                       */
/************************************************************************/
int allocVariableMemStorage_code39(unsigned char * heapPtr, int heapSize);

void resetVariableMemStorage_code39();

int RecgCode39(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
			   int * code_module, int * code_direct, int * code_idxL, int * code_idxR);

int Decoder_code39(int * decode_arr, int arr_count, DecodeResultNode * result);

/************************************************************************/
/* Code93解码模块                                                       */
/************************************************************************/
int allocVariableMemStorage_code93(unsigned char * heapPtr, int heapSize);

void resetVariableMemStorage_code93();

int RecgCode93(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
			   int * code_module, int * code_direct, int * code_idxL, int * code_idxR);

int Decoder_code93(int * decode_arr, int arr_count, DecodeResultNode * result);



#endif


