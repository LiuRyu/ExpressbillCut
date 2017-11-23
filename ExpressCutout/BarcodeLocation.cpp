#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

#include "RyuCore.h"
#include "BarcodeLocation.h"

#ifdef	_DEBUG_
#define _DEBUG_FASTLOCATE
#ifdef  _DEBUG_LOCATE
#include "OpenCv_debugTools.h"
#endif
#endif


#define FNC_AVG_VALUE(v0, c0, v1, c1)	(((v0) * (c0) + (v1) * (c1)) / ((c0) + (c1)))

//////////////////////////////////////////////////////////////////////////
// [v2.6.1] 图像中存在强反光时，条码提取不完整
// [v2.6.1] 注释为
//#define AUTOCONTRAST_THRESHOLD_RATIO_L	(0.05)
// [v2.6.1] 更改为
#define AUTOCONTRAST_THRESHOLD_RATIO_L	(0.1)
// [v2.6.1] 新增为
#define LOCATE_OUTPUT_MAXWIDTH			(1200)
#define LOCATE_OUTPUT_MAXHEIGHT			(600)
#define LOCATE_OUTPUT_MAXAREAS			(360000)
//////////////////////////////////////////////////////////////////////////

#define GRADHIST_THRE					(36)

//++++++++++++++++++++++++++++++++++重要改进+++++++++++++++++++++++++++++++++++++\\
// 可以将算法申请的较大内存空间统一为一个内存池，无关联的文件间可以复用同一片内存，节约算法的内存开支 

int gpnLctGradHist[256];
float gfLctGradHistRatio = 0;
int gnLctGradHistThre = 0;

static short gpsLctGradCnt[64] = {0};
static short gpsLctGradAcc[64] = {0};

static int gnLctImgMaxWidth = 0, gnLctImgMaxHeight = 0;
static int gnLctOGWid = 0, gnLctOGHei = 0;
static int gnLctBlobsWid = 0, gnLctBlobsHei = 0, gnLctBlobOGSize = 0;

static unsigned char * gucLctAtanLUT512 = 0;

unsigned char *	gucLctGradient = 0, * gucLctOrientation = 0;

int * gpnLctPrimaryMarks = 0;
/*
short *	gpsLctOGLabelMat = 0;

FastLocateClus * gflsLctPrimaryClus = 0;
int gnLctPrimaryClusCnt = 0;
*/
unsigned char * gucLctBlobMask = 0;		// 用来标识图像中的非零点(边缘)
RyuPoint * gpptLctBlobSeq = 0;				// 记录所有非零点的坐标
LocateClusLine * glclLctBlobClusLine = 0;
LocateClusArea * glcaLctBlobClusArea = 0;

int gnLctMaxClusCnt = 0;

int gnLctInitFlag = 0;


#ifdef _DEBUG_LOCATE
CvFont font, font1;
char txt[50];
IplImage * labl_img = 0;
IplImage * show_img = 0;
IplImage * show_img2 = 0;
IplImage * show_img3 = 0;
IplImage * show_img4 = 0;
IplImage * hist_acc = 0;
IplImage * iplShendu1 = 0;
IplImage * iplShendu2 = 0;
IplImage * iplLabelPxl = 0;
int mskSize_dbg = 0;
#endif

int FNC_LBC00_GetSampleGradOrient(unsigned char * const srcimg, int width, int height, 
								  int min_thre, int max_thre, float ratio, int * grad_thre);

int FNC_LBC01_GetBlobFeature(int grad_thre);
/*
int FNC_LBC02_FilterBorderBlob(int orin_thre, int thre1, int thre2);

int FNC_LBC03_GetRegionFeature(int * label_mat, int clus_cnt);
*/
int FNC_LBC02_BlobCluster2Line();

int FNC_LBC03_LineCluster2Area(int line_cnt);


int  BarcodeLocation_init(int max_width, int max_height)
{
	int error_ret = -1020100;
	int nRet = 0, status = 0;
	int imgMaxSize = 0;
	unsigned char * pLUT = 0;
	int i = 0, j = 0;
	int dx = 0, dy = 0;

	if(gnLctInitFlag) {
#ifdef	_PRINT_PROMPT
		printf("Error! BarcodeLocation is already initialized, DO NOT init again. --BarcodeLocation_init\n");
#endif
		nRet = error_ret - 1;
		goto nExit;
	}

	if(max_width <= 0 || max_height <= 0) {
#ifdef	_PRINT_PROMPT
		printf("Error! Invalid input of max_width/max_height, max_width=%d, max_height=%d. --BarcodeLocation_init\n",
				max_width, max_height);
#endif
		nRet = error_ret - 2;
		goto nExit;
	}

	gnLctImgMaxWidth = max_width;
	gnLctImgMaxHeight = max_height;
	imgMaxSize = gnLctImgMaxWidth * gnLctImgMaxHeight;

	gucLctAtanLUT512 = (unsigned char*) malloc(sizeof(unsigned char) * 512 * 512);
	if(!gucLctAtanLUT512) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gucLctAtanLUT512=0x%x. --BarcodeLocation_init\n", gucLctAtanLUT512);
#endif
		nRet = error_ret - 3;
		goto nExit;
	}
	memset(gucLctAtanLUT512, 0, 512 * 512 * sizeof(unsigned char));

	dy = -256;
	pLUT = gucLctAtanLUT512;
	for(i = 512; i > 0; i--) {
		dx = -256;
		for(j = 512; j > 0; j--) {
			*pLUT = ryuArctan180Shift(dy, dx);
			dx++;
			pLUT++;
		}
		dy++;
	}

	gucLctGradient = (unsigned char *)malloc(sizeof(unsigned char) * (imgMaxSize>>4));
	if(!gucLctGradient) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gucLctGradient=0x%x. --BarcodeLocation_init\n", gucLctGradient);
#endif
		nRet = error_ret - 4;
		goto nExit;
	}
	memset(gucLctGradient, 0, sizeof(unsigned char) * (imgMaxSize>>4));

	gucLctOrientation = (unsigned char *)malloc(sizeof(unsigned char) * (imgMaxSize>>4));
	if(!gucLctOrientation) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gucLctOrientation=0x%x. --BarcodeLocation_init\n", gucLctOrientation);
#endif
		nRet = error_ret - 5;
		goto nExit;
	}
	memset(gucLctOrientation, 0, sizeof(unsigned char) * (imgMaxSize>>4));

	gpnLctPrimaryMarks = (int *)malloc(sizeof(int) * (imgMaxSize>>4));
	if(!gpnLctPrimaryMarks) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gpnLctPrimaryMarks=0x%x. --BarcodeLocation_init\n", gpnLctPrimaryMarks);
#endif
		nRet = error_ret - 6;
		goto nExit;
	}
	memset(gpnLctPrimaryMarks, 0, sizeof(int) * (imgMaxSize>>4));

	/*
	gpsLctOGLabelMat = (short *)malloc(sizeof(short) * (imgMaxSize>>4));
	if(!gpsLctOGLabelMat) {
		nRet = -10000002;
		goto nExit;
	}
	memset(gpsLctOGLabelMat, 0, sizeof(short) * (imgMaxSize>>4));
	

	gflsLctPrimaryClus = (FastLocateClus *)malloc(sizeof(FastLocateClus) * (imgMaxSize>>8));
	if(!gflsLctPrimaryClus) {
		nRet = -10000003;
		goto nExit;
	}
	memset(gflsLctPrimaryClus, 0, sizeof(FastLocateClus) * (imgMaxSize>>8));

	status = ClassicCCA_Init(imgWid, imgHei, (imgMaxSize>>4));
	if((imgMaxSize>>4) != status) {
		nRet = -10000006;
		goto nExit;
	}
	*/

	gucLctBlobMask = (unsigned char *)malloc((imgMaxSize>>8) * sizeof(unsigned char));
	if(!gucLctBlobMask) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gucLctBlobMask=0x%x. --BarcodeLocation_init\n", gucLctBlobMask);
#endif
		nRet = error_ret - 7;
	}
	memset(gucLctBlobMask, 0, (imgMaxSize>>8) * sizeof(unsigned char));

	gpptLctBlobSeq = (RyuPoint *)malloc((imgMaxSize>>8) * sizeof(RyuPoint));
	if(!gpptLctBlobSeq) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, gpptLctBlobSeq=0x%x. --BarcodeLocation_init\n", gpptLctBlobSeq);
#endif
		nRet = error_ret - 8;
	}
	memset(gpptLctBlobSeq, 0, (imgMaxSize>>8) * sizeof(RyuPoint));

	gnLctMaxClusCnt = imgMaxSize>>12;
	glclLctBlobClusLine = (LocateClusLine *)malloc(gnLctMaxClusCnt * sizeof(LocateClusLine));
	if(!glclLctBlobClusLine) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, glclLctBlobClusLine=0x%x. --BarcodeLocation_init\n", glclLctBlobClusLine);
#endif
		nRet = error_ret - 9;
	}
	memset(glclLctBlobClusLine, 0, gnLctMaxClusCnt * sizeof(LocateClusLine));

	glcaLctBlobClusArea = (LocateClusArea *)malloc(gnLctMaxClusCnt * sizeof(LocateClusArea));
	if(!glcaLctBlobClusArea) {
#ifdef	_PRINT_PROMPT
		printf("Error! Memory allocation failed, glcaLctBlobClusArea=0x%x. --BarcodeLocation_init\n", glcaLctBlobClusArea);
#endif
		nRet = error_ret - 10;
	}
	memset(glcaLctBlobClusArea, 0, gnLctMaxClusCnt * sizeof(LocateClusArea));


	gnLctInitFlag = 1;
	nRet = 1;

nExit:
	return nRet;
}


void BarcodeLocation_release()
{
	gnLctInitFlag = 0;

	if(gucLctAtanLUT512) {
		free(gucLctAtanLUT512);
		gucLctAtanLUT512 = 0;
	}

	if(gucLctGradient) {
		free(gucLctGradient);
		gucLctGradient = 0;
	}

	if(gucLctOrientation) {
		free(gucLctOrientation);
		gucLctOrientation = 0;
	}

	if(gpnLctPrimaryMarks) {
		free(gpnLctPrimaryMarks);
		gpnLctPrimaryMarks = 0;
	}

	/*
	if(gpsLctOGLabelMat) {
		free(gpsLctOGLabelMat);
		gpsLctOGLabelMat = 0;
	}

	if(gflsLctPrimaryClus) {
		free(gflsLctPrimaryClus);
		gflsLctPrimaryClus = 0;
	}

	ClassicCCA_Release();
	*/

	if(gucLctBlobMask) {
		free(gucLctBlobMask);
		gucLctBlobMask = 0;
	}

	if(gpptLctBlobSeq) {
		free(gpptLctBlobSeq);
		gpptLctBlobSeq = 0;
	}

	if(glclLctBlobClusLine) {
		free(glclLctBlobClusLine);
		glclLctBlobClusLine = 0;
	}

	if(glcaLctBlobClusArea) {
		free(glcaLctBlobClusArea);
		glcaLctBlobClusArea = 0;
	}

	return;
}

int LocateBarcodeAreas(unsigned char * img, int wid, int hei, int bloc_size)
{
	int error_ret = -1020200;
	int nRet = 0, status = 0;
	int nGradThre = 0;
	int clus_cnt = 0, * label_mat = 0;

	int line_cnt = 0;

	if(1 != gnLctInitFlag) {
#ifdef	_PRINT_PROMPT
		printf("Error! LocateBarcodeAreas runs without initialization, DO init before use. --LocateBarcodeAreas\n");
#endif
		nRet = error_ret - 1;
		goto nExit;
	}

	gnLctBlobOGSize = (bloc_size >> 2) + ((bloc_size % 4) >> 1);
	bloc_size = gnLctBlobOGSize << 2;

	gnLctOGWid = wid >> 2;
	gnLctOGHei = hei >> 2;

	if(gnLctOGWid <= 0 || gnLctOGHei <= 0) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected value of gnLctOGWid/gnLctOGHei, gnLctOGWid=%d, gnLctOGHei=%d. --LocateBarcodeAreas\n",
				gnLctOGWid, gnLctOGHei);
#endif
		nRet = error_ret - 2;
		goto nExit;
	}

	if((gnLctBlobOGSize >> 1) <= 0) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected value of gnLctBlobOGSize, gnLctBlobOGSize=%d, (gnLctBlobOGSize>>1)=%d. --LocateBarcodeAreas\n",
			gnLctBlobOGSize, (gnLctBlobOGSize>>1));
#endif
		nRet = error_ret - 3;
		goto nExit;
	}

	gnLctBlobsWid = gnLctOGWid / (gnLctBlobOGSize >> 1) - 1;
	gnLctBlobsHei = gnLctOGHei / (gnLctBlobOGSize >> 1) - 1;

#ifdef _DEBUG_LOCATE
	cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.5, 0.5, 0.0, 1, CV_AA);
	cvInitFont(&font1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0.0, 2, CV_AA);
	labl_img = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 1);
	show_img = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 3);
	show_img2 = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 3);
	show_img3 = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 3);
	show_img4 = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 3);
	hist_acc = cvCreateImage(cvSize(360, 220), 8, 3);
#endif

	// 4*4mask对图像进行扫描,记录每个扫描区域的梯度幅值和方向
	status = FNC_LBC00_GetSampleGradOrient(img, wid, hei, 0, 36, AUTOCONTRAST_THRESHOLD_RATIO_L, &nGradThre);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of GetSampleGradOrient, ret_val=%d. --LocateBarcodeAreas\n", status);
#endif
		nRet = status;
		goto nExit;
	}
#ifdef _DEBUG_LOCATE
	unsigned char * pLabl_dbg = 0;
	unsigned char * pGrad_dbg = gucLctGradient;
	unsigned char * pOrie_dbg = gucLctOrientation;

	for(int i_dbg = 0; i_dbg < gnLctOGHei; i_dbg++) {
		pLabl_dbg = (unsigned char *)labl_img->imageData + i_dbg * labl_img->widthStep;
		for(int j_dbg = 0; j_dbg < gnLctOGWid; j_dbg++) {
			if(*pGrad_dbg > nGradThre) {
				pLabl_dbg[j_dbg] = 255;
			} else {
				pLabl_dbg[j_dbg] = 0;
			}
// 			pLabl_dbg[j_dbg] = *pGrad_dbg;
			pGrad_dbg++;
			pOrie_dbg++;
		}
	}

//	cvCvtColor(labl_img, show_img2, CV_GRAY2BGR);
	cvCopy(show_img, show_img2);
// 	cvNamedWindow("梯度点标记");
// 	cvShowImage("梯度点标记", show_img2);
// 	cvWaitKey();
#endif

	// 聚类元检测,可优化提速的空间最大
	status = FNC_LBC01_GetBlobFeature(nGradThre);
	if(0 == status) {
		nRet = 0;
		goto nExit;
	} else if(0 > status) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of GetBlobFeatur, ret_val=%d. --LocateBarcodeAreas\n", status);
#endif
		nRet = status;
		goto nExit;
	}

#if (1)

	line_cnt = FNC_LBC02_BlobCluster2Line();

	if(0 == line_cnt) {
		nRet = 0;
		goto nExit;
	} else if(0 > line_cnt) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of BlobCluster2Line, ret_val=%d. --LocateBarcodeAreas\n", status);
#endif
		nRet = status;
		goto nExit;
	}

	clus_cnt = FNC_LBC03_LineCluster2Area(line_cnt);
	if(0 == clus_cnt) {
		nRet = 0;
		goto nExit;
	} else if(0 > clus_cnt) {
#ifdef	_PRINT_PROMPT
		printf("Error! Unexpected return of LineCluster2Area, ret_val=%d. --LocateBarcodeAreas\n", status);
#endif
		nRet = status;
		goto nExit;
	}

	nRet = clus_cnt;

#else
	// 1为比重,2为个数
	FNC_LBC02_FilterBorderBlob(20, 20, 20);
	
	// 聚类
	clus_cnt = ClassicCCA_LabelImage(0, gpnLctPrimaryMarks, gnLctBlobsWid, gnLctBlobsHei, gnLctBlobsWid, 15, 18);
	label_mat = ClassicCCA_GetLabelMat();

	if(0 > clus_cnt) {
		nRet = -10000008;
		goto nExit;
	} else if(0 == clus_cnt) {
		nRet = 0;
		goto nExit;
	}

	// 聚类写入,融合
	status = FNC_LBC03_GetRegionFeature(label_mat, clus_cnt);
	
	nRet = status;

#endif

nExit:
#ifdef _DEBUG_LOCATE
	if(!labl_img)	cvReleaseImage(&labl_img);
	if(!show_img)	cvReleaseImage(&show_img);
	if(!show_img2)	cvReleaseImage(&show_img2);
	if(!show_img3)	cvReleaseImage(&show_img3);
	if(!show_img4)	cvReleaseImage(&show_img4);
	if(!hist_acc)	cvReleaseImage(&hist_acc);
	if(!iplShendu1)	cvReleaseImage(&iplShendu1);
	if(!iplShendu2)	cvReleaseImage(&iplShendu2);
	if(!iplLabelPxl)	cvReleaseImage(&iplLabelPxl);
#endif

	return nRet;
}

/*
FastLocateClus * getLocateBarCodePrimary()
{
	return gflsLctPrimaryClus;
}
*/

LocateClusArea * getLocateFeatureAreas()
{
	return glcaLctBlobClusArea;
}


int FNC_LBC00_GetSampleGradOrient(unsigned char * const srcimg, int width, int height, 
								  int min_thre, int max_thre, float ratio, int * grad_thre)
{
	int nRet = 0;
	int i = 0, j = 0, nstep = 0;

	int dx[4] = {0}, dy[4] = {0}, t[4] = {0};

	int val = 0, val1 = 0, val2 = 0, idx = 0, idx1 = 0, idx2 = 0;

	int nsum = 0, nthre = 0;

	int * hist = gpnLctGradHist;

	unsigned char * pOrient	= 0;
	unsigned char * pGrad	= 0;

	unsigned char * lOrient	= 0;
	unsigned char * lGrad	= 0;

	unsigned char * loffset_1, * loffset_2, * loffset_3, * loffset_4;
	unsigned char * poffset_1, * poffset_2, * poffset_3, * poffset_4;

	if(1 != gnLctInitFlag) {
		nRet = -10000011;
		goto nExit;
	}

	if(!srcimg || !grad_thre) {
		nRet = -10000012;
		goto nExit;
	}

	if(gfLctGradHistRatio != ratio) {
		gfLctGradHistRatio = ratio;
		gnLctGradHistThre = (int)(gnLctOGWid * gnLctOGHei * gfLctGradHistRatio);
	}

	memset(hist, 0, sizeof(int) * 256);

	loffset_1 = loffset_2 = loffset_3 = loffset_4 = 0;
	poffset_1 = poffset_2 = poffset_3 = poffset_4 = 0;

	nstep = width << 2;
	lOrient	= gucLctOrientation;
	lGrad	= gucLctGradient;

	loffset_1	= srcimg;
	loffset_2	= loffset_1 + width;
	loffset_3	= loffset_2 + width;
	loffset_4	= loffset_3 + width;

	for(i = gnLctOGHei; i > 0; i--) {
		poffset_1 = loffset_1;
		poffset_2 = loffset_2;
		poffset_3 = loffset_3;
		poffset_4 = loffset_4;

		pGrad = lGrad;
		pOrient = lOrient;

		for(j = gnLctOGWid; j > 0; j--) {
			dx[0] = poffset_2[2] - poffset_2[0];
			dy[0] = poffset_3[1] - poffset_1[1];
			dx[1] = poffset_2[3] - poffset_2[1];
			dy[1] = poffset_3[2] - poffset_1[2];
			dx[2] = poffset_3[2] - poffset_3[0];
			dy[2] = poffset_4[1] - poffset_2[1];
			dx[3] = poffset_3[3] - poffset_3[1];
			dy[3] = poffset_4[2] - poffset_2[2];

			nsum = (poffset_1[0] + poffset_1[1] + poffset_1[2] + poffset_1[3]
			+  poffset_2[0] + poffset_2[1] + poffset_2[2] + poffset_2[3]
			+  poffset_3[0] + poffset_3[1] + poffset_3[2] + poffset_3[3]
			+  poffset_4[0] + poffset_4[1] + poffset_4[2] + poffset_4[3]) >> 6;

			// nthre = (grad_thre > nsum) ? grad_thre : nsum;

			t[0] = abs(dx[0]) > abs(dy[0]) ? abs(dx[0]) : abs(dy[0]);
			t[1] = abs(dx[1]) > abs(dy[1]) ? abs(dx[1]) : abs(dy[1]);
			t[2] = abs(dx[2]) > abs(dy[2]) ? abs(dx[2]) : abs(dy[2]);
			t[3] = abs(dx[3]) > abs(dy[3]) ? abs(dx[3]) : abs(dy[3]);

			val1 = (t[0] > t[1]) ? t[0] : t[1];
			idx1 = (t[0] > t[1]) ? 0 : 1;
			val2 = (t[2] > t[3]) ? t[2] : t[3];
			idx2 = (t[2] > t[3]) ? 2 : 3;

			idx = (val1 > val2) ? idx1 : idx2;
			val = (val1 > val2) ? val1 : val2;

			*pGrad = (val < min_thre) ? 0 : val;
			*pOrient = (*pGrad) ? gucLctAtanLUT512[(dx[idx]+256)|((dy[idx]+256)<<9)] : 0xff;

			hist[val]++;

			poffset_1 += 4;
			poffset_2 += 4;
			poffset_3 += 4;
			poffset_4 += 4;
			pGrad++;
			pOrient++;
		}
		loffset_1 += nstep;
		loffset_2 += nstep;
		loffset_3 += nstep;
		loffset_4 += nstep;
		lOrient += gnLctOGWid;
		lGrad += gnLctOGWid;
	}

	nsum = nthre = j = 0;
	hist = gpnLctGradHist + 255;
	for(i = 256; i > 0; i--) {
		nsum += *hist;

		if(nsum >= gnLctGradHistThre) {
			j = i - 1;
			break;
		}
		hist--;
	}

	j = (j > min_thre) ? j : min_thre;
	*grad_thre = (j < max_thre) ? j : max_thre;

#ifdef _DEBUG_LOCATE
	IplImage * iplImg8u1c_1 = cvCreateImage(cvSize(gnLctOGWid, gnLctOGHei), 8, 1);
	unsigned char * pGrad_dbg = gucLctGradient;
	for(int j_dbg = 0; j_dbg < gnLctOGHei; j_dbg++) {
		unsigned char * pHist_dbg = (unsigned char *)iplImg8u1c_1->imageData + j_dbg * iplImg8u1c_1->widthStep;
		for(int i_dbg = 0; i_dbg < gnLctOGWid; i_dbg++) {
			*pHist_dbg = *pGrad_dbg;
			pHist_dbg++;
			pGrad_dbg++;
		}
	}
	cvCvtColor(iplImg8u1c_1, show_img, CV_GRAY2BGR);
	//cvNamedWindow("采样梯度");
	//cvShowImage("采样梯度", show_img);
	cvReleaseImage(&iplImg8u1c_1);
#endif

	nRet = 1;
nExit:
	return nRet;
}

int FNC_LBC01_GetBlobFeature(int grad_thre)
{
	int nRet = 0;
	int i = 0, j = 0, m = 0, n = 0;

	int offsetH = 0, offsetW = 0;

	unsigned char * pGrad_H = gucLctGradient, * pOrie_H = gucLctOrientation;
	unsigned char * pGrad, *pOrie, * pGrad_B, * pOrie_B, * pGrad_L, * pOrie_L;

	int nGradCnt = 0, nGradAcc = 0, nGradAvg = 0;

	int * pBlockStamps = gpnLctPrimaryMarks;
	int effectCnt = 0;

	short * pCnt1 = 0, * pCnt2 = 0, * pAcc1 = 0, * pAcc2 = 0;

	short gradVal = 0, gradSgn = 0;

	int index = 0, index_t = 0;
	int val_acc = 0, cnt_acc = 0, cnt_macc = 0, val_macc = 0;
	int val_rat = 0;
	int val = 0;

	const int Set0Cnt = sizeof(short) * 45;
	const int aPI = 45;
	const int acc_radius = 2;
	int acc_range = (acc_radius<<1) + 1;

	offsetH = gnLctOGWid * (gnLctBlobOGSize >> 1);
	offsetW = gnLctBlobOGSize >> 1;

	for(m = gnLctBlobsHei; m > 0; m--) {
		pGrad_B = pGrad_H;
		pOrie_B = pOrie_H;
		for(n = gnLctBlobsWid; n > 0; n--) {
			pGrad_L = pGrad_B;
			pOrie_L = pOrie_B;
			nGradCnt = nGradAcc = nGradAvg = 0;
			memset(gpsLctGradCnt, 0, Set0Cnt);
			memset(gpsLctGradAcc, 0, Set0Cnt);
			for(i = gnLctBlobOGSize; i > 0; i--) {
				pGrad = pGrad_L;
				pOrie = pOrie_L;
				for(j = gnLctBlobOGSize; j > 0; j--) {
					//////////////////////////////////////////////////////////////////////////
					// ***耗时长主要是因为这两句,约占用13ms!!!
					// 是否可以通过cache优化来提速
					//////////////////////////////////////////////////////////////////////////
					index = *pOrie >> 2;
					gradVal = (*pGrad < grad_thre) ? 0 : *pGrad;
					gradSgn = (gradVal && 1);
					*(gpsLctGradCnt+index) += gradSgn;	// ***
					*(gpsLctGradAcc+index) += gradVal;	// ***
					//////////////////////////////////////////////////////////////////////////
					nGradCnt += gradSgn;
					nGradAcc += gradVal;
					pGrad++;
					pOrie++;
				}
				pGrad_L += gnLctOGWid;
				pOrie_L += gnLctOGWid;
			}
			pGrad_B += offsetW;
			pOrie_B += offsetW;

			if(nGradCnt < gnLctBlobOGSize || nGradAcc <= 0) {
				*pBlockStamps = 0;
				pBlockStamps++;
				continue;
			}

			// nGradAcc++;		// 防止除数为0

			// 后面这一部分耗时约10ms,可以通过降低角度分辨率提速
			val_acc = cnt_acc = 0;
			pAcc1 = gpsLctGradAcc;
			pAcc2 = gpsLctGradAcc + aPI;
			pCnt1 = gpsLctGradCnt;
			pCnt2 = gpsLctGradCnt + aPI;

			index = index_t = acc_radius;
			for(i = acc_range; i > 0; i--) {
				*(pAcc2++) = *pAcc1;
				*(pCnt2++) = *pCnt1;
				val_acc += *(pAcc1++);
				cnt_acc += *(pCnt1++);
			}
			// 中间值加成
			val_macc = val = val_acc + gpsLctGradAcc[index_t++];
			cnt_macc = cnt_acc;

			pAcc1 = gpsLctGradAcc;
			pCnt1 = gpsLctGradCnt;
			pAcc2 = gpsLctGradAcc + acc_range;
			pCnt2 = gpsLctGradCnt + acc_range;

			for(i = aPI; i > 0; i--) {
				val_acc += (*(pAcc2++) - *(pAcc1++));
				cnt_acc += (*(pCnt2++) - *(pCnt1++));
				val = val_acc + gpsLctGradAcc[index_t];
				cnt_macc = (val > val_macc) ? cnt_acc : cnt_macc;
				index = (val > val_macc) ? index_t : index;
				val_macc = (val > val_macc) ? val : val_macc;
				index_t++;
			}

			
			index = (index >= aPI) ? (index - aPI) : index;
			val_macc -= gpsLctGradAcc[index];

			index = (index << 2) + 2;

			cnt_acc = cnt_macc;
			val_acc = val_macc;

// 			// 考虑条码边界,对于垂直方向进行加和
// 			if(index - 9 < 90) {
// 				index_t = index + 90;
// 			} else {
// 				index_t = index - 90;
// 			}
// 			for(i = -9; i < 10; i++) {
// 				cnt_acc += sGradCnt[index_t+i];
// 				val_acc += sGradAcc[index_t+i];
// 			}

//			cnt_rat = cnt_acc * 100 / nGradCnt;
			val_rat = val_acc * 100 / nGradAcc;
			nGradAvg = nGradAcc / nGradCnt;
			nGradAvg = (nGradAvg < 0) ? 0 : nGradAvg;
			nGradAvg = (nGradAvg > 0xff) ? 0xff : nGradAvg;

			if(cnt_acc >= gnLctBlobOGSize && val_rat > GRADHIST_THRE) {
				
#if (1)
				*pBlockStamps = (0xf<<24) | (cnt_acc<<16) | (nGradAvg<<8) | index;
#else
				*pBlockStamps = (0xf<<24) | (cnt_acc<<16) | (val_rat<<8) | index;
#endif

				effectCnt++;
			} else if(nGradCnt >= gnLctBlobOGSize) {
				*pBlockStamps = (0x1<<24);
			} else {
				*pBlockStamps = 0;
			}

			pBlockStamps++;

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_FASTLOCATE_STEP
			if (nGradCnt > 0) {
				printf("-----nGradCnt=%3d, nGradAcc=%3d\n", nGradCnt, nGradAcc);
				printf("     cnt_acc= %3d, val_acc= %3d, orie=%3d\n", cnt_acc, val_acc, index);
				printf("     val_rat= %3d\n", val_rat);
				cvZero(hist_acc);
				for(i = 0; i < 180; i++) {
					if(abs(i-index)<acc_radius || abs(i-index)>180-acc_radius) {
						cvLine(hist_acc, cvPoint(i*2, 220-200*gpsLctGradAcc[i>>2]/(val_macc+1)), cvPoint(i*2, 220), CV_RGB(0,255,0));
						cvLine(hist_acc, cvPoint(i*2+1, 220-200*gpsLctGradAcc[i>>2]/(val_macc+1)), cvPoint(i*2+1, 220), CV_RGB(0,255,0));
					} else {
						cvLine(hist_acc, cvPoint(i*2, 220-200*gpsLctGradAcc[i>>2]/(val_macc+1)), cvPoint(i*2, 220), CV_RGB(255,255,255));
						cvLine(hist_acc, cvPoint(i*2+1, 220-200*gpsLctGradAcc[i>>2]/(val_macc+1)), cvPoint(i*2+1, 220), CV_RGB(255,255,255));
					}
				}
			}
#endif
			if(cnt_acc >= gnLctBlobOGSize && val_rat > GRADHIST_THRE) {
				cvRectangle(show_img2, cvPoint((gnLctBlobsWid-n)*offsetW, (gnLctBlobsHei-m)*offsetW), 
					cvPoint((gnLctBlobsWid-n+1)*offsetW-1, (gnLctBlobsHei-m+1)*offsetW-1), CV_RGB(255, 0, 0));
#ifdef _DEBUG_FASTLOCATE_STEP
				cvShowImage("DEBUG_FASTLOCATE", show_img2);
				cvNamedWindow("hist_acc");
				cvShowImage("hist_acc", hist_acc);
				cvWaitKey();
#endif
#ifdef _DEBUG_FASTLOCATE_STEP
			} else if(nGradCnt >= gnLctBlobOGSize) {
				cvRectangle(show_img2, cvPoint((gnLctBlobsWid-n)*offsetW, (gnLctBlobsHei-m)*offsetW), 
					cvPoint((gnLctBlobsWid-n+1)*offsetW-1, (gnLctBlobsHei-m+1)*offsetW-1), CV_RGB(216, 39, 203));
				cvShowImage("DEBUG_FASTLOCATE", show_img2);
				cvNamedWindow("hist_acc");
				cvShowImage("hist_acc", hist_acc);
				cvWaitKey();
#endif
			}
#ifdef _DEBUG_FASTLOCATE_STEP
			else if(nGradCnt > 0) {
				cvRectangle(show_img2, cvPoint((gnLctBlobsWid-n)*offsetW, (gnLctBlobsHei-m)*offsetW), 
					cvPoint((gnLctBlobsWid-n+1)*offsetW-1, (gnLctBlobsHei-m+1)*offsetW-1), CV_RGB(0, 64, 128));
				cvShowImage("DEBUG_FASTLOCATE", show_img2);
				cvNamedWindow("hist_acc");
				cvShowImage("hist_acc", hist_acc);
				cvWaitKey();
			}
#endif
#endif
#endif
		}
		pGrad_H += offsetH;
		pOrie_H += offsetH;
	}
#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
	
	printf("effectCnt = %d \n", effectCnt);
	cvShowImage("DEBUG_FASTLOCATE", show_img2);
	//cvWaitKey();
	
#endif
#endif

	nRet = effectCnt;

nExit:
	return nRet;
}


/*
int FNC_LBC03_GetRegionFeature(int * label_mat, int clus_cnt)
{
	int nRet = 0;

	int i = 0, j = 0, x = 0, y = 0;
	int * pLabel = 0, * pLabel_L = 0;
	int effcCnt = 0;

	int index = 0, angle = 0;
	int nTmp1 = 0, nTmp2 = 0;

	FastLocateClus * pClus = 0;
	FastLocateClus temClus = gflsLctPrimaryClus[0];

	int * pBlockStamps = gpnLctPrimaryMarks;
	int A = 0, B = 0, C = 0;
	int A_t = 0, B_t = 0, C_t = 0;

	int min_intr1, min_intr2, max_intr1, max_intr2;
	int min_intr_t1, min_intr_t2, max_intr_t1, max_intr_t2;

	int nCX1 = 0, nCX2 = 0, nCY1 = 0, nCY2 = 0;
	short * pOGLb = 0;

	float sqrtVal = 0.0;

	min_intr1 = min_intr2 = max_intr1 = max_intr2 = 0;
	min_intr_t1 = min_intr_t2 = max_intr_t1 = max_intr_t2 = 0;

	pLabel = label_mat;
	memset(gflsLctPrimaryClus, 0, sizeof(FastLocateClus) * (clus_cnt+1));

	// 写入聚类结构体数组
	for(j = 0; j < gnLctBlobsHei; j++) {
		for(i = 0; i < gnLctBlobsWid; i++) {
			if(*pLabel <= 0) {
				*pLabel = 0;
				pBlockStamps++;
				pLabel++;
				continue;
			}
			x = (i + 1) << 4;
			y = (j + 1) << 4;

			index = *pLabel;
			angle = *pBlockStamps & 0xff;
			pClus = &(gflsLctPrimaryClus[index]);
			pClus->clus_label = index;
			pClus->tgt_num++;
			pClus->center_x += x;
			pClus->center_y += y;

			if(1 == pClus->tgt_num) {
				pClus->LTRB[0] = i;
				pClus->LTRB[1] = j;
				pClus->LTRB[2] = i;
				pClus->LTRB[3] = j;
			} else {
				pClus->LTRB[0] = (pClus->LTRB[0] < i) ? pClus->LTRB[0] : i;
				pClus->LTRB[1] = (pClus->LTRB[1] < j) ? pClus->LTRB[1] : j;
				pClus->LTRB[2] = (pClus->LTRB[2] > i) ? pClus->LTRB[2] : i;
				pClus->LTRB[3] = (pClus->LTRB[3] > j) ? pClus->LTRB[3] : j;
			}

			pClus->intrcpt += ((angle < 90) ? angle : 0);
			pClus->intrcpt_t += ((angle < 90) ? 1 : 0);

			pClus->cen_intr += ((angle < 90) ? 0 : angle);
			pClus->cen_intr_t += ((angle < 90) ? 0 : 1);

			pBlockStamps++;
			pLabel++;
		}
	}

	// 计算角度、中心点参数
	pClus = gflsLctPrimaryClus + 1;
	for(i = 1; i <= clus_cnt; i++) {
		if(2 >= pClus->tgt_num) {
			memset(pClus, 0, sizeof(FastLocateClus));
			pClus++;
			continue;
		}
		if(pClus->intrcpt_t + pClus->cen_intr_t != pClus->tgt_num) {
			memset(pClus, 0, sizeof(FastLocateClus));
			pClus++;
			continue;
		}

		if(pClus->intrcpt_t > 0 && pClus->cen_intr_t > 0) {
			nTmp1 = pClus->intrcpt / pClus->intrcpt_t;
			nTmp2 = pClus->cen_intr / pClus->cen_intr_t;
			if(nTmp1 - nTmp2 > 90) {
				angle = (nTmp1 * pClus->intrcpt_t + (nTmp2+180) * pClus->cen_intr_t) / pClus->tgt_num;
				angle = (angle >= 180) ? (angle - 180) : angle;
			} else if(nTmp2 - nTmp1 > 90) {
				angle = ((nTmp1+180) * pClus->intrcpt_t + nTmp2 * pClus->cen_intr_t) / pClus->tgt_num;
				angle = (angle >= 180) ? (angle - 180) : angle;
			} else {
				angle = (nTmp1 * pClus->intrcpt_t + nTmp2 * pClus->cen_intr_t) / pClus->tgt_num;
			}
		} else {
			angle = (pClus->intrcpt + pClus->cen_intr) / pClus->tgt_num;
		}

		pClus->angle = angle;
		pClus->center_x /= pClus->tgt_num;
		pClus->center_y /= pClus->tgt_num;

		A = ryuCosShift(pClus->angle+90);
		B = ryuSinShift(pClus->angle+90);
		C = A * pClus->center_x + B * pClus->center_y;
		pClus->cen_intr = C >> 10;

		A = ryuCosShift(pClus->angle+180);
		B = ryuSinShift(pClus->angle+180);
		C_t = A * pClus->center_x + B * pClus->center_y;
		pClus->cen_intr_t = C_t >> 10;

		pClus->intrcpt = pClus->intrcpt_t = 0;

		effcCnt++;
		pClus++;
	}

	// 计算外围矩形参数
	pLabel = label_mat;
	pBlockStamps = gpnLctPrimaryMarks;
	for(j = 0; j < gnLctBlobsHei; j++) {
		for(i = 0; i < gnLctBlobsWid; i++) {
			if(0 >= *pLabel) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			pClus = &(gflsLctPrimaryClus[*pLabel]);
			if(0 >= pClus->tgt_num) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			x = (i + 1) << 4;
			y = (j + 1) << 4;

			A = ryuCosShift(pClus->angle+90);
			B = ryuSinShift(pClus->angle+90);
			C = (A * x + B * y) >> 10;
			C = abs(C - pClus->cen_intr);

			A = ryuCosShift(pClus->angle+180);
			B = ryuSinShift(pClus->angle+180);
			C_t = (A * x + B * y) >> 10;
			C_t = abs(C_t - pClus->cen_intr_t);

			pClus->intrcpt = (pClus->intrcpt > C) ? pClus->intrcpt : C;
			pClus->intrcpt_t = (pClus->intrcpt_t > C_t) ? pClus->intrcpt_t : C_t;

			pLabel++;
			pBlockStamps++;
		}
	}

// 	// 角度匹配、位置相近且形状拟合者，进行合并操作
// 	for(i = 1; i < clus_cnt; i++) {
// 		if(0 >= glcPrimaryClus[i].tgt_num)
// 			continue;
// 		for(j = i + 1; j < clus_cnt; j++) {
// 			if(0 >= glcPrimaryClus[j].tgt_num)
// 				continue;
// 			// 角度匹配
// 			angle = abs(glcPrimaryClus[i].angle - glcPrimaryClus[j].angle);
// 			if(angle < 172 && angle > 8) 
// 				continue;
// 			// 计算融合角度
// 			index = glcPrimaryClus[i].angle*glcPrimaryClus[i].tgt_num + glcPrimaryClus[j].angle*glcPrimaryClus[j].tgt_num;
// 			nTmp1 = (glcPrimaryClus[i].angle < glcPrimaryClus[j].angle) ? glcPrimaryClus[i].tgt_num : glcPrimaryClus[j].tgt_num;
// 			nTmp2 = (angle > 90) ? 1 : 0;
// 			index += 180 * nTmp1 * nTmp2;
// 			index /= (glcPrimaryClus[i].tgt_num + glcPrimaryClus[j].tgt_num);
// 			index = (index < 180) ? index : index - 180;
// 
// 			// 使用融合角度重新计算各自的中心点参数和截距范围
// 			A = ryuCosShift(index+90);
// 			B = ryuSinShift(index+90);
// 			C = A * glcPrimaryClus[i].center_x + B * glcPrimaryClus[i].center_y;
// 			C = C >> 10;
// 			nTmp1 = C;
// 			min_intr1 = C - glcPrimaryClus[i].intrcpt;
// 			max_intr1 = C + glcPrimaryClus[i].intrcpt;
// 			C = A * glcPrimaryClus[j].center_x + B * glcPrimaryClus[j].center_y;
// 			C = C >> 10;
// 			nTmp1 = abs(C - nTmp1);		// 中心点截距差(条码垂直方向)
// 			min_intr2 = C - glcPrimaryClus[j].intrcpt;
// 			max_intr2 = C + glcPrimaryClus[j].intrcpt;
// 
// 			A_t = ryuCosShift(index+180);
// 			B_t = ryuSinShift(index+180);
// 			C_t = A_t * glcPrimaryClus[i].center_x + B_t * glcPrimaryClus[i].center_y;
// 			C_t = C_t >> 10;
// 			nTmp2 = C_t;
// 			min_intr_t1 = C_t - glcPrimaryClus[i].intrcpt_t;
// 			max_intr_t1 = C_t + glcPrimaryClus[i].intrcpt_t;
// 			C_t = A_t * glcPrimaryClus[j].center_x + B_t * glcPrimaryClus[j].center_y;
// 			C_t = C_t >> 10;
// 			nTmp2 = abs(C_t - nTmp2);		// 中心点截距差(条码方向)
// 			min_intr_t2 = C_t - glcPrimaryClus[j].intrcpt_t;
// 			max_intr_t2 = C_t + glcPrimaryClus[j].intrcpt_t;
// 
// 			// 条码垂直方向中心点截距差-应小于-条码方向中心点截距差
// 			if(nTmp1 > nTmp2)
// 				continue;
// 
// 			// 垂直方向位置相近
// 			if(min_intr1 > max_intr2) {
// 				nTmp1 = min_intr1 - max_intr2;
// 			} else if(max_intr1 < min_intr2) {
// 				nTmp1 = min_intr2 - max_intr1;
// 			} else {
// 				nTmp1 = 0;
// 			}
// 			if(nTmp1 > 32)
// 				continue;
// 
// 			// 水平方向位置相近
// 			if(min_intr_t1 > max_intr_t2) {
// 				nTmp2 = min_intr_t1 - max_intr_t2;
// 			} else if(max_intr_t1 < min_intr_t2) {
// 				nTmp2 = min_intr_t2 - max_intr_t1;
// 			} else {
// 				nTmp2 = 0;
// 			}
// 			if(nTmp2 > 64)
// 				continue;
// 
// 			// 改变标记号
// 			pLabel_L = label_mat + glcPrimaryClus[i].LTRB[1] * blocs_wid + glcPrimaryClus[i].LTRB[0];
// 			for(y = glcPrimaryClus[i].LTRB[3] - glcPrimaryClus[i].LTRB[1] + 1; y > 0; y--) {
// 				pLabel = pLabel_L;
// 				for(x = glcPrimaryClus[i].LTRB[2] - glcPrimaryClus[i].LTRB[0] + 1; x > 0; x--) {
// 					*pLabel = (*pLabel == glcPrimaryClus[i].clus_label) ? glcPrimaryClus[j].clus_label : *pLabel;
// 					pLabel++;
// 				}
// 				pLabel_L += blocs_wid;
// 			}
// 
// 			min_intr1 = (min_intr1 < min_intr2) ? min_intr1 : min_intr2;
// 			max_intr1 = (max_intr1 > max_intr2) ? max_intr1 : max_intr2;
// 			min_intr_t1 = (min_intr_t1 < min_intr_t2) ? min_intr_t1 : min_intr_t2;
// 			max_intr_t1 = (max_intr_t1 > max_intr_t2) ? max_intr_t1 : max_intr_t2;
// 
// 			glcPrimaryClus[j].intrcpt = (max_intr1 - min_intr1) >> 1;
// 			glcPrimaryClus[j].intrcpt_t = (max_intr_t1 - min_intr_t1) >> 1;
// 			glcPrimaryClus[j].cen_intr = (max_intr1 + min_intr1) >> 1;
// 			glcPrimaryClus[j].cen_intr_t = (max_intr_t1 + min_intr_t1) >> 1;
// 
// 			glcPrimaryClus[j].LTRB[0] = (glcPrimaryClus[j].LTRB[0] < glcPrimaryClus[i].LTRB[0])
// 				? glcPrimaryClus[j].LTRB[0] : glcPrimaryClus[i].LTRB[0];
// 			glcPrimaryClus[j].LTRB[1] = (glcPrimaryClus[j].LTRB[1] < glcPrimaryClus[i].LTRB[1])
// 				? glcPrimaryClus[j].LTRB[1] : glcPrimaryClus[i].LTRB[1];
// 			glcPrimaryClus[j].LTRB[2] = (glcPrimaryClus[j].LTRB[2] > glcPrimaryClus[i].LTRB[2])
// 				? glcPrimaryClus[j].LTRB[2] : glcPrimaryClus[i].LTRB[2];
// 			glcPrimaryClus[j].LTRB[3] = (glcPrimaryClus[j].LTRB[3] > glcPrimaryClus[i].LTRB[3])
// 				? glcPrimaryClus[j].LTRB[3] : glcPrimaryClus[i].LTRB[3];
// 
// 			glcPrimaryClus[j].center_x = (glcPrimaryClus[j].center_x * glcPrimaryClus[j].tgt_num 
// 				+ glcPrimaryClus[i].center_x * glcPrimaryClus[i].tgt_num) 
// 				/ (glcPrimaryClus[j].tgt_num + glcPrimaryClus[i].tgt_num);
// 			glcPrimaryClus[j].center_y = (glcPrimaryClus[j].center_y * glcPrimaryClus[j].tgt_num 
// 				+ glcPrimaryClus[i].center_y * glcPrimaryClus[i].tgt_num) 
// 				/ (glcPrimaryClus[j].tgt_num + glcPrimaryClus[i].tgt_num);
// 
// 			glcPrimaryClus[j].angle = index;
// 			glcPrimaryClus[j].tgt_num += glcPrimaryClus[i].tgt_num;
// 			glcPrimaryClus[j].fus_num += glcPrimaryClus[i].fus_num;
// 
// 			memset(&glcPrimaryClus[i], 0, sizeof(FastLocateClus));
// 			effcCnt--;
// 			break;
// 		}
// 	}

	// 求取拟合矩形角度,修正旋转角度
	pLabel = label_mat;
	pBlockStamps = gpnLctPrimaryMarks;
	for(j = 0; j < gnLctBlobsHei; j++) {
		for(i = 0; i < gnLctBlobsWid; i++) {
			if(0 >= *pLabel) {
				pLabel++;
				pBlockStamps++;
				continue;
			}
			pClus = &(gflsLctPrimaryClus[*pLabel]);
			if(0 >= pClus->tgt_num) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			x = (i + 1) << 4;
			y = (j + 1) << 4;
			pClus->Ixx += (x - pClus->center_x) * (x - pClus->center_x);
			pClus->Iyy += (y - pClus->center_y) * (y - pClus->center_y);
			pClus->Ixy += (x - pClus->center_x) * (y - pClus->center_y);

			pLabel++;
			pBlockStamps++;
		}
	}

	// 计算拟合矩形角度,去除形状和角度不符者,拓展边界,获取四点坐标
	pClus = gflsLctPrimaryClus + 1;
	for(i = 1; i <= clus_cnt; i++) {
		if(0 >= pClus->tgt_num) {
			pClus++;
			continue;
		}

		// 去除形状不符者
		if(pClus->intrcpt > pClus->intrcpt_t) {
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		}

		// 去除尺寸不足者
		if(pClus->intrcpt_t < 32) {
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		}

		// 计算拟合矩形角度
		pClus->Ixx = pClus->Ixx / pClus->tgt_num;
		pClus->Iyy = pClus->Iyy / pClus->tgt_num;
		pClus->Ixy = pClus->Ixy / pClus->tgt_num;

		sqrtVal = sqrt(1.0 * (pClus->Ixx - pClus->Iyy) * (pClus->Ixx - pClus->Iyy) 
			+ 4.0 * pClus->Ixy * pClus->Ixy);
		nTmp1 = (pClus->Ixx + pClus->Iyy + (int)sqrtVal) >> 1;
		nTmp2 = (pClus->Ixx + pClus->Iyy - (int)sqrtVal) >> 1;

		nTmp2 = (abs(nTmp1) < abs(nTmp2)) ? nTmp1 : nTmp2;

		angle = (abs(pClus->Ixx) > abs(pClus->Iyy)) ? 
			AtanAngle180_fixed(nTmp2-pClus->Ixx, pClus->Ixy) : AtanAngle180_fixed(pClus->Ixy, nTmp2-pClus->Iyy);
		angle = (angle < 90) ? (angle + 90) : (angle - 90);
		pClus->Ixx = pClus->angle;
		pClus->Iyy = angle;

		index = abs(angle - pClus->angle);
		if(index > 18 && index < 162) {		// 去除两个矩形角度相差过多者
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		} else if(index >= 162) {		// 角度修正
// 			if(angle > pClus->angle) {
// 				pClus->angle = (angle * 3 + pClus->angle + 180) >> 2;
// 			} else {
// 				pClus->angle = ((angle + 180) * 3 + pClus->angle) >> 2;
// 			}
			pClus->angle = (angle + pClus->angle + 180) >> 1;
			pClus->angle = (pClus->angle < 180) ? pClus->angle : (pClus->angle - 180);
		} else {
// 			pClus->angle = (angle * 3 + pClus->angle) >> 2;
			pClus->angle = (angle + pClus->angle) >> 1;
		}

		// 适当拓宽边界
		pClus->intrcpt += 32;
		pClus->intrcpt_t += 48;

		// 根据角度扩宽边界,以补足旋转造成的缺损
		nTmp1 = abs(ryuSinShift(pClus->angle));
		nTmp2 = abs(ryuCosShift(pClus->angle));
		nTmp1 = (nTmp1 > nTmp2) ? nTmp1 : nTmp2;
		pClus->intrcpt = (pClus->intrcpt<<10) / nTmp1;
		pClus->intrcpt_t = (pClus->intrcpt_t<<10) / nTmp1;

		A = ryuCosShift(pClus->angle+90);
		B = ryuSinShift(pClus->angle+90);
		A_t = ryuCosShift(pClus->angle+180);
		B_t = ryuSinShift(pClus->angle+180);

		// 修正中心点参数
		pClus->cen_intr = (A * pClus->center_x + B * pClus->center_y) >> 10;
		pClus->cen_intr_t = (A_t * pClus->center_x + B_t * pClus->center_y) >> 10;

		// 获取四点坐标
		pClus->rect_ptX[0] = ((pClus->cen_intr-pClus->intrcpt) * B_t - (pClus->cen_intr_t+pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[0] = ((pClus->cen_intr_t+pClus->intrcpt_t) * A - (pClus->cen_intr-pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[1] = ((pClus->cen_intr+pClus->intrcpt) * B_t - (pClus->cen_intr_t+pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[1] = ((pClus->cen_intr_t+pClus->intrcpt_t) * A - (pClus->cen_intr+pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[2] = ((pClus->cen_intr-pClus->intrcpt) * B_t - (pClus->cen_intr_t-pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[2] = ((pClus->cen_intr_t-pClus->intrcpt_t) * A - (pClus->cen_intr-pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[3] = ((pClus->cen_intr+pClus->intrcpt) * B_t - (pClus->cen_intr_t-pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[3] = ((pClus->cen_intr_t-pClus->intrcpt_t) * A - (pClus->cen_intr+pClus->intrcpt) * A_t)>>10;

		pClus++;
	}
	
	// 排序
	pClus = gflsLctPrimaryClus;
	for(i = clus_cnt; i > 0; i--) {
		for(j = i - 1; j > 0; j--) {
			if(pClus[j].tgt_num < pClus[i].tgt_num) {
				temClus = pClus[j];
				pClus[j] = pClus[i];
				pClus[i] = temClus;
			}
		}
	}
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	IplImage * show_img3 = cvCreateImage(cvGetSize(show_img2), 8, 3);
	IplImage * show_img4 = cvCreateImage(cvGetSize(show_img2), 8, 3);
	pClus = gflsLctPrimaryClus;

	cvCopy(show_img2, show_img4);

	for(i = 1; i <= effcCnt; i++) {
		if(pClus[i].tgt_num < 0)
			break;
		cvCopy(show_img2, show_img3);

		for(int j = 0; j < 4; j++) {
			sprintf(txt, "%d", j);
			cvPutText(show_img3, txt, cvPoint(pClus[i].rect_ptX[j]/4+2, pClus[i].rect_ptY[j]/4), &font, CV_RGB(255,255,0));
			cvCircle(show_img3, cvPoint(pClus[i].rect_ptX[j]/4, pClus[i].rect_ptY[j]/4), 2, CV_RGB(255, 255, 0), CV_FILLED);

			sprintf(txt, "%d", i);
			cvPutText(show_img4, txt, cvPoint(pClus[i].rect_ptX[j]/4+2, pClus[i].rect_ptY[j]/4), &font, CV_RGB(255,255,0));
			cvCircle(show_img4, cvPoint(pClus[i].rect_ptX[j]/4, pClus[i].rect_ptY[j]/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		}

		pLabel = label_mat;
		for(int i_dbg = 0; i_dbg < gnLctBlobsHei; i_dbg++) {
			for(int j_dbg = 0; j_dbg < gnLctBlobsWid; j_dbg++) {
				if(*pLabel == pClus[i].clus_label) {
					cvRectangle(show_img3, cvPoint(j_dbg*4, i_dbg*4), 
						cvPoint((j_dbg+1)*4-1, (i_dbg+1)*4-1), CV_RGB(0, 255, 0));
					cvRectangle(show_img4, cvPoint(j_dbg*4, i_dbg*4), 
						cvPoint((j_dbg+1)*4-1, (i_dbg+1)*4-1), CV_RGB(0, 255, 0));

				}
				pLabel++;
			}
		}
		printf("\nLabel %d\n", pClus[i].clus_label);
		printf("tgt_num= %d, fus_num= %d\n", pClus[i].tgt_num, pClus[i].fus_num);
		printf("center= (%d, %d), angle= %d\n", pClus[i].center_x, pClus[i].center_y, pClus[i].angle);
		printf("grad_angle = %d, rect_angle = %d\n", pClus[i].Ixx, pClus[i].Iyy);
		printf("cen_intr= %d, cen_intr_t= %d\n", pClus[i].cen_intr, pClus[i].cen_intr_t);
		printf("intrcpt= %d, intrcpt_t= %d\n", pClus[i].intrcpt, pClus[i].intrcpt_t);


		sprintf(txt, "%d", i);
		cvCircle(show_img3, cvPoint(pClus[i].center_x/4, pClus[i].center_y/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		cvCircle(show_img4, cvPoint(pClus[i].center_x/4, pClus[i].center_y/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		cvPutText(show_img3, txt, cvPoint(pClus[i].center_x/4+4, pClus[i].center_y/4), &font, CV_RGB(0,0,255));
		cvNamedWindow("聚类分步结果");
		cvShowImage("聚类分步结果", show_img3);
		
		cvWaitKey();		
	}
	cvDestroyWindow("聚类分步结果");
	cvNamedWindow("聚类结果");
	cvShowImage("聚类结果", show_img4);
	cvWaitKey();
	cvReleaseImage(&show_img3);
	cvReleaseImage(&show_img4);
#endif
#endif
#endif

	return effcCnt;
}


int FNC_LBC02_FilterBorderBlob(int orin_thre, int thre1, int thre2)
{
	int nRet = 0;
	int i = 0, j = 0, k = 0;

	int offset[8][8] = {0};
	int w1 = gnLctBlobsWid, w2 = gnLctBlobsWid<<1;

	int * pBlockStamps = 0;
	int orin1 = 0, orin2 = 0, diff1 = 0, diff2 = 0;
	int index = 0, tBlkStm = 0;
	int status = 0;

	offset[0][0] = 1;		offset[0][1] = 2;		offset[0][2] = w1+1;	offset[0][3] = w1+2;
	offset[0][4] = -1;		offset[0][5] = -2;		offset[0][6] = -w1-1;	offset[0][7] = -w1-2;
	offset[1][0] = w1+1;	offset[1][1] = w1+2;	offset[1][2] = w2+1;	offset[1][3] = w2+2;
	offset[1][4] = -w1-1;	offset[1][5] = -w1-2;	offset[1][6] = -w2-1;	offset[1][7] = -w2-2;
	offset[2][0] = w1+1;	offset[2][1] = w1+2;	offset[2][2] = w2+1;	offset[2][3] = w2+2;
	offset[2][4] = -w1-1;	offset[2][5] = -w1-2;	offset[2][6] = -w2-1;	offset[2][7] = -w2-2;
	offset[3][0] = w1;		offset[3][1] = w1+1;	offset[3][2] = w2;		offset[3][3] = w2+1;
	offset[3][4] = -w1;		offset[3][5] = -w1-1;	offset[3][6] = -w2;		offset[3][7] = -w2-1;
	offset[4][0] = w1;		offset[4][1] = w1-1;	offset[4][2] = w2;		offset[4][3] = w2-1;
	offset[4][4] = -w1;		offset[4][5] = -w1+1;	offset[4][6] = -w2;		offset[4][7] = -w2+1;
	offset[5][0] = w1-1;	offset[5][1] = w1-2;	offset[5][2] = w2-1;	offset[5][3] = w2-2;
	offset[5][4] = -w1+1;	offset[5][5] = -w1+2;	offset[5][6] = -w2+1;	offset[5][7] = -w2+2;
	offset[6][0] = w1-1;	offset[6][1] = w1-2;	offset[6][2] = w2-1;	offset[6][3] = w2-2;
	offset[6][4] = -w1+1;	offset[6][5] = -w1+2;	offset[6][6] = -w2+1;	offset[6][7] = -w2+2;
	offset[7][0] = -1;		offset[7][1] = -2;		offset[7][2] = w1-1;	offset[7][3] = w1-2;
	offset[7][4] = 1;		offset[7][5] = 2;		offset[7][6] = -w1+1;	offset[7][7] = -w1+2;

	// 首次过滤处于条码边缘的点,基于条码垂直方向
	pBlockStamps = gpnLctPrimaryMarks + (gnLctBlobsWid<<1) + 2;
	for(j = gnLctBlobsHei - 4; j > 0; j--) {
		for(i = gnLctBlobsWid - 4; i > 0; i--) {
			if(*pBlockStamps>>27) {
				orin1 = (*pBlockStamps&0xff);		// 取出角度
				index = (orin1 >= 90) ? (orin1 - 90) : (orin1 + 90);	// 取垂直方向mask
				index = (index<<1) / 45;
				for(k = 7; k >= 0; k--) {
					tBlkStm = *(pBlockStamps+offset[index][k]);
					orin2 = (tBlkStm&0xff);
					diff1 = ((tBlkStm>>8)&0xff) - ((*pBlockStamps>>8)&0xff);
					diff2 = ((tBlkStm>>16)&0xff) - ((*pBlockStamps>>16)&0xff);
					if((diff1 > thre1 || diff2 > thre2) 
						&& (abs(orin2-orin1) <= orin_thre || abs(orin2-orin1) >= 180-orin_thre)) {
							*pBlockStamps &= 0x07ffffff;
							break;
					}
				}
			}
			pBlockStamps++;
		}
		pBlockStamps += 4;
	}

	// 二次过滤在条码方向上无连接的点
	pBlockStamps = gpnLctPrimaryMarks + (gnLctBlobsWid<<1) + 2;
	for(j = gnLctBlobsHei - 4; j > 0; j--) {
		for(i = gnLctBlobsWid - 4; i > 0; i--) {
			if(*pBlockStamps>>27) {
				orin1 = (*pBlockStamps&0xff);		// 取出角度
				index = (orin1<<1) / 45;
				status = 0;
				for(k = 7; k >= 0; k--) {
					tBlkStm = *(pBlockStamps+offset[index][k]);
					orin2 = (tBlkStm&0xff);
					if((abs(orin2-orin1) <= orin_thre || abs(orin2-orin1) >= 180-orin_thre)
						&& (tBlkStm>>26)) {
							status = 1;
							break;
					}
				}
				*pBlockStamps &= (status) ? 0xffffffff : 0x07ffffff;
			}
			pBlockStamps++;
		}
		pBlockStamps += 4;
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	
	pBlockStamps = gpnLctPrimaryMarks;
	for(int m = 0; m < gnLctBlobsHei; m++) {
		for(int n = 0; n < gnLctBlobsWid; n++) {
			if(7 == *pBlockStamps>>24) {			
// 				cvRectangle(iplShendu1, cvPoint(n*4, m*4), 
// 					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 0, 0));
// 				cvRectangle(iplShendu2, cvPoint(n*4, m*4), 
// 					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 0, 0));			
				cvRectangle(show_img2, cvPoint(n*4, m*4), 
					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 128, 0));
			}
			pBlockStamps++;
		}
	}
	
// 	cvNamedWindow("iplShendu1");
// 	cvShowImage("iplShendu1", iplShendu1);
// 	cvNamedWindow("iplShendu2");
// 	cvShowImage("iplShendu2", iplShendu2);
// 	cvWaitKey();
	
#endif
#endif
#endif

	return 0;
}
*/

int FNC_LBC02_BlobCluster2Line()
{
	int nRet = 0;
	int i = 0, j = 0, k = 0, i1 = 0, j1 = 0, base = 0;
	int count = 0, idx = 0;
	int offset = 0;

	int a = 0, b = 0, trigflag = 0, angle = 0, angle_f = 0, angleflag = 0;
	int grad = 0, grad_f = 0, density = 0, density_f = 0;
	int ele_ang = 0, ele_cnt = 0, length = 0;
	int gap = 0, x = 0, y = 0, dx = 0, dy = 0;
	int x0 = 0, y0 = 0, dx0 = 0, dy0 = 0, xflag = 0;

	int line_end[2][2] = {{0,0}, {0,0}};		// 表示直线两端点的坐标
	int good_line = 0, line_cnt = 1;

	int * pBlockStamps = gpnLctPrimaryMarks, * pStamp = 0;
	unsigned char * pBlobMask = gucLctBlobMask, * pMask = 0;
	RyuPoint * pBlobSeq = gpptLctBlobSeq;
	LocateClusLine * pBlobClusLine = glclLctBlobClusLine;

	LocateClusLine tmpClusLine;

	const int shift = 16;
	const int lineGap = 1;
	const int lineLength = 4;
	const int angdiff_thre = 18;

	// stage 1. 收集所有的非零点，将它们存入数组中，并在mask中标记为1
	for(i = 0; i < gnLctBlobsHei; i++) {
		offset = i * gnLctBlobsWid;
		pStamp = pBlockStamps + offset;
		pMask = pBlobMask + offset;
		for(j = 0; j < gnLctBlobsWid; j++) {
			if(*pStamp>>27) {
				*pMask = 0xff;
				pBlobSeq->x = j;
				pBlobSeq->y = i;
				count++;
				pBlobSeq++;
			} else {
				*pMask = 0;
			}
			pStamp++;
			pMask++;
		}
	}

	// stage 2. 以随机顺序处理所有的非零点
	pBlobSeq = gpptLctBlobSeq;
	for( ; count > 0; count--) {
		// 选择随机点
		idx = rand();
		idx = idx % count;

		ele_cnt = ele_ang = angleflag = length = 0;

		j = line_end[0][0] = line_end[1][0] = pBlobSeq[idx].x;
		i = line_end[0][1] = line_end[1][1] = pBlobSeq[idx].y;

		// 通过重新赋值删除此点
		pBlobSeq[idx].x = pBlobSeq[count-1].x;
		pBlobSeq[idx].y = pBlobSeq[count-1].y;

		// 检查此点是否已经被排除 (例如属于别的直线)
		offset = i*gnLctBlobsWid+j;
		if(!(pBlobMask[offset] & 0xf0))
			continue;

		// 如果超过阈值，就从当前点沿各个方向寻找并提取线段
		density = (pBlockStamps[offset]>>16) & 0xff;
		grad  = (pBlockStamps[offset]>>8) & 0xff;
		angle = pBlockStamps[offset] & 0xff;
		trigflag = (angle <= 90) ? 1: -1;
		a = ryuSinShift(angle);
		b = trigflag * ryuCosShift(angle);
		x0 = j;
		y0 = i;
		ele_cnt = length = 1;
		angleflag = (angle < angdiff_thre || angle > 180 - angdiff_thre);
		ele_ang = (angleflag && angle < 90) ? (angle + 180) : angle;

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
		cvCopy(show_img2, show_img4);
		// 绘制起始点
		cvRectangle(show_img4, cvPoint(x0 * 4, y0 * 4), cvPoint(x0 * 4 + 3, y0 * 4 + 3), CV_RGB(0,255,0));
		printf("\n");
		printf("起始点坐标：%d, %d   搜索角度：%d\n", x0, y0, angle);
		cvNamedWindow("Feature Cluster Line by Step");
		cvShowImage("Feature Cluster Line by Step", show_img4);
		cvWaitKey();
#endif
#endif
#endif

		// 计算方向上点追踪的x,y坐标步长
		// 此处逻辑示意图见笔记本
		if(a < b) {
			xflag = 1;
			dx0 = -1;
			dy0 = (a << shift) / b * (0 - trigflag);
			y0 = (y0 << shift) + (1 << (shift-1));
		} else {
			xflag = 0;
			dy0 = 0 - trigflag;
			dx0 = 0 - (b << shift) / a;
			x0 = (x0 << shift) + (1 << (shift-1));
		}

		for(k = 0; k < 2; k++) {
			gap = 0; dx = dx0; dy = dy0;

			if(k > 0)
				dx = -dx, dy = -dy;

			x = x0 + dx;
			y = y0 + dy;

			// 用固定步长沿此点的两个方向寻找直线上的点
			// 直至图像边届或出现过大的跳跃
			for( ;; x += dx, y += dy)
			{
				if( xflag ) {
					j1 = x;
					i1 = y >> shift;
				} else {
					j1 = x >> shift;
					i1 = y;
				}

				// 图像边界
				if(j1 < 0 || j1 >= gnLctBlobsWid || i1 < 0 || i1 >= gnLctBlobsHei) {
					length += lineGap;
					break;
				}

				base = i1 * gnLctBlobsWid + j1;
				pMask = pBlobMask + base;
				density_f = (pBlockStamps[base]>>16) & 0xff;
				grad_f  = (pBlockStamps[base] >> 8) & 0xff;
				angle_f = pBlockStamps[base] & 0xff;

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
				// 绘制测量点
				if(*pMask) {
					cvRectangle(show_img4, cvPoint(j1 * 4, i1 * 4), cvPoint(j1 * 4 + 3, i1 * 4 + 3), CV_RGB(128,0,0));
					printf("搜索点坐标%d, %d  mask%d  该点角度%d  角度差%d\n", j1, i1, *pMask, (*pStamp & 0xff), 
						ANGLE_DIFF(abs(angle_f-angle)));
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
				}
#endif
#endif
#endif

				// 对于聚类点：
				// 更新线的端点,
				// 重令gap为0
				if(*pMask && angdiff_thre > ryuCycleDistance(angle, angle_f, 180)) {
					gap = 0;
					ele_cnt++;
					grad += grad_f;
					density += density_f;
					ele_ang += (angleflag && angle_f < 90) ? (angle_f + 180) : angle_f;
					line_end[k][1] = i1;
					line_end[k][0] = j1;

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
					// 标记测量点
					cvRectangle(show_img4, cvPoint(j1 * 4, i1 * 4), cvPoint(j1 * 4 + 3, i1 * 4 + 3), CV_RGB(255,255,0));
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
#endif
#endif
#endif

				// 空隙数超过阈值，过大的跳跃
				} else if (++gap > lineGap) {

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
					printf("空隙过大，结束该方向检测\n");
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
#endif
#endif
#endif

					break;
				}
				length++;
			}
		}

		if(0 >= ele_cnt)
			continue;

		// 判断此线是否是满足长度lineLength的直线
		good_line = abs(line_end[1][0] - line_end[0][0]) >= lineLength ||
			abs(line_end[1][1] - line_end[0][1]) >= lineLength;

		if(!good_line) 
			continue;

		// 重新遍历此线来:
		// 置线上的点mask为0
		// 对good_line来减去此线在累加器上的作用
		for(k = 0; k < 2; k++) {
			dx = dx0, dy = dy0;

			if(k > 0)
				dx = -dx, dy = -dy;

			x = x0 + dx;
			y = y0 + dy;

			for( ; ; x += dx, y += dy) {
				if( xflag ) {
					j1 = x;
					i1 = y >> shift;
				} else {
					j1 = x >> shift;
					i1 = y;
				}

				// 图像边界
				if(j1 < 0 || j1 >= gnLctBlobsWid || i1 < 0 || i1 >= gnLctBlobsHei)
					break;

				pMask = pBlobMask + i1 * gnLctBlobsWid + j1;
				if(*pMask) {
					*pMask &= 0x0f;		// 去除首选点
				}

				if(i1 == line_end[k][1] && j1 == line_end[k][0])
					break;
			}
		}

		//  把此线端点存入lines结构
		if(good_line) {
			pBlobClusLine[line_cnt].label = 0;
			pBlobClusLine[line_cnt].line.pt0.x = line_end[0][0];
			pBlobClusLine[line_cnt].line.pt0.y = line_end[0][1];
			pBlobClusLine[line_cnt].line.pt1.x = line_end[1][0];
			pBlobClusLine[line_cnt].line.pt1.y = line_end[1][1];
			pBlobClusLine[line_cnt].center.x = (line_end[0][0] + line_end[1][0]) >> 1;
			pBlobClusLine[line_cnt].center.y = (line_end[0][1] + line_end[1][1]) >> 1;
			pBlobClusLine[line_cnt].angle = angle;
			pBlobClusLine[line_cnt].element = ele_cnt;
			pBlobClusLine[line_cnt].length = length - 2 * lineGap;
			ele_ang /= ele_cnt;
			pBlobClusLine[line_cnt].avg_angle = (180 <= ele_ang) ? (ele_ang - 180) : ele_ang;
			grad /= ele_cnt;
			pBlobClusLine[line_cnt].avg_grad = (grad > 0xff) ? 0xff : grad;
			pBlobClusLine[line_cnt].density = density;
			if(++line_cnt >= gnLctMaxClusCnt)
				break;
		}

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
		cvLine(show_img4, cvPoint(line_end[0][0]*4+2, line_end[0][1]*4+2), cvPoint(line_end[1][0]*4+2, line_end[1][1]*4+2), 
			CV_RGB(255,255,0), 1);
		cvNamedWindow("Feature Cluster Line by Step");
		cvShowImage("Feature Cluster Line by Step", show_img4);
		cvWaitKey();
#endif
#endif
#endif

	}

	line_cnt -= 1;

	// 根据长度对测得直线进行排序
	for(i = 1; i <= line_cnt; i++) {
		for(j = i + 1; j <= line_cnt; j++) {
			if(pBlobClusLine[i].element < pBlobClusLine[j].element) {
				tmpClusLine = pBlobClusLine[i];
				pBlobClusLine[i] = pBlobClusLine[j];
				pBlobClusLine[j] = tmpClusLine;
			}
		}
	}

#ifdef _DEBUG_LOCATE
#ifdef _DEBUG_FASTLOCATE
	cvDestroyWindow("Feature Cluster Line by Step");

#if	0
	for(i = 1; i <= line_cnt; i++) {
		cvCopy(show_img2, show_img3);
		cvLine(show_img3, cvPoint(pBlobClusLine[i].line.pt0.x*4+2, pBlobClusLine[i].line.pt0.y*4+2), 
			cvPoint(pBlobClusLine[i].line.pt1.x*4+2, pBlobClusLine[i].line.pt1.y*4+2), CV_RGB(0,255,0));
		cvNamedWindow("Feature Cluster Line");
		cvShowImage("Feature Cluster Line", show_img3);
		cvWaitKey();
	}
#endif
#if	1
	cvCopy(show_img, show_img3);
	for(i = line_cnt; i > 0; i--) {
		cvLine(show_img3, cvPoint(pBlobClusLine[i].line.pt0.x*4+2, pBlobClusLine[i].line.pt0.y*4+2), 
			cvPoint(pBlobClusLine[i].line.pt1.x*4+2, pBlobClusLine[i].line.pt1.y*4+2), 
			CV_RGB(rand()%192+63,rand()%192+63,rand()%192+63), 1);
	}

	printf("Find %d Feature Cluster Lines\n", line_cnt);
	cvNamedWindow("Feature Cluster Line");
	cvShowImage("Feature Cluster Line", show_img3);
	//cvWaitKey();
#endif
#endif
#endif

	nRet = line_cnt;
nExit:
	return nRet;
}


inline int fnc_inline_avg_angle(int ang0, int cnt0, int ang1, int cnt1)
{
	int avg_ang = 0;

	if(90 < abs(ang0 - ang1)) {
		if(ang0 > ang1) {
			avg_ang = (ang0 * cnt0 + (ang1 + 180) * cnt1) / (cnt0 + cnt1);
		} else {
			avg_ang = ((ang0 + 180) * cnt0 + ang1 * cnt1) / (cnt0 + cnt1);
		}
		avg_ang = (avg_ang < 180) ? avg_ang : (avg_ang - 180);
	} else {
		avg_ang = FNC_AVG_VALUE(ang0, cnt0, ang1, cnt1);
	}

	return avg_ang;
}


int FNC_LBC03_LineCluster2Area(int line_cnt)
{
	int nRet = 0;
	int i = 0, j = 0;
	int clus_cnt = 1, isMatch = 0, effcclus_cnt = 0, goodclus_cnt = 0;
	int theta0 = 0, dist0 = 0, dist = 0, idx0 = 0, idx = 0;

	LocateClusLine * pBlobClusLine = glclLctBlobClusLine;
	LocateClusArea * pBlobClusArea = glcaLctBlobClusArea, * pBCA = 0;
	LocateClusArea tmpBlobClusArea;

	int A = 0, B = 0, AT = 0, BT = 0, C0 = 0, C1 = 0, CT0 = 0, CT1 = 0;

	const int distthreshold = (2 << TRIGONOMETRIC_SHIFT_DIGIT);
	const int distthreshold2 = (1 << TRIGONOMETRIC_SHIFT_DIGIT) + (1 << (TRIGONOMETRIC_SHIFT_DIGIT-1));
	const int onelineclusthre = 5;
	const int intcpt_ext = 24, ontcpt_ext = 32;
	const int densitythreshold = 200;

	// 特征线聚类
	for(i = 1; i <= line_cnt; i++) {
		isMatch = 0;
		theta0 = pBlobClusLine[i].avg_angle + 90;
		dist0 = pBlobClusLine[i].center.x * ryuCosShift(theta0) + pBlobClusLine[i].center.y * ryuSinShift(theta0);
		idx0 = pBlobClusLine[i].label;
		// 若所在聚类区域为子节点，则计算时指向父区域
		pBCA = &pBlobClusArea[idx0];
		while(idx0 > 0 && pBCA->parent > 0) {
			idx0 = pBCA->parent;
			pBCA = &pBlobClusArea[idx0];
		}

		// 当聚类中心直线长度已明显小于聚类长度时，则不继续作为中心进行聚类
		if(idx0 && pBlobClusLine[i].length < pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length / 4) {
			continue;
		}

		for(j = i + 1; j <= line_cnt; j++) {
			idx = pBlobClusLine[j].label;
			pBCA = &pBlobClusArea[idx];
			while(idx > 0 && pBCA->parent > 0) {
				idx = pBCA->parent;
				pBCA = &pBlobClusArea[idx];
			}

			if(idx0 && idx0 == idx)
				continue;

// 			// 元素数验证
// 			if(pBlobClusLine[i].element > 2 * pBlobClusLine[j].element)
// 				continue;
// 
// 			// 长度验证
// 			if(pBlobClusLine[i].length > 2 * pBlobClusLine[j].length)
// 				continue;
// 
// 			if(idx0 && pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length > 2 * pBlobClusLine[j].length)
// 				continue;

			// 角度验证
			if(18 < ryuCycleDistance(pBlobClusLine[i].avg_angle, pBlobClusLine[j].avg_angle, 180))
				continue;

			A = abs(pBlobClusLine[i].center.x - pBlobClusLine[j].center.x);
			B = abs(pBlobClusLine[i].center.y - pBlobClusLine[j].center.y);

			// 中心点距离验证
			if(A > pBlobClusLine[i].length / 2 || B > pBlobClusLine[i].length / 2)
				continue;

			// 直线距离
			dist = pBlobClusLine[j].center.x * ryuCosShift(theta0) + pBlobClusLine[j].center.y * ryuSinShift(theta0);
			if(abs(dist - dist0) > distthreshold)
				continue;

			// 修改新的长度聚类准则，将邻近短线加入聚类，更加强调聚类的集中性
			if(pBlobClusLine[j].length < 3 * pBlobClusLine[i].length / 4) {
				// 若短线不够接近，则过滤
				if(abs(dist - dist0) > distthreshold2) {
					continue;
				}
				// 如果i元素有已知聚类，且i元素长度与聚类长度接近，则i元素具备聚类周围短线的能力；否则过滤
				if(idx0 && pBlobClusLine[i].length < 3 * pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length / 4) {
					continue;
				}
			}


			// 聚类添加元素
			if(idx0) {
				// 聚类合并
				if(idx) {
					if(idx0 < idx) {
						pBlobClusArea[idx].parent = idx0;
						pBlobClusArea[idx0].linecnt += pBlobClusArea[idx].linecnt;
						pBlobClusArea[idx0].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].center.x, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].center.y, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].grad, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].angle, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].element += pBlobClusArea[idx].element;
						pBlobClusArea[idx0].density += pBlobClusArea[idx].density;
					} else {
						pBlobClusArea[idx0].parent = idx;
						pBlobClusArea[idx].linecnt += pBlobClusArea[idx0].linecnt;
						pBlobClusArea[idx].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].center.x, pBlobClusArea[idx].element);
						pBlobClusArea[idx].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].center.y, pBlobClusArea[idx].element);
						pBlobClusArea[idx].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].grad, pBlobClusArea[idx].element);
						pBlobClusArea[idx].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element, 
							pBlobClusArea[idx].angle, pBlobClusArea[idx].element);
						pBlobClusArea[idx].element += pBlobClusArea[idx0].element;
						pBlobClusArea[idx].density += pBlobClusArea[idx0].density;
						idx0 = idx;
					}
				}
				// 添加一个元素
				else {
					pBlobClusLine[j].label = idx0;
					pBlobClusArea[idx0].linecnt += 1;
					pBlobClusArea[idx0].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element, 
						pBlobClusLine[j].center.x, pBlobClusLine[j].element);
					pBlobClusArea[idx0].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element, 
						pBlobClusLine[j].center.y, pBlobClusLine[j].element);
					pBlobClusArea[idx0].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element, 
						pBlobClusLine[j].avg_grad, pBlobClusLine[j].element);
					pBlobClusArea[idx0].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element, 
						pBlobClusLine[j].angle, pBlobClusLine[j].element);
					pBlobClusArea[idx0].element += pBlobClusLine[j].element;
					pBlobClusArea[idx0].density += pBlobClusLine[j].density;
				}
			}
			// 加入已有聚类
			else if(idx) {
				idx0 = idx;
				pBlobClusLine[i].label = idx;
				pBlobClusArea[idx].linecnt += 1;
				pBlobClusArea[idx].center.x = FNC_AVG_VALUE(pBlobClusArea[idx].center.x, pBlobClusArea[idx].element, 
					pBlobClusLine[i].center.x, pBlobClusLine[i].element);
				pBlobClusArea[idx].center.y = FNC_AVG_VALUE(pBlobClusArea[idx].center.y, pBlobClusArea[idx].element, 
					pBlobClusLine[i].center.y, pBlobClusLine[i].element);
				pBlobClusArea[idx].grad = FNC_AVG_VALUE(pBlobClusArea[idx].grad, pBlobClusArea[idx].element, 
					pBlobClusLine[i].avg_grad, pBlobClusLine[i].element);
				pBlobClusArea[idx].angle = fnc_inline_avg_angle(pBlobClusArea[idx].angle, pBlobClusArea[idx].element, 
					pBlobClusLine[i].angle, pBlobClusLine[i].element);
				pBlobClusArea[idx].element += pBlobClusLine[i].element;
				pBlobClusArea[idx].density += pBlobClusLine[i].density;
			}
			// 生成新聚类
			else {
				pBlobClusArea[clus_cnt].flag = 0;
				idx0 = pBlobClusArea[clus_cnt].label = clus_cnt;
				pBlobClusArea[clus_cnt].parent = 0;
				pBlobClusLine[i].label = pBlobClusLine[j].label = clus_cnt;
				pBlobClusArea[clus_cnt].linecnt = 2;
				pBlobClusArea[clus_cnt].maxlineidx = i;
				pBlobClusArea[clus_cnt].element = pBlobClusLine[i].element + pBlobClusLine[j].element;
				pBlobClusArea[clus_cnt].center.x = FNC_AVG_VALUE(pBlobClusLine[i].center.x, pBlobClusLine[i].element, 
					pBlobClusLine[j].center.x, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].center.y = FNC_AVG_VALUE(pBlobClusLine[i].center.y, pBlobClusLine[i].element, 
					pBlobClusLine[j].center.y, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].grad = FNC_AVG_VALUE(pBlobClusLine[i].avg_grad, pBlobClusLine[i].element, 
					pBlobClusLine[j].avg_grad, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].angle = fnc_inline_avg_angle(pBlobClusLine[i].angle, pBlobClusLine[i].element, 
					pBlobClusLine[j].angle, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].density = pBlobClusLine[i].density + pBlobClusLine[j].density;
				clus_cnt++;
			}
			isMatch = 1;
		}

		// 单线形成聚类
		if(0 == isMatch && 0 == idx0) {
			if(pBlobClusLine[i].length > onelineclusthre) {
				pBlobClusArea[clus_cnt].flag = 0;
				pBlobClusArea[clus_cnt].label = clus_cnt;
				pBlobClusArea[clus_cnt].parent = 0;
				pBlobClusArea[clus_cnt].linecnt = 1;
				pBlobClusArea[clus_cnt].maxlineidx = i;
				pBlobClusArea[clus_cnt].element = pBlobClusLine[i].element;
				pBlobClusArea[clus_cnt].center = pBlobClusLine[i].center;
				pBlobClusArea[clus_cnt].grad = pBlobClusLine[i].avg_grad;
				pBlobClusArea[clus_cnt].angle = pBlobClusLine[i].avg_angle;
				pBlobClusArea[clus_cnt].density = pBlobClusLine[i].density;
				pBlobClusLine[i].label = idx0 = clus_cnt;
				clus_cnt++;
			}
		}
	}

	clus_cnt -= 1;
	
	// 矫正线标记，计算特征区域边界参数
	for(j = 1; j <= line_cnt; j++) {
		idx = pBlobClusLine[j].label;
		pBCA = &pBlobClusArea[idx];
		while(idx > 0 && pBCA->parent > 0) {
			idx = pBCA->parent;
			pBCA = &pBlobClusArea[idx];
		}
		pBlobClusLine[j].label = idx;

		if(0 == idx) {
			continue;
		}

		A = ryuCosShift(pBlobClusArea[idx].angle + 90);
		B = ryuSinShift(pBlobClusArea[idx].angle + 90);
		C0 = A * ((pBlobClusLine[j].line.pt0.x + 1)<<4) + B * ((pBlobClusLine[j].line.pt0.y+1)<<4);
		C0 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		C1 = A * ((pBlobClusLine[j].line.pt1.x + 1)<<4) + B * ((pBlobClusLine[j].line.pt1.y+1)<<4);
		C1 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		AT = ryuCosShift(pBlobClusArea[idx].angle + 180);
		BT = ryuSinShift(pBlobClusArea[idx].angle + 180);
		CT0 = AT * ((pBlobClusLine[j].line.pt0.x + 1)<<4) + BT * ((pBlobClusLine[j].line.pt0.y+1)<<4);
		CT0 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		CT1 = AT * ((pBlobClusLine[j].line.pt1.x + 1)<<4) + BT * ((pBlobClusLine[j].line.pt1.y+1)<<4);
		CT1 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		if(pBlobClusArea[idx].flag) {
			pBlobClusArea[idx].min_intcpt = RYUMIN(pBlobClusArea[idx].min_intcpt, C0);
			pBlobClusArea[idx].min_intcpt = RYUMIN(pBlobClusArea[idx].min_intcpt, C1);
			pBlobClusArea[idx].max_intcpt = RYUMAX(pBlobClusArea[idx].max_intcpt, C0);
			pBlobClusArea[idx].max_intcpt = RYUMAX(pBlobClusArea[idx].max_intcpt, C1);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(pBlobClusArea[idx].min_ontcpt, CT0);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(pBlobClusArea[idx].min_ontcpt, CT1);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(pBlobClusArea[idx].max_ontcpt, CT0);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(pBlobClusArea[idx].max_ontcpt, CT1);
		} else {
			pBlobClusArea[idx].min_intcpt = RYUMIN(C0, C1);
			pBlobClusArea[idx].max_intcpt = RYUMAX(C0, C1);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(CT0, CT1);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(CT0, CT1);
			pBlobClusArea[idx].flag = 1;
		}
	}

	// 根据聚类标号、形状特征等筛选有效区域，计算有效区域边界四点
	for(i = 1; i <= clus_cnt; i++) {
		// 标记子区域及空区域标识为0
		if(0 != pBlobClusArea[i].parent || 0 >= pBlobClusArea[i].linecnt) {
			pBlobClusArea[i].flag = 0;
			continue;
		}

		if(densitythreshold > pBlobClusArea[i].density) {
			pBlobClusArea[i].flag = 0;
			continue;
		}

		C0  = pBlobClusArea[i].max_intcpt - pBlobClusArea[i].min_intcpt;
		CT0 = pBlobClusArea[i].max_ontcpt - pBlobClusArea[i].min_ontcpt;
		// 版本2.3.2修改20170228
		// 过滤掉超大型区域，优化算法时间
		//printf("[%d] C0=%d, CT0=%d, AREA=%d\n", i, C0, CT0, C0*CT0);
		if(C0 > LOCATE_OUTPUT_MAXHEIGHT || CT0 > LOCATE_OUTPUT_MAXWIDTH 
			|| C0 * CT0 > LOCATE_OUTPUT_MAXAREAS) {
			pBlobClusArea[i].flag = 0;
			continue;
		}

		// 标记为effcclus
		effcclus_cnt++;

		// 标记形状不符一般条形码特征区域的标识为1
		//if(C0 > CT0) {
		if(C0 > 2 * CT0) {
			pBlobClusArea[i].flag = 1;
		}
		// 标记形状符合一般条形码特征区域的标识为2，认为是goodclus
		else {
			pBlobClusArea[i].flag = 2;
			goodclus_cnt++;
		}

		// 换算为图像实际坐标系
		pBlobClusArea[i].center.x = (pBlobClusArea[i].center.x + 1) << 4;
		pBlobClusArea[i].center.y = (pBlobClusArea[i].center.y + 1) << 4;

		// 适当拓宽边界，计算边界四点
		pBlobClusArea[i].min_intcpt -= intcpt_ext;
		pBlobClusArea[i].max_intcpt += intcpt_ext;
		pBlobClusArea[i].min_ontcpt -= ontcpt_ext;
		pBlobClusArea[i].max_ontcpt += ontcpt_ext;

// 		C0  += (intcpt_ext << 1);
// 		CT0 += (ontcpt_ext << 1);
// 
// 		// 根据角度扩宽边界,以补足旋转造成的缺损
// 		A = abs(ryuSinShift(pBlobClusArea[i].angle));
// 		B = abs(ryuCosShift(pBlobClusArea[i].angle));
// 		A = RYUMAX(A, B);
// 		C1 = (((C0<<10) / A - C0) >> 1) + 1;
// 		CT1 = (((CT0<<10) / A - CT0) >> 1) + 1;
// 
// 		pBlobClusArea[i].min_intcpt -= C1;
// 		pBlobClusArea[i].max_intcpt += C1;
// 		pBlobClusArea[i].min_ontcpt -= CT1;
// 		pBlobClusArea[i].max_ontcpt += CT1;

		// 获取四点坐标
		A = ryuCosShift(pBlobClusArea[i].angle+90);
		B = ryuSinShift(pBlobClusArea[i].angle+90);
		AT = ryuCosShift(pBlobClusArea[i].angle+180);
		BT = ryuSinShift(pBlobClusArea[i].angle+180);

		pBlobClusArea[i].corner[0].x = (pBlobClusArea[i].min_intcpt * BT - pBlobClusArea[i].max_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[0].y = (pBlobClusArea[i].max_ontcpt * A - pBlobClusArea[i].min_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[1].x = (pBlobClusArea[i].max_intcpt * BT - pBlobClusArea[i].max_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[1].y = (pBlobClusArea[i].max_ontcpt * A - pBlobClusArea[i].max_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[2].x = (pBlobClusArea[i].min_intcpt * BT - pBlobClusArea[i].min_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[2].y = (pBlobClusArea[i].min_ontcpt * A - pBlobClusArea[i].min_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[3].x = (pBlobClusArea[i].max_intcpt * BT - pBlobClusArea[i].min_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[3].y = (pBlobClusArea[i].min_ontcpt * A - pBlobClusArea[i].max_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;
	}

	// 根据优先级和元素个数排序
	for(i = 1; i <= clus_cnt; i++) {
		for(j = i + 1; j <= clus_cnt; j++) {
			if(pBlobClusArea[i].flag < pBlobClusArea[j].flag) {
				tmpBlobClusArea = pBlobClusArea[i];
				pBlobClusArea[i] = pBlobClusArea[j];
				pBlobClusArea[j] = tmpBlobClusArea;
			} else if(pBlobClusArea[i].flag == pBlobClusArea[j].flag
				&& pBlobClusArea[i].density < pBlobClusArea[j].density) {
					tmpBlobClusArea = pBlobClusArea[i];
					pBlobClusArea[i] = pBlobClusArea[j];
					pBlobClusArea[j] = tmpBlobClusArea;
			}
		}
	}

#ifdef _DEBUG_LOCATE
	cvCopy(show_img, show_img2);
	printf("共找到%d个有效聚类区域\n", goodclus_cnt);
	for(i = 1; i <= goodclus_cnt; i++) {
		if(0 != pBlobClusArea[i].parent || 0 >= pBlobClusArea[i].linecnt)
			continue;
		printf("聚类区域%d: %d条, %d元素, 最长长度%d, 拟合角度%d, 梯度:%d, 密集度:%d\n", 
			i, pBlobClusArea[i].linecnt, pBlobClusArea[i].element, pBlobClusLine[pBlobClusArea[i].maxlineidx].length, 
			pBlobClusArea[i].angle, pBlobClusArea[i].grad, pBlobClusArea[i].density);
		CvScalar rgb = CV_RGB(rand()%192+63,rand()%192+63,rand()%192+63);
		for(j = 1; j <= line_cnt; j++) {
			if(pBlobClusArea[i].label == pBlobClusLine[j].label) {
				cvLine(show_img2, cvPoint(pBlobClusLine[j].line.pt0.x*4+2, pBlobClusLine[j].line.pt0.y*4+2), 
					cvPoint(pBlobClusLine[j].line.pt1.x*4+2, pBlobClusLine[j].line.pt1.y*4+2), 
					rgb, 1);
			}
		}
		for(j = 0; j < 4; j++) {
			sprintf(txt, "%d", j);
			cvPutText(show_img2, txt, cvPoint(pBlobClusArea[i].corner[j].x/4+2, pBlobClusArea[i].corner[j].y/4), &font, rgb);
			cvCircle(show_img2, cvPoint(pBlobClusArea[i].corner[j].x/4, pBlobClusArea[i].corner[j].y/4), 2, rgb, CV_FILLED);
		}
		sprintf(txt, "%d", i);
		cvPutText(show_img2, txt, cvPoint((pBlobClusArea[i].corner[0].x+pBlobClusArea[i].corner[2].x)/8+2,
			(pBlobClusArea[i].corner[0].y+pBlobClusArea[i].corner[2].y)/8), &font1, rgb);
	}

	cvNamedWindow("Feature Cluster Area");
	cvShowImage("Feature Cluster Area", show_img2);
	cvWaitKey();
#endif

	nRet = goodclus_cnt;

nExit:
	return nRet;
}



