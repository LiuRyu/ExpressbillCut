/* 
 * windlyu 20140319 create 
 */
#ifndef BARCODE_IMAGEPROC_H
#define BARCODE_IMAGEPROC_H

#include "RyuCore.h"

int BarcodeImageproc_init(int max_width, int max_height);

int BarcodeImgProcess(unsigned char * in_data, int width, int height);

unsigned char * getBarcodeImgProcOutput();

int BarcodeImageproc_recombination( unsigned char * in_data, int width, int height );

int AutoContrast(unsigned char * img, unsigned char * rglr_img, int width, int height, 
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale, int opration);

int AutoContrastAnalyze(unsigned char * img, int width, int height, int widthstep, 
	float thre_ratio, int * low_scale, int * high_scale, int * grav_scale, int * min_scale, int * max_scale);

int ryuThreshold(unsigned char * img, unsigned char * bina, int width, int height, int widthstep, int thresh);


int BarcodeImgProcessIntegrogram(unsigned char * in_data, int width, int height);

void BarcodeImageproc_release();

#endif

