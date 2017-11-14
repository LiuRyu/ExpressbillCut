#ifndef IMAGE_ROTATION_H
#define IMAGE_ROTATION_H

#include "RyuCore.h"
/*
*angle：旋转的角度，以度为单位,指按逆时针旋转角度
*srcPts:有效矩形区域的四个端点坐标，xy共存的形式
*/

int  ImageRotation_init(int max_wid, int max_hei);

void ImageRotation_release();

// int RotateImage(unsigned char* ucSrcImg, short sSrcImgWidth, short sSrcImgHeight,
// 	int * srcPts, int cAngle, short * usDstW,short * usDstH);
int RotateImage(unsigned char* ucSrcImg, short sSrcImgWidth, short sSrcImgHeight,
	RyuPoint * corner, int cAngle, int cZoom, short * usDstW,short * usDstH);

unsigned char * GetRotateImage();

unsigned char * GetRotateImage2();

unsigned char * GetRotateZoomImage();

#endif


