#ifndef BARCODE_DECODING_H
#define BARCODE_DECODING_H

#include "RyuCore.h"

int BarcodeDecoding_init( int max_width, int max_height );

// int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH, 
// 	char * code_result, int * code_type, int * char_num, int * module_num, 
// 	int * code_direct, int * leftOffset, int * rightOffset);
int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH, 
				  int * code_type, int * char_num, int * char_valid, int * module_num,
				  int * code_direct, int * leftOffset, int * rightOffset, char * code_result);

void BarcodeDecoding_release();

#endif


