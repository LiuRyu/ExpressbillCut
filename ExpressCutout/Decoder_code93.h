#ifndef DECODER_CODE93_H
#define DECODER_CODE93_H

#include "RyuCore.h"

int RecgCode93(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);


#endif