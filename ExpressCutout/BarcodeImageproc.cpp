#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

#include "ImageRotation.h"
#include "BarcodeImageproc.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_IMGPROC
#include "OpenCv_debugTools.h"
#endif
#endif

#define IMGPROC_AOTU_CONTRAST_THRESH	(0.05)
#define IMGPROC_DISTING_GRAYSCALE		(36)
#define IMGPROC_MAX_PIECE_CNT			(8)

int gnImgprocMaxWidth = 0;
int gnImgprocMaxHeight = 0;

unsigned char * gucImgprocBuff1 = 0;
unsigned char * gucImgprocOutput = 0;

int gnImgprocInitFlag = 0;

int USMSharpeningWithBinarize( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
	int amount, int thresh, int radius, int bina_thresh );

int USMSharpening( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
	int amount, int thresh, int radius);

int AutoContrast(unsigned char * img, unsigned char * rglr_img, int width, int height, 
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale, int opration);

int AutoContrastAnalyze(unsigned char * img, int width, int height, int widthstep, 
	float thre_ratio, int * low_scale, int * high_scale, int * grav_scale, int * min_scale, int * max_scale);

int ryuThreshold(unsigned char * img, unsigned char * bina, int width, int height, int widthstep, int thresh);

int GaussianBlur3x3(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int GaussianBlur5x5(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int GaussianBlur5x5_Fast(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int DilateDenoising(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int binarizeCentralLocal(unsigned char * im, int width, int height, 
	int * integrogram, RyuSize mask_sz, double alpha, unsigned char * binary);

int ImVerticalIntegrogram(unsigned char * in_data, int width, int height, int * integrogram);

int BarcodeImgProcess(unsigned char * in_data, int width, int height)
{
	int status = 0, nRet = 0, i = 0;

//	int piece = 0, pieceW[IMGPROC_MAX_PIECE_CNT] = {0}, thresh[IMGPROC_MAX_PIECE_CNT] = {0};
	int nTmp1 = 0, nTmp2 = 0, nTmp3 = 0;
	int bina_thresh = 0;

	unsigned char * pImg = 0, * pBuff = 0;
	unsigned char * out_data = gucImgprocOutput;

	int gravity = 0;

	int piece = 0, offsetW[128] = {0}, pieceW[128] = {0}, thresh[128] = {0};

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data || !out_data) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x, out_data=0x%x\n",
			in_data, out_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnImgprocMaxWidth || height > gnImgprocMaxHeight ) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height * 4), 8, 1);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*y))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = AutoContrast(in_data, out_data, width, height, IMGPROC_AOTU_CONTRAST_THRESH, 0, 0, &gravity, 1);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height)))[x]=out_data[y*width+x];
		}
	}
#endif
#endif

	//////////////////////////////////////////////////////////////////////////
	// 2.0.7.2更新，将分块和分块阈值计算放到锐化之前
	// 图像分块
	/*
	piece = width * 2 / height;
	piece = RYUMAX( 3, RYUMIN(piece, IMGPROC_MAX_PIECE_CNT) );	// 这里将最小块数限制为3
	nTmp1 = width / piece;
	nTmp2 = 0;
	for( i = 0; i < piece - 1; i++ ) {
		pieceW[i] = nTmp1;
		nTmp2 += nTmp1;
	}
	pieceW[piece-1] = width - nTmp2;


	nTmp3 = pieceW[0];
	for(i = 1; i < piece-1; i++) {
		pIn = in_data + nTmp3;
		nTmp3 += pieceW[i];

		// 获取二值化阈值
		AutoContrastAnalyze(pIn, pieceW[i], height, width, 0.1, &nTmp1, &nTmp2, &gravity, 0, 0);

		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
			if(nTmp2 <= 64)
				thresh[i] = nTmp2 + 1;	// 置阈值为大值(多数置黑)
			else
				thresh[i] = nTmp1;		// 置阈值为小值(多数置白)
		} else if(nTmp1 >= 192) {
			thresh[i] = nTmp1;			// 置阈值为小值(多数置白)
		} else {
			thresh[i] = gravity;	// 取重值
		}
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
		printf("idx=%d, min=%d, max=%d, oldthresh=%d, grav=%d, thresh=%d\n", i, nTmp1, nTmp2, (nTmp1+nTmp2)/2, gravity, thresh[i]);
#endif
#endif
	}
	thresh[0] = thresh[1];
	thresh[piece-1] = thresh[piece-2];
	*/
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 2.1.5更新，将分块宽度固定为32，使分块更细致，阈值更可靠
	
	piece = (width - 32) / 32 + 2;
	if(piece > 128 || width < 32) {
		nRet = -2;
		goto nExit;
	}
	nTmp1 = width % 32;
	nTmp2 = nTmp1 >> 1;
	nTmp3 = 16 + nTmp2;
	offsetW[0] = 0;
	pieceW[0] = nTmp3 + 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] = nTmp3 + (i-1) * 32 - 16;
		pieceW[i] = 64;
	}
	offsetW[piece-1] = nTmp3 + (piece-2) * 32 - 16;
	pieceW[piece-1] = nTmp1 - nTmp2 + 32;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];

		// 获取二值化阈值
		AutoContrastAnalyze(pImg, pieceW[i], height, width, 0.1, &nTmp1, &nTmp2, &nTmp3, 0, 0);

		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
			if(nTmp2 <= 64)
				thresh[i] = nTmp2 + 1;	// 置阈值为大值(多数置黑)
			else
				thresh[i] = nTmp1;		// 置阈值为小值(多数置白)
		} else if(nTmp1 >= 192) {
			thresh[i] = nTmp1;			// 置阈值为小值(多数置白)
		} else {
//			thresh[i] = gravity;	// 取重值
			thresh[i] = (nTmp1 + nTmp2) / 2;	// 取均值
			thresh[i] = (thresh[i] > nTmp3) ? nTmp3 : thresh[i];
		}
	}

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 更新前，在锐化同时二值化，固定阈值
	// 注释部分
//	status = USMSharpeningWithBinarize(in_data, in_data, width, height, 5, 0, 2, 128);
//	if( status <= 0 ) {
//#ifdef	_PRINT_PROMPT
//		printf("Warning! Unexpected return of USMSharpening, return=%d, BarcodeImgProcess exit\n",
//			status);
//#endif
//		nRet = 0;
//		goto nExit;
//	}
	//////////////////////////////////////////////////////////////////////////
	// 更新：将图像在条码方向上切为n块，每块计算二值化阈值，使用不同阈值进行二值化
	// 新增算法部分
 	status = USMSharpening(out_data, out_data, width, height, 5, 0, 2);
 	if( status <= 0 ) {
 #ifdef	_PRINT_PROMPT
 		printf("Warning! Unexpected return of USMSharpening, return=%d, BarcodeImgProcess exit\n",
 			status);
 #endif
 		nRet = 0;
 		goto nExit;
 	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height*2)))[x]=out_data[y*width+x];
		}
	}
#endif
#endif
	/*/////////////////////////////////////////////////////////////////////////
	// 2017.8.4使用局部二值化方法
	unsigned char * ucTemp = (unsigned char *)malloc(width * height);
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	// 算法测时启动
	ryuTimerStart(1);
#endif
#endif
	status = binarizeCentralLocal(out_data, width, height, (int *)gucImgprocBuff1, ryuSize(72, 72), 0.08, ucTemp);
	DilateDenoising(ucTemp, out_data, width, height);
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	// 算法测时停止
	lTimeCost[1] = ryuTimerStop(1);
	ryuThousandType(lTimeCost[1], sTimeCost);
	printf("--_tmain-- 算法2耗时: %s us\n", sTimeCost);
#endif
#endif
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of binarizeCentralLocal, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * localBinaIm = cvCreateImage(cvSize(width, height * 2), 8, 1);
	IplImage * localBinaIm_slice = cvCreateImageHeader(cvSize(width, height), 8, 1);
	localBinaIm_slice->imageData = localBinaIm->imageData;
	uc2IplImageGray(ucTemp, localBinaIm_slice);
	localBinaIm_slice->imageData = localBinaIm->imageData + height * localBinaIm->widthStep;
	uc2IplImageGray(out_data, localBinaIm_slice);
	cvNamedWindow("localBinaIm");
	cvShowImage("localBinaIm", localBinaIm);
	free(ucTemp);
#endif
#endif
	// 二值化方法 end
	/////////////////////////////////////////////////////////////////////////*/

/*
 	// 图像分块
 	piece = width * 2 / height;
 	piece = RYUMAX( 3, RYUMIN(piece, IMGPROC_MAX_PIECE_CNT) );	// 这里将最小块数限制为3
 	nTmp1 = width / piece;
 	nTmp2 = 0;
 	for( i = 0; i < piece - 1; i++ ) {
 		pieceW[i] = nTmp1;
 		nTmp2 += nTmp1;
 	}
 	pieceW[piece-1] = width - nTmp2;
 
	//////////////////////////////////////////////////////////////////////////
	// 2.0.4.1版本优化
	// 更新前，针对每块得到的高低值，取平均作为二值化阈值
	// 注释部分
//  	nTmp3 = 0;
//  	for(i = 0; i < piece; i++) {
//  		pIn = in_data + nTmp3;
//  		nTmp3 += pieceW[i];
//  		// 获取二值化阈值
//  		AutoContrastAnalyze(pIn, pieceW[i], height, width, IMGPROC_AOTU_CONTRAST_THRESH, &nTmp1, &nTmp2, 0);
// 
//  		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
//  			if(nTmp2 <= 64)
//  				bina_thresh = nTmp2 + 1;	// 置阈值为大值(多数置黑)
//  			else
//  				bina_thresh = nTmp1;		// 置阈值为小值(多数置白)
//  		} else if(nTmp1 >= 192) {
//  			bina_thresh = nTmp1;			// 置阈值为小值(多数置白)
//  		} else {
//  			bina_thresh = (nTmp1 + nTmp2) / 2;	// 取中间值
// 
//  		}
// 
//  		// 进行二值化操作
//  		ryuThreshold(pIn, pIn, pieceW[i], height, width, bina_thresh);
//  	}

	//////////////////////////////////////////////////////////////////////////
	// 更新：针对每块得到的重值，作为二值化阈值，且两端的块取其邻近块作为阈值
	// 修改算法部分
	nTmp3 = pieceW[0];
	for(i = 1; i < piece-1; i++) {
		pIn = in_data + nTmp3;
		nTmp3 += pieceW[i];

		// 获取二值化阈值
		AutoContrastAnalyze(pIn, pieceW[i], height, width, 0.1, &nTmp1, &nTmp2, &gravity);

		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
			if(nTmp2 <= 64)
				bina_thresh = nTmp2 + 1;	// 置阈值为大值(多数置黑)
			else
				bina_thresh = nTmp1;		// 置阈值为小值(多数置白)
		} else if(nTmp1 >= 192) {
			bina_thresh = nTmp1;			// 置阈值为小值(多数置白)
		} else {
			bina_thresh = gravity;	// 取重值
			//bina_thresh = (gravity * 2 + nTmp1 + nTmp2) >> 2;
		}
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
		printf("idx=%d, min=%d, max=%d, oldthresh=%d, grav=%d, thresh=%d\n", i, nTmp1, nTmp2, (nTmp1+nTmp2)/2, gravity, bina_thresh);
#endif
#endif
		// 进行二值化操作
		ryuThreshold(pIn, pIn, pieceW[i], height, width, bina_thresh);
		thresh[i] = bina_thresh;
	}

	// 首尾块操作
	pIn = in_data;
	ryuThreshold(pIn, pIn, pieceW[0], height, width, thresh[1]);

	pIn = in_data + nTmp3;
	ryuThreshold(pIn, pIn, pieceW[i], height, width, thresh[i-1]);
	//////////////////////////////////////////////////////////////////////////
*/
	//////////////////////////////////////////////////////////////////////////
	// 2.0.7.2更新，将分块和分块阈值计算放到锐化之前，这里直接阈值化
	/*
	nTmp3 = 0;
	for(i = 0; i < piece; i++) {
		pIn = in_data + nTmp3;
		nTmp3 += pieceW[i];
		ryuThreshold(pIn, pIn, pieceW[i], height, width, thresh[i]);
	}
	*/
	//////////////////////////////////////////////////////////////////////////

 	// XXX COMMENT by v2.3.3.6
 	/*
	//////////////////////////////////////////////////////////////////////////
	// 2.1.5更新，更改分块方式
	offsetW[0] = 0;
	pieceW[0] -= 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] += 16;
		pieceW[i] = 32;
	}
	offsetW[piece-1] += 16;
	pieceW[piece-1] -= 16;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];
		ryuThreshold(pImg, pImg, pieceW[i], height, width, thresh[i]);
	}
	//////////////////////////////////////////////////////////////////////////
	 */
 	// XXX COMMENT END


	// XXX MODIFY v2.3.3.6 注释上一段代码【2.1.5更新，更改分块方式】，增加去噪模块
	offsetW[0] = 0;
	pieceW[0] -= 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] += 16;
		pieceW[i] = 32;
	}
	offsetW[piece-1] += 16;
	pieceW[piece-1] -= 16;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];
		pBuff = gucImgprocBuff1 + offsetW[i];
		ryuThreshold(pImg, pBuff, pieceW[i], height, width, thresh[i]);
	}

	DilateDenoising(gucImgprocBuff1, out_data, width, height);
	// XXX MODIFY ENDING


	nRet = 1;

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height*3)))[x]=out_data[y*width+x];
		}
	}
	IplImage * iplImgproc3C = cvCreateImage(cvGetSize(iplImgproc), 8, 3);
	cvCvtColor(iplImgproc, iplImgproc3C, CV_GRAY2BGR);
// 	nTmp3 = pieceW[0];
// 	for( i = 1; i < piece; i++ ) {
// 		cvLine(iplImgproc3C, cvPoint(nTmp3, 0), cvPoint(nTmp3, iplImgproc3C->height-1), CV_RGB(0, 255, 0));
// 		nTmp3 += pieceW[i];
// 	}
	printf("Whole image gravity scale = %d\n", gravity);
	cvNamedWindow("Imgproc");
	cvShowImage("Imgproc", iplImgproc3C);
	cvWaitKey();
	cvReleaseImage(&iplImgproc);
	cvReleaseImage(&iplImgproc3C);
#endif
#endif

nExit:
	return nRet;
}

unsigned char * getBarcodeImgProcOutput()
{
	return gucImgprocOutput;
}

int BarcodeImgProcessIntegrogram(unsigned char * in_data, int width, int height)
{
	int status = 0, nRet = 0, i = 0;

	unsigned char * pImg = 0, * pBuff = 0;
	unsigned char * out_data = gucImgprocOutput;

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data || !out_data) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x, out_data=0x%x\n",
			in_data, out_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnImgprocMaxWidth || height > gnImgprocMaxHeight ) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height * 3), 8, 1);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*y))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = AutoContrast(in_data, in_data, width, height, IMGPROC_AOTU_CONTRAST_THRESH, 0, 0, 0, 1);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height)))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = ImVerticalIntegrogram(in_data, width, height, (int *)out_data);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	int * pOut = (int *)out_data;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height*2)))[x]=pOut[y*width+x] / (y+1);
		}
	}
	cvNamedWindow("Imgproc");
	cvShowImage("Imgproc", iplImgproc);
	cvWaitKey();
	cvReleaseImage(&iplImgproc);
#endif
#endif

	nRet = 1;

nExit:
	return nRet;
}

// 识别图像重组
int BarcodeImageproc_recombination( unsigned char * in_data, int width, int height )
{
	int i = 0, nRet = 0;
	unsigned char * pBuf = gucImgprocBuff1, * pImg1 = 0, * pImg2 = 0;
	int offsetW = width >> 1, offsetH = height >> 1;

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x\n",
			in_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0 
		|| width > gnImgprocMaxWidth || height > gnImgprocMaxHeight ) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height*2+16), 8, 1);
	IplImage * iplImgprocHeader = cvCreateImageHeader(cvSize(width, height), 8, 1);
	iplImgprocHeader->imageData = iplImgproc->imageData;
	uc2IplImageGray(in_data, iplImgprocHeader);
#endif
#endif

	pImg1 = in_data + (offsetH-1) * width + offsetW;
	pImg2 = in_data + offsetH * width + offsetW;
	for(i = 0; i < offsetH; i++) {
		memcpy(pBuf, pImg1, sizeof(unsigned char) * offsetW);
		memcpy(pImg1, pImg2, sizeof(unsigned char) * offsetW);
		memcpy(pImg2, pBuf, sizeof(unsigned char) * offsetW);
		pImg1 -= width;
		pImg2 += width;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	iplImgprocHeader->imageData = iplImgproc->imageData + (height+16) * iplImgproc->widthStep;
	uc2IplImageGray(in_data, iplImgprocHeader);
	cvNamedWindow("recombination");
	cvShowImage("recombination", iplImgproc);
	cvWaitKey();
	cvDestroyWindow("recombination");
	cvReleaseImage(&iplImgproc);
	cvReleaseImageHeader(&iplImgprocHeader);
#endif
#endif

	nRet = 1;

nExit:
	return nRet;
}

int BarcodeImageproc_init( int max_width, int max_height )
{
	int nsize = 0;

	if( gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Unexpected value of gnImgprocInitFlag in BarcodeImageproc_init\n");
#endif
		return -1;
	}

	if( max_width <= 0 || max_height <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImageproc_init, max_width=%d, max_height=%d\n",
			max_width, max_height);
#endif
		return -1;
	}

	gnImgprocMaxWidth = max_width;
	gnImgprocMaxHeight = max_height;
	nsize = gnImgprocMaxWidth * gnImgprocMaxHeight;

// 	gucImgprocBuff1 = (unsigned char *) malloc(  nsize * sizeof(unsigned char) );
	gucImgprocBuff1 = GetRotateZoomImage();
	if( !gucImgprocBuff1 ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gucImgprocBuff1 in BarcodeImageproc_init\n");
#endif
		return -1;
	}

//	gucImgprocOutput = (unsigned char *) malloc(  nsize * sizeof(unsigned char) );
	gucImgprocOutput = GetRotateImage2();
	if( !gucImgprocOutput ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Cannot alloc memory for gucImgprocBuff1 in BarcodeImageproc_init\n");
#endif
		return -1;
	}
	
	gnImgprocInitFlag = 1;
	return 1;
}

void BarcodeImageproc_release()
{
	gnImgprocInitFlag = 0;

// 	if(gucImgprocBuff1) {
// 		free(gucImgprocBuff1);
// 		gucImgprocBuff1 = 0;
//	}

// 	if(gucImgprocOutput) {
// 		free(gucImgprocOutput);
// 		gucImgprocOutput = 0;
// 	}

	gucImgprocBuff1 = 0;
	gucImgprocOutput = 0;

	return;
}

int USMSharpeningWithBinarize( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
		int amount, int thresh, int radius, int bina_thresh )
{
	int i = 0;

	int sub = 0;
	int nsize = wid * hei;

	unsigned char * pIn = in_data;
	unsigned char * pBlur = gucImgprocBuff1;
	unsigned char * pOut = out_data;

	if(1 != gnImgprocInitFlag) {
		return -1;
	}

	if(!in_data || !out_data) {
		return -1;
	}
	
	if(wid <= 0 || wid > gnImgprocMaxWidth || hei <= 0 || hei > gnImgprocMaxHeight) {
		return -1;
	}

	amount = RYUMAX( 0, RYUMIN(255, amount) );
	thresh = RYUMAX( 0, RYUMIN(255, thresh) );

	switch (radius)
	{
	case (1):
		GaussianBlur3x3(pIn, pBlur, wid, hei);

	case (2):
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);

	default:
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);
	}

	for(i = nsize; i > 0; i--) {
		sub = *pIn - *pBlur;
		if(abs(sub) > thresh) {
			sub = *pIn + amount * sub;
// 			sub = (sub > 255) ? 255 : sub;
// 			sub = (sub < 0) ? 0 : sub;
// 			*pOut = sub;
		} else {
//			*pOut = *pIn;
			sub = *pIn;
		}

		*pOut = (sub > bina_thresh) ? 0xff : 0;

		pIn++;
		pBlur++;
		pOut++;
	}

	return 1;
}


int USMSharpening( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
		int amount, int thresh, int radius)
{
	int i = 0;

	int sub = 0;
	int nsize = wid * hei;

	unsigned char * pIn = in_data;
	unsigned char * pBlur = gucImgprocBuff1;
	unsigned char * pOut = out_data;

	if(1 != gnImgprocInitFlag) {
		return -1;
	}

	if(!in_data || !out_data) {
		return -1;
	}

	if(wid <= 0 || wid > gnImgprocMaxWidth || hei <= 0 || hei > gnImgprocMaxHeight) {
		return -1;
	}

	amount = RYUMAX( 0, RYUMIN(255, amount) );
	thresh = RYUMAX( 0, RYUMIN(255, thresh) );

	switch (radius)
	{
	case (1):
		GaussianBlur3x3(pIn, pBlur, wid, hei);

	case (2):
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);

	default:
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);
	}

	for(i = nsize; i > 0; i--) {
		sub = *pIn - *pBlur;
		if(abs(sub) > thresh) {
			sub = *pIn + amount * sub;
		} else {
			sub = *pIn;
		}

		*pOut = RYUMAX(0, RYUMIN(255, sub));

		pIn++;
		pBlur++;
		pOut++;
	}

	return 1;
}


int AutoContrast(unsigned char * img, unsigned char * rglr_img, int width, int height, 
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale, int opration)
{
	int gHistNorm_hist[256] = {0};
	int gHistNorm_eqHist[256] = {0};
	int * hist = gHistNorm_hist;
	int * eqHist = gHistNorm_eqHist;

	int ret_val = 0;
	int i = 0, j = 0;
	int l_index = 0, r_index = 255;
	int temp = 0, temp2 = 0, temp3 = 0, m = 0, n = 0;
	int size = height * width;
	int threshold = size * thre_ratio;

	float avg = 0, acc = 0;

	unsigned char * pImg = 0;
	unsigned char * pNorm = 0;
	int * pHist = 0;
	int * pEqHist = 0;

// 	memset(hist, 0, sizeof(int)*256);
// 	memset(eqHist, 0, sizeof(int)*256);

	pImg = img;
	for (i = size; i > 0; i--) //计算图像亮度直方图
	{
		hist[*pImg]++;
		pImg++;
	}

	// 由于0一般都是旋转填充引起的，所以去掉0的累积
	temp = 0;
	pHist = hist + 1;
	for(i = 255; i > 0; i--) {
		temp += (*pHist);
		pHist++;
		if(temp >= threshold) {
			l_index = 256 - i;
			break;
		}
	}

	// 由于255一般都是反光引起的，所以去掉255的累积
	temp = 0;
	pHist = hist + 254;
	for(i = 255; i > 0; i--) {
		temp += (*pHist);
		pHist--;
		if(temp >= threshold) {
			r_index = i - 1;
			break;
		}
	}

	m = r_index - l_index;
	n = 255 - m;

	if(min_scale && max_scale) {
		*min_scale = l_index;
		*max_scale = r_index;
	}

	if(grav_scale) {
		temp = temp2 = 0;
		pHist = hist + 254;
		for(i = 255; i > 0; i--) {
			temp += (*pHist);
			temp2 += (*pHist) * i;
			pHist--;
		}
		if(temp > 0) {
			*grav_scale = temp2 / temp;
		} else {
			*grav_scale = 0;
		}
	}

	if(1 != opration) {
		ret_val = 0;
		goto nExit;
	}

	if(m < 0 || n < 0) {
		ret_val = -1;
		goto nExit;
	}

	if(m <= 0 || n <= 0) {
		if(img == rglr_img) {
			ret_val = 1;
			goto nExit;
		}
		else {
			memcpy(rglr_img, img, size * sizeof(unsigned char));
			ret_val = 1;
			goto nExit;
		}
	}

	pEqHist = eqHist;
	for(i = l_index + 1; i > 0; i--) {
		*(pEqHist++) = 0;
	}

	pEqHist = eqHist + 255;
	for(i = 255 - r_index + 1; i > 0; i--) {
		*(pEqHist--) = 255;
	}

	if(m < n) {
		avg = n * 1.0 / m;
		acc = 1;
		pEqHist = eqHist + l_index + 1;
		for(i = m - 1; i > 0; i--) {
			acc = acc + avg;
			*(pEqHist++) = acc + 0.5;
			acc = acc + 1;
		}
	} else {
		avg = m * 1.0 / n;
		acc = avg / 2;
		temp = temp2 = 0;
		temp3 = 1;
		pEqHist = eqHist + l_index + 1;
		for(i = n; i > 0; i--) {
			temp = acc + 0.5;
			temp -= temp2;
			for(j = temp; j > 0; j--) {
				*(pEqHist++) = (temp3++);
			}
			temp2 += temp;
			temp3++;
			acc = acc + avg;
		}

		for(i = m - temp2; i > 0; i--) {
			*(pEqHist++) = (temp3++);
		}
	}

	pImg = img;
	pNorm = rglr_img;
	for (i = size; i > 0; i--) //进行灰度映射均衡化
	{
		*pNorm = eqHist[*pImg];
		pNorm++;
		pImg++;
	}

	ret_val = 2;

nExit:
	return ret_val;
}


int AutoContrastAnalyze(unsigned char * img, int width, int height, int widthstep, 
		float thre_ratio, int * low_scale, int * high_scale, int * grav_scale, 
		int * min_scale, int * max_scale)
{
	int gHistNorm_hist[256] = {0};
	int * hist = gHistNorm_hist;

	int i = 0, j = 0;
	int index = 0;
	int temp = 0, temp2 = 0;
	int size = height * width;
	int threshold = size * thre_ratio;

	unsigned char * pImg = 0, * pImgL = img;
	int * pHist = 0;

	// 统计图像亮度直方图
	for(j = 0; j < height; j++) {
		pImg = pImgL;
		for(i = 0; i < width; i++) {
			hist[*pImg]++;
			pImg++;
		}
		pImgL += widthstep;
	}

	if(min_scale) {
		index = temp = 0;
		pHist = hist;
		for(i = 0; i < 256; i++) {
			temp += (*pHist);
			pHist++;
			if(temp > 0) {
				index = i;
				break;
			}
		}
		*min_scale = index;
	}

	if(max_scale) {
		index = temp = 0;
		pHist = hist + 255;
		for(i = 255; i >= 0; i--) {
			temp += (*pHist);
			pHist--;
			if(temp > 0) {
				index = i;
				break;
			}
		}
		*max_scale = index;
	}

	if(low_scale && threshold > 0) {
		index = temp = 0;
		pHist = hist + 1;
		for(i = 0; i < 255; i++) {
			temp += (*pHist);
			pHist++;
			if(temp >= threshold) {
				index = i;
				break;
			}
		}
		*low_scale = index;
	}

	if(high_scale) {
		index = temp = 0;
		pHist = hist + 254;
		for(i = 255; i > 0; i--) {
			temp += (*pHist);
			pHist--;
			if(temp >= threshold) {
				index = i;
				break;
			}
		}
		*high_scale = index;
	}

	//////////////////////////////////////////////////////////////////////////
	// 2.0.7.1修正参数，将255像素计算在内
	if(grav_scale) {
		temp = temp2 = 0;
		pHist = hist + 255;
		for(i = 255; i >= 0; i--) {
			temp += (*pHist);
			temp2 += (*pHist) * i;
			pHist--;
		}

		if(temp > 0) {
			*grav_scale = temp2 / temp;
		} else {
			*grav_scale = 0;
		}
	}

	return 1;
}


int ryuThreshold(unsigned char * img, unsigned char * bina, int width, int height, int widthstep, int thresh)
{
	int i = 0, j = 0;
	unsigned char * pImg = 0, * pImgL = img;
	unsigned char * pBina = 0, * pBinaL = bina;

	for(j = 0; j < height; j++) {
		pImg = pImgL;
		pBina = pBinaL;
		for(i = 0; i < width; i++) {
			*pBina = (*pImg > thresh) ? 255 : 0;
			pImg++;
			pBina++;
		}
		pImgL += widthstep;
		pBinaL += widthstep;
	}

	return 1;
}


int GaussianBlur3x3(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 2; 
	int nw		= wid - 2;

	int nsize = wid * hei;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b;
	unsigned char * poffset, * poffset_t, * poffset_b;

	loffset = loffset_t = loffset_b = 0;
	poffset = poffset_t = poffset_b = 0;

	memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * wid);

	loffset		= pIn + 1;
	loffset_t	= pIn - wid + 1;
	loffset_b	= pIn + wid + 1;
	lOut		= pOut + 1;

	for(i = nh; i > 0; i--)	
	{
		loffset		+= wid;
		loffset_t	+= wid;
		loffset_b	+= wid;
		poffset		= loffset;
		poffset_t	= loffset_t;
		poffset_b	= loffset_b;

		lOut += wid;
		pOut = lOut;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t-1))	+ ((*poffset_t)<<1)	+ (*(poffset_t+1))
				+ ((*(poffset-1))<<1)	+ ((*poffset)<<2)	+ ((*(poffset+1))<<1)
				+ (*(poffset_b-1))	+ ((*poffset_b)<<1)	+ (*(poffset_b+1));

			*pOut = (t>>4);

			poffset++;
			poffset_t++;
			poffset_b++;
			pOut++;
		}

		*(lOut-1) = *lOut;	// 首元素处理
		*pOut = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+1, poffset+1, sizeof(unsigned char) * wid);

	return 0;
}

int GaussianBlur5x5(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 4; 
	int nw		= wid - 4;

	int nsize = wid * hei;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b, * loffset_t2, * loffset_b2;
	unsigned char * poffset, * poffset_t, * poffset_b, * poffset_t2, * poffset_b2;

	loffset = loffset_t = loffset_b = loffset_t2 = loffset_b2 = 0;
	poffset = poffset_t = poffset_b = poffset_t2 = poffset_b2 = 0;

	memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * (wid<<1));

	loffset_t2	= pIn + 2;
	loffset_t	= loffset_t2 + wid;
	loffset		= loffset_t + wid;
	loffset_b	= loffset + wid;
	loffset_b2	= loffset_b + wid;
	lOut		= pOut + (wid<<1) + 2;

	for(i = nh; i > 0; i--)	
	{
		poffset_t2  = loffset_t2;
		poffset_t	= loffset_t;
		poffset		= loffset;
		poffset_b	= loffset_b;
		poffset_b2	= loffset_b2;

		loffset_t2	+= wid;
		loffset_t	+= wid;
		loffset		+= wid;
		loffset_b	+= wid;
		loffset_b2	+= wid;

		pOut = lOut;
		lOut += wid;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t2-2))		+ ((*(poffset_t2-1))<<2) + ((*(poffset_t2))*7) + ((*(poffset_t2+1))<<2) + (*(poffset_t2+2))
				+ ((*(poffset_t-2))<<2)	+ ((*(poffset_t-1))<<4)	 + ((*(poffset_t))*26) + ((*(poffset_t+1))<<4)	+ ((*(poffset_t+2))<<2)
				+ ((*(poffset-2))*7)	+ ((*(poffset-1))*26)	 + ((*(poffset))*41)   + ((*(poffset+1))*26)	+ ((*(poffset+2))*7)
				+ ((*(poffset_b-2))<<2)	+ ((*(poffset_b-1))<<4)	 + ((*(poffset_b))*26) + ((*(poffset_b+1))<<4)	+ ((*(poffset_b+2))<<2)
				+ (*(poffset_b2-2))		+ ((*(poffset_b2-1))<<2) + ((*(poffset_b2))*7) + ((*(poffset_b2+1))<<2) + (*(poffset_b2+2));

			*pOut = t / 273;

			poffset_t2++;
			poffset_t++;
			poffset++;
			poffset_b++;
			poffset_b2++;
			pOut++;
		}

		*(lOut-1) = *lOut;		// 首元素处理
		*(lOut-2) = *lOut;		// 首元素处理
		*pOut = *(pOut-1);		// 尾元素处理
		*(pOut+1) = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+2, poffset+2, sizeof(unsigned char) * (wid<<1));

	return 0;
}

int GaussianBlur5x5_Fast(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 4; 
	int nw		= wid - 4;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b, * loffset_t2, * loffset_b2;
	unsigned char * poffset, * poffset_t, * poffset_b, * poffset_t2, * poffset_b2;

	loffset = loffset_t = loffset_b = loffset_t2 = loffset_b2 = 0;
	poffset = poffset_t = poffset_b = poffset_t2 = poffset_b2 = 0;

	//memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * (wid<<1));

	loffset_t2	= pIn + 2;
	loffset_t	= loffset_t2 + wid;
	loffset		= loffset_t + wid;
	loffset_b	= loffset + wid;
	loffset_b2	= loffset_b + wid;
	lOut		= pOut + (wid<<1) + 2;

	for(i = nh; i > 0; i--)	
	{
		poffset_t2  = loffset_t2;
		poffset_t	= loffset_t;
		poffset		= loffset;
		poffset_b	= loffset_b;
		poffset_b2	= loffset_b2;

		loffset_t2	+= wid;
		loffset_t	+= wid;
		loffset		+= wid;
		loffset_b	+= wid;
		loffset_b2	+= wid;

		pOut = lOut;
		lOut += wid;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t2-2))		+ ((*(poffset_t2-1))<<2) + ((*(poffset_t2))<<3) + ((*(poffset_t2+1))<<2) + (*(poffset_t2+2))
				+ ((*(poffset_t-2))<<2)	+ ((*(poffset_t-1))<<4)	 + ((*(poffset_t))<<5) + ((*(poffset_t+1))<<4)	+ ((*(poffset_t+2))<<2)
				+ ((*(poffset-2))<<3)	+ ((*(poffset-1))<<5)	 + ((*(poffset))<<6)   + ((*(poffset+1))<<5)	+ ((*(poffset+2))<<3)
				+ ((*(poffset_b-2))<<2)	+ ((*(poffset_b-1))<<4)	 + ((*(poffset_b))<<5) + ((*(poffset_b+1))<<4)	+ ((*(poffset_b+2))<<2)
				+ (*(poffset_b2-2))		+ ((*(poffset_b2-1))<<2) + ((*(poffset_b2))<<3) + ((*(poffset_b2+1))<<2) + (*(poffset_b2+2));

			*pOut = t / 324;

			poffset_t2++;
			poffset_t++;
			poffset++;
			poffset_b++;
			poffset_b2++;
			pOut++;
		}

		*(lOut-1) = *lOut;		// 首元素处理
		*(lOut-2) = *lOut;		// 首元素处理
		*pOut = *(pOut-1);		// 尾元素处理
		*(pOut+1) = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+2, poffset+2, sizeof(unsigned char) * (wid<<1));

	return 0;
}


// 用变形膨胀法对二值图像进行去噪，去除栅栏间噪声
int DilateDenoising(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 2;
	int nw		= wid - 2;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b;
	unsigned char * poffset, * poffset_t, * poffset_b;

	loffset = loffset_t = loffset_b = 0;
	poffset = poffset_t = poffset_b = 0;

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * wid);

	loffset		= pIn + 1;
	loffset_t	= pIn - wid + 1;
	loffset_b	= pIn + wid + 1;
	lOut		= pOut + 1;

	for(i = nh; i > 0; i--)
	{
		loffset		+= wid;
		loffset_t	+= wid;
		loffset_b	+= wid;
		poffset		= loffset;
		poffset_t	= loffset_t;
		poffset_b	= loffset_b;

		lOut += wid;
		pOut = lOut;

		for(j = nw; j > 0; j--)
		{
			if(0 == *poffset) {
				t = (*(poffset_t-1) == *poffset) + (*poffset_t == *poffset) + (*(poffset_t+1) == *poffset)
					+ (*(poffset-1) == *poffset) + (*(poffset+1) == *poffset)
					+ (*(poffset_b-1) == *poffset) + (*poffset_b == *poffset) + (*(poffset_b+1) == *poffset);

				*pOut = (2 >= t) ? 255 : 0;
			} else {
				*pOut = 255;
			}

			poffset++;
			poffset_t++;
			poffset_b++;
			pOut++;
		}

		*(lOut-1) = *lOut;	// 首元素处理
		*pOut = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+1, poffset+1, sizeof(unsigned char) * wid);

	return 0;
}

// 求取图像的积分图像
int acquireImageIntegrogram(unsigned char * im, int width, int height, unsigned int * out)
{
	unsigned char * pIm = 0, * pImL = im;
	unsigned int * pOut = 0, * pOutL = out;
	int i = 0, j = 0;

	if(NULL == im || NULL == out) {
		return -1;
	}

	if(0 >= width || 0 >= height) {
		return -1;
	}

	// 建立首行积分图
	*pOutL = (unsigned int)(*pImL);		// 行首元素
	pIm = pImL + 1;
	pOut = pOutL + 1;
	for(i = 1; i < width; i++) {
		*pOut = *(pOut-1) + *pIm;
		pIm++;
		pOut++;
	}
	pImL += width;
	pOutL += width;
	// 建立积分图
	for(j = 1; j < height; j++) {
		*pOutL = *(pOutL-width) + *pImL;		// 行首元素
		pIm = pImL + 1;
		pOut = pOutL + 1;
		for(i = 1; i < width; i++) {
			*pOut = *(pOut-width) + *(pOut-1) + *pIm - *(pOut-width-1);
			pIm++;
			pOut++;
		}
		pImL += width;
		pOutL += width;
	}
	return 1;
}

static int getImrectLuminanceSum(unsigned int * integrogram, int width, int height, RyuRect rc)
{
	unsigned int * pData1 = integrogram + (rc.y-1) * width + rc.x - 1;
	unsigned int * pData2 = pData1 + rc.height * width;
	int sum = ((0 == rc.x || 0 == rc.y) ? 0 : pData1[0]) 
		+ pData2[rc.width] 
	- ((0 == rc.y) ? 0 : pData1[rc.width]) 
		- ((0 == rc.x) ? 0 : pData2[0]);

	return sum;
}

int binarizeCentralLocal(unsigned char * im, int width, int height, 
	int * integrogram, RyuSize mask_sz, double alpha, unsigned char * binary)
{
	int status = 0;
	int i = 0, j = 0, thre = 0;
	int falpha = (int)(1024 - alpha * 1024);
	unsigned char * pIm = 0, * pImL = im;
	unsigned char * pBina = 0, * pBinaL = binary;

	RyuSize radius = ryuSize(mask_sz.width>>1, mask_sz.height>>1);
	int rctAreas = 0, increment = radius.height;
	unsigned int * pIntr1_f = 0, * pIntr1_b = 0, * pIntrL1 = (unsigned int *)integrogram;
	unsigned int * pIntr2_f = 0, * pIntr2_b = 0, * pIntrL2 = (unsigned int *)integrogram + radius.height * width;

	RyuRect rc;
	RyuPoint pt;

	if(NULL == binary) {
		return -1;
	}

	if(0 >= width || 0 >= height) {
		return -1;
	}

	if(0 >= mask_sz.width || 0 >= mask_sz.height) {
		return -1;
	}

	status = acquireImageIntegrogram(im, width, height, (unsigned int *)integrogram);
	if(1 != status) {
		return -1;
	}

	// 若图像高度较小，使用原始算法，不做快速运算处理
	if(height <= 2 * radius.height + 1) {
		// 二值化操作
		for(j = 0; j < height; j++) {
			pIm = pImL;
			pBina = pBinaL;
			// 二值化阈值计算区域范围
			rc.y = j - radius.height;
			pt.y = j + radius.height;
			if(0 > rc.y)
				rc.y = 0;
			if(height - 1 < pt.y)
				pt.y = height - 1;
			rc.height = pt.y - rc.y + 1;
			for(i = 0; i < width; i++) {
				rc.x = i - radius.width;
				pt.x = i + radius.width;
				if(0 > rc.x)
					rc.x = 0;
				if(width - 1 < pt.x)
					pt.x = width - 1;
				rc.width = pt.x - rc.x + 1;
				thre = getImrectLuminanceSum((unsigned int *)integrogram, width, height, rc);
				thre /= (rc.width * rc.height);		
				thre = (falpha * thre) >> 10;
				*pBina = (*pIm < thre) ? 0 : 255;
				pIm++;
				pBina++;
			}
			pImL += width;
			pBinaL += width;
		}
		return 1;
	}

	// Y上界区域
	for(j = 0; j <= radius.height; j++) {
		pIm = pImL;
		pBina = pBinaL;
		pIntr2_f = pIntrL2;
		pIntr2_b = pIntrL2 + radius.width;
		++increment;
		rctAreas = radius.width * increment;
		// X左界区域
		for(i = 0; i <= radius.width; i++) {
			rctAreas += increment;
			thre = *(pIntr2_b++) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X中间区域
		for( ; i < width-radius.width-1; i++) {
			thre = (*(pIntr2_b++) - *(pIntr2_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X右界区域
		for( ; i < width; i++) {
			thre = (*pIntr2_b - *(pIntr2_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
			rctAreas -= increment;
		}
		pImL += width;
		pBinaL += width;
		pIntrL2 += width;
	}

	// Y中间区域
	rctAreas = radius.width * increment;
	for( ; j < height-radius.height-1; j++) {
		pIm = pImL;
		pBina = pBinaL;
		pIntr1_f = pIntrL1;
		pIntr1_b = pIntrL1 + radius.width;
		pIntr2_f = pIntrL2;
		pIntr2_b = pIntrL2 + radius.width;
		// X左界区域
		for(i = 0; i <= radius.width; i++) {
			rctAreas += increment;
			thre = (*(pIntr2_b++) - *(pIntr1_b++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X中间区域
		for( ; i < width-radius.width-1; i++) {
			thre = (*(pIntr2_b++) - *(pIntr2_f++) - *(pIntr1_b++) + *(pIntr1_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X右界区域
		for( ; i < width; i++) {
			thre = (*pIntr2_b - *(pIntr2_f++) - *pIntr1_b + *(pIntr1_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
			rctAreas -= increment;
		}
		pImL += width;
		pBinaL += width;
		pIntrL1 += width;
		pIntrL2 += width;
	}

	// Y下界区域
	for( ; j < height; j++) {
		pIm = pImL;
		pBina = pBinaL;
		pIntr1_f = pIntrL1;
		pIntr1_b = pIntrL1 + radius.width;
		pIntr2_f = pIntrL2;
		pIntr2_b = pIntrL2 + radius.width;
		rctAreas = radius.width * increment;
		// X左界区域
		for(i = 0; i <= radius.width; i++) {
			rctAreas += increment;
			thre = (*(pIntr2_b++) - *(pIntr1_b++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X中间区域
		for( ; i < width-radius.width-1; i++) {
			thre = (*(pIntr2_b++) - *(pIntr2_f++) - *(pIntr1_b++) + *(pIntr1_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
		}
		// X右界区域
		for( ; i < width; i++) {
			thre = (*pIntr2_b - *(pIntr2_f++) - *pIntr1_b + *(pIntr1_f++)) / rctAreas;
			thre = (falpha * thre) >> 10;
			*(pBina++) = (*(pIm++) < thre) ? 0 : 255;
			rctAreas -= increment;
		}
		pImL += width;
		pBinaL += width;
		pIntrL1 += width;
		increment--;
	}

	return 1;
}


int ImVerticalIntegrogram(unsigned char * in_data, int width, int height, int * integrogram)
{
	int ret_val = 0;

	int i = 0, j = 0;

	unsigned char * pIn = 0, * pInL = in_data;
	int * pOut = 0, * pOutL = integrogram;
	int * pOutUppr = 0;

	if(NULL == in_data || NULL == integrogram) {
		ret_val = -1;
		goto nExit;
	}

	if(0 >= width || 0 >= height) {
		return -1;
	}

	// 首行
	pIn = pInL;
	pOut = pOutL;
	for(i = 0; i < width; i++) {
		*pOut = *pIn;
		pIn++;
		pOut++;
	}
	pInL += width;
	pOutL += width;

	for(j = 1; j < height; j++) {
		pIn = pInL;
		pOut = pOutL;
		pOutUppr = pOutL - width;
		for(i = 0; i < width; i++) {
			*pOut = *pIn + *pOutUppr;
			pIn++;
			pOut++;
			pOutUppr++;
		}
		pInL += width;
		pOutL += width;
	}

	ret_val = 1;

nExit:

	return ret_val;
}