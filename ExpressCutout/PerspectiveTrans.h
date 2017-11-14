#ifndef PERSPECTIVE_TRANS_H
#define PERSPECTIVE_TRANS_H

/**********************************************************
	// 使用方法
	int srcX[8] = {0}, dstX[8] = {0};
	double matrix[9];
	srcX[0] = 0;			srcX[1] = 0; 
	srcX[2] = 0;			srcX[3] = dstHeight-1;
	srcX[4] = dstWidth-1;	srcX[5] = 0; 
	srcX[6] = dstWidth-1;	srcX[7] = dstHeight-1;

	dstX[0] = 0;			dstX[1] = 0; 
	dstX[2] = 0;			dstX[3] = srcHeight-1;
	dstX[4] = srcWidth-1;	dstX[5] = 0; 
	dstX[6] = srcWidth-1;	dstX[7] = srcHeight-1;

	GetPerspectiveTransformMat( srcX, dstX, matrix );
	status = WarpPerspectiveTransformFixed( srcImgPtr, srcWidth, srcHeight,	dstImgPtr, dstWidth, dstHeight, matrix );
***********************************************************/

int IPA_getPerspectiveTransformMat(const int* src, const int* dst, double* mat);

int IPA_warpPerspectiveTransformFixed(unsigned char * srcImgPtr, int srcWidth, int srcHeight,
									  unsigned char * dstImgPtr, int dstWidth, int dstHeight, double* matrix);

void ImageBinarization(unsigned char*src,int imgwidth,int imgheight,int *threshold);

#endif

