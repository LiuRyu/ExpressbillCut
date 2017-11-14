#ifndef EXPRESS_BILL_CUTOUT_H
#define EXPRESS_BILL_CUTOUT_H

#include "RyuCore.h"

int  ExBillCut_init(int input_width, int input_height);

int ExBillCut_run(unsigned char * im, int width, int height, int * code_bounds,
	int code_orient, unsigned char * bill, int * bill_width, int * bill_height);

void ExBillCut_release();

#endif  EXPRESS_BILL_CUTOUT_H