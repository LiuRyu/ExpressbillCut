#ifndef DECODER_I2OF5_H
#define DECODER_I2OF5_H

#include "RyuCore.h"

int RecgCodeI2of5(int * decode_arr, int arr_count, char * code_result, int * code_digit, 
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);


#endif