#ifndef DECODER_CODE128_H
#define DECODER_CODE128_H

#include "RyuCore.h"

int RecgCode128(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);


#endif
