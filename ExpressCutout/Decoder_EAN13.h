#ifndef DECODER_EAN_H
#define DECODER_EAN_H

#include "RyuCore.h"

int RecgCodeEAN13(int * decode_arr, int * decode_arrproc, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);


#endif