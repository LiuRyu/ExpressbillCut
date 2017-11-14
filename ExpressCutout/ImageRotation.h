#ifndef IMAGE_ROTATION_H
#define IMAGE_ROTATION_H

#include "RyuCore.h"
/*
*angle����ת�ĽǶȣ��Զ�Ϊ��λ,ָ����ʱ����ת�Ƕ�
*srcPts:��Ч����������ĸ��˵����꣬xy�������ʽ
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


