#include "stdafx.h"

#include "RyuCore.h"
#include "WaybillSegmentation.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
#include "OpenCv_debugTools.h"
#endif
#endif

/************************************************************************/
/* 可调节参数													        */
/************************************************************************/
const int gnRectDistMinThresh = 20;
const double gfRectOverlapThreshRatio = 0.5;
const int gnContourDistMinThresh = 30;
const int gnContourConnectedThresh = 100;

const int gnXiaoCheFanGuangHeight = 40;
const int gnXiaoCheFanGuangRangeX = 40;
const int gnXiaoCheFanGuangRangeY = 40;

const double gfPkgDetectTopBoundRatio = 0.15;
const double gfPkgDetectTopCenterRatio = 0.1;
const double gfPkgDetectBottomBoundRatio = 0.15;
const double gfPkgDetectBottomCenterRatio = 0.1;

const int gfCodeInAreaThreshRatio = 0.8;

/************************************************************************/
/* 主函数全局变量                                                        */
/************************************************************************/
int gnFloodFillMatMaxSize = 0, gnFloodFillStatMaxCount = 0;

RyuPoint * gptFloodFillSeeds = 0;

int * gnFloodFillLabelMat = 0;

FFN gtFloodFillStatisticsNode;

FFN * gtFloodFillStatistics = 0;
int gnFloodFillStatCount = 0;

int gnFloodFillHistogram[256];

/************************************************************************/
/* FindContour全局变量                                                   */
/************************************************************************/
int * gnFloodFillContourMat = 0;		// 轮廓标记数组
int * gnFloodFillContourFlag = 0;
RyuPoint * gptFloodFillContourPts = 0;

/************************************************************************/
/* ClassCluster全局变量                                                  */
/************************************************************************/
int * gnFloodFillClassSeeds = 0;
FFC * gtFloodFillClassClusters = 0;

CAN * gtFloodFillCodeAreas = 0;
int gnFloodFillCodeAreaCount = 0;


int WaybillSegment(unsigned char * img, int wid, int hei, RyuRect * bound_box)
{
	int nRet = 0, status = 0;
	int i = 0, j = 0;

	RyuImage * imageIn = 0, * imageProc = 0, *imageProc2 = 0;

	double zoomRatio = 0.0;
	int procWid = 0, procHei = 0;

	unsigned char * pImg = 0;
	int * pMat = 0, * LabelMat = 0, * ContourMat = 0;

	int globalThresh = 54, lowThresh = 0, upThresh = 0;
	int labelCount = 1, pixCountThresh = 1600;

	FFN * FFStatistics = 0;
	int FFStatisticsCount = 0;

	RyuPoint * ContourPts = 0;
	int ptCount = 0, ptTotalCount = 0;

	FFC * FFClassClusters = 0;
	int * ClassSeeds = gnFloodFillClassSeeds;
	int stackPoint = 0, currentIndex = 0, classCount = 0;
	int isCluster = 0, codeinSimilar = 0;

	CAN* FFCodeAreas = 0;
	int FFCodeAreaCount = 0;

	FFN ffnTmp;
	int nTmp1 = 0, nTmp2 = 0;
	int topBound = 0, topCenter = 0, btmBound = 0, btmCenter = 0;

	bound_box->x = bound_box->y = -1;
	bound_box->width = bound_box->height = 0;
	imageIn = ryuCreateImageHeader(ryuSize(wid, hei), RYU_DEPTH_8C, 1);
	imageIn->imagedata = img;

	if(wid >= hei) {
		procWid = 400;
		zoomRatio = procWid * 1.0 / wid;
		procHei = (int)(hei * zoomRatio);
	} else {
		procHei = 400;
		zoomRatio = procHei * 1.0 / hei;
		procWid = (int)(wid * zoomRatio);
	}

	imageProc = ryuCreateImage(ryuSize(procWid, procHei), RYU_DEPTH_8C, 1);
	imageProc2 = ryuCreateImage(ryuSize(procWid, procHei), RYU_DEPTH_8C, 1);

	topBound = procHei * gfPkgDetectTopBoundRatio;
	topCenter = procHei * gfPkgDetectTopCenterRatio;
	btmBound = procHei * (1.0 - gfPkgDetectBottomBoundRatio);
	btmCenter = procHei * (1.0 - gfPkgDetectBottomCenterRatio);

	status = ryuResizeImage(imageIn, imageProc2);

	ryuDilate(imageProc2, imageProc);

#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	IplImage * iplimgProc1 = cvCreateImage(cvSize(procWid, procHei), 8, 1);
	uc2IplImageGray(imageProc->imagedata, iplimgProc1);
	cvNamedWindow("漫水图像1");
	cvShowImage("漫水图像1", iplimgProc1);
	cvReleaseImage(&iplimgProc1);

	IplImage * iplimgProcD = cvCreateImage(cvSize(procWid, procHei), 8, 1);
	uc2IplImageGray(imageProc2->imagedata, iplimgProcD);
	cvNamedWindow("漫水图像D");
	cvShowImage("漫水图像D", iplimgProcD);
	cvReleaseImage(&iplimgProcD);
#endif
#endif

	// 全图做漫水填充操作
	ryuResetFloodFill();
	LabelMat = ryuGetFloodFillLabelMat();
	FFStatistics = ryuGetFloodFillStatistics();
	for(j = 0; j < procHei; j++) {
		pImg = imageProc->imagedata + j * imageProc->widthstep;
		pMat = LabelMat+ j * imageProc->width;
		for(i = 0; i < procWid; i++) {
			if(pImg[i] >= globalThresh && pMat[i] == 0) {
				lowThresh = (int)(0.15 * pImg[i]);
				upThresh = lowThresh;
				status = ryuFloodFill(imageProc, ryuPoint(i, j), globalThresh, lowThresh, upThresh, labelCount++, 4, 0, 0);
				if(status > pixCountThresh) {
					FFStatistics[FFStatisticsCount++] = ryuGetFloodFillStatisticsNode();
					if(FFStatisticsCount >= gnFloodFillStatMaxCount) {
						printf("Warning, risks of overflow found in FloodFill Statistics Array\n");
						break;
					}
				}
			}
		}
	}

	// 找到小于一个漫水区域，则返回
	if(FFStatisticsCount <= 0) {
		nRet = FFStatisticsCount;
		goto nExit;
	}

#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	int * colorPad = (int *)malloc(labelCount * 3 * sizeof(int));
	for(i = 0; i < labelCount; i++) {
		int randTmp = rand()%3;
		colorPad[i*3] = (0 == randTmp) ? rand()%128+128 : rand()%256;
		colorPad[i*3+1] = (1 == randTmp) ? rand()%128+128 : rand()%256;
		colorPad[i*3+2] = (2 == randTmp) ? rand()%128+128 : rand()%256;
	}
	IplImage * iplimgProc2 = cvCreateImage(cvSize(procWid, procHei), 8, 3);
	cvZero(iplimgProc2);
	cvLine(iplimgProc2, cvPoint(0, topBound), cvPoint(procWid, topBound), CV_RGB(192,192,192));
	cvLine(iplimgProc2, cvPoint(0, btmBound), cvPoint(procWid, btmBound), CV_RGB(192,192,192));
	cvLine(iplimgProc2, cvPoint(0, topCenter), cvPoint(procWid, topCenter), CV_RGB(128,128,128));
	cvLine(iplimgProc2, cvPoint(0, btmCenter), cvPoint(procWid, btmCenter), CV_RGB(128,128,128));
	for(j = 0; j < procHei; j++) {
		pImg = (unsigned char *)iplimgProc2->imageData + j * iplimgProc2->widthStep;
		pMat = LabelMat+ j * imageProc->width;
		for(i = 0; i < procWid; i++) {
			if(pMat[i] > 0) {
				pImg[i*3] = colorPad[pMat[i]*3];
				pImg[i*3+1] = colorPad[pMat[i]*3+1];
				pImg[i*3+2] = colorPad[pMat[i]*3+2];
			}
		}
	}
	for(i = 0; i < FFStatisticsCount; i++) {
		int nTmp = FFStatistics[i].label;
		cvRectangle(iplimgProc2, cvPoint(FFStatistics[i].min_x, FFStatistics[i].min_y), 
			cvPoint(FFStatistics[i].max_x, FFStatistics[i].max_y), 
			CV_RGB(colorPad[nTmp*3+2], colorPad[nTmp*3+1], colorPad[nTmp*3]));
	}
	cvNamedWindow("漫水图像2");
	cvShowImage("漫水图像2", iplimgProc2);
	//cvWaitKey();
	cvReleaseImage(&iplimgProc2);
#endif
#endif

// 	// 过滤掉小车金属边缘反光区域
// 	for(i = 0; i < FFStatisticsCount; i++) {
// 		if(FFStatistics[i].height > gnXiaoCheFanGuangHeight)
// 			continue;
// 		if(FFStatistics[i].width < FFStatistics[i].height * 2)
// 			continue;
// 		nTmp1 = (FFStatistics[i].min_x + FFStatistics[i].max_x) / 2;
// 		if(abs(nTmp1 - procWid/2) > gnXiaoCheFanGuangRangeX)
// 			continue;
// 		nTmp2 = (FFStatistics[i].min_y + FFStatistics[i].max_y) / 2;
// 		if(nTmp2 > gnXiaoCheFanGuangRangeY && procHei-nTmp2 < gnXiaoCheFanGuangRangeY)
// 			continue;
// 		// 判定为小车金属边缘反光, 删去该节点
// 		for(j = i+1; j < FFStatisticsCount; j++) {
// 			FFStatistics[j-1] = FFStatistics[j];
// 		}
// 		FFStatisticsCount--;
// 		i--;
// 	}

	// 过滤掉小车上下缘位于两车中间的包裹，及小车上端金属边缘反光区域
	for(i = 0; i < FFStatisticsCount; i++) {
		nTmp1 = (FFStatistics[i].min_y + FFStatistics[i].max_y) >> 1;
		if(FFStatistics[i].max_y < topBound || FFStatistics[i].min_y > btmBound
			|| nTmp1 < topCenter || nTmp1 > btmCenter) {
				for(j = i+1; j < FFStatisticsCount; j++) {
					FFStatistics[j-1] = FFStatistics[j];
				}
				FFStatisticsCount--;
				i--;
		}
	}

	// 找到小于一个有效漫水区域，则返回
	if(FFStatisticsCount <= 0) {
		nRet = FFStatisticsCount;
		goto nExit;
	} else if(1 == FFStatisticsCount) {
		bound_box->x = 1.0 * FFStatistics[0].min_x / zoomRatio;
		bound_box->y = 1.0 * FFStatistics[0].min_y / zoomRatio;
		bound_box->width = 1.0 * FFStatistics[0].width / zoomRatio;
		bound_box->height = 1.0 * FFStatistics[0].height / zoomRatio;
		nRet = 1;
		goto nExit;
	}

	// 将漫水区域与条码识别结果相匹配
	FFCodeAreas = WaybillSegm_getCodeAreas();
	FFCodeAreaCount = WaybillSegm_getCodeAreaCount();
	// 坐标缩放
	for(i = 0; i < FFCodeAreaCount; i++) {
		FFCodeAreas[i].min_x = (int)(FFCodeAreas[i].min_x * zoomRatio);
		FFCodeAreas[i].min_y = (int)(FFCodeAreas[i].min_y * zoomRatio); 
		FFCodeAreas[i].max_x = (int)(FFCodeAreas[i].max_x * zoomRatio); 
		FFCodeAreas[i].max_y = (int)(FFCodeAreas[i].max_y * zoomRatio); 
		FFCodeAreas[i].width = (int)(FFCodeAreas[i].width * zoomRatio); 
		FFCodeAreas[i].height = (int)(FFCodeAreas[i].height * zoomRatio); 
	}
	for(i = 0; i < FFStatisticsCount; i++) {
		for(j = 0; j < FFCodeAreaCount; j++) {
			status = WaybillSegm_rectangleDistance(
				ryuRect(FFStatistics[i].min_x, FFStatistics[i].min_y, FFStatistics[i].width, FFStatistics[i].height),
				ryuRect(FFCodeAreas[j].min_x, FFCodeAreas[j].min_y, FFCodeAreas[j].width, FFCodeAreas[j].height));
			// 条码被漫水区域包含
			if(status <= 0 - FFCodeAreas[j].width * FFCodeAreas[j].height * gfCodeInAreaThreshRatio) {
				FFStatistics[i].code_in |= (1 << j);	// 将条码编号写入漫水区域
				FFStatistics[i].code_cnt++;
			}
		}
	}

	// 找出每个有效漫水区域的外轮廓
	ContourPts = ryuGetFloodFillHoughSeeds();
	for(i = 0; i < FFStatisticsCount; i++) {
		FFStatistics[i].contour_idx = FFStatistics[i].contour_cnt = FFStatistics[i].class_idx = 0;
		ptCount = WaybillSegm_findContour(&FFStatistics[i], procWid, procHei, &ContourPts[ptTotalCount]);
		if(ptCount > 0) {
			FFStatistics[i].contour_idx = ptTotalCount;
			FFStatistics[i].contour_cnt = ptCount;
			ptTotalCount += ptCount;
		}
	}

	ContourMat = ryuGetFloodFillContourMat();

#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	IplImage * iplimgProc3 = cvCreateImage(cvSize(procWid, procHei), 8, 3);
	cvZero(iplimgProc3);
	for(j = 0; j < procHei; j++) {
		pImg = (unsigned char *)iplimgProc3->imageData + j * iplimgProc3->widthStep;
		pMat = ContourMat+ j * imageProc->width;
		for(i = 0; i < procWid; i++) {
			if(pMat[i] > 0) {
				pImg[i*3] = colorPad[pMat[i]*3];
				pImg[i*3+1] = colorPad[pMat[i]*3+1];
				pImg[i*3+2] = colorPad[pMat[i]*3+2];
			}
		}
	}
	cvNamedWindow("漫水图像3");
	cvShowImage("漫水图像3", iplimgProc3);
#endif
#endif

	// 根据漫水区域位置关系及外轮廓距离，聚类漫水区域
	classCount = 0;
	FFClassClusters = ryuGetFloodFillClassClusters();
	for(i = 0; i < FFStatisticsCount; i++) {
		if(FFStatistics[i].class_idx > 0)
			continue;
		FFStatistics[i].class_idx = (++classCount);
		if(classCount >= gnFloodFillStatMaxCount)
			break;
		FFClassClusters[classCount].min_x = FFStatistics[i].min_x;
		FFClassClusters[classCount].max_x = FFStatistics[i].max_x;
		FFClassClusters[classCount].min_y = FFStatistics[i].min_y;
		FFClassClusters[classCount].max_y = FFStatistics[i].max_y;
		FFClassClusters[classCount].ffn_count = 1;

		stackPoint = 1;
		ClassSeeds[1] = i;
		while(stackPoint != 0) {
			// 从堆栈中取出一个节点
			currentIndex = ClassSeeds[stackPoint];
			stackPoint--;
			for(j = 0; j < FFStatisticsCount; j++) {
				if(FFStatistics[j].class_idx > 0 || FFStatistics[j].flag)
					continue;
				// 包含有相同条码，直接判定为同一类簇
				if(FFStatistics[currentIndex].code_in &  FFStatistics[j].code_in) {
					stackPoint++;
					ClassSeeds[stackPoint] = j;
					FFStatistics[j].class_idx = FFStatistics[currentIndex].class_idx;

					FFClassClusters[classCount].min_x = RYUMIN(FFStatistics[j].min_x, FFClassClusters[classCount].min_x);
					FFClassClusters[classCount].max_x = RYUMAX(FFStatistics[j].max_x, FFClassClusters[classCount].max_x);
					FFClassClusters[classCount].min_y = RYUMIN(FFStatistics[j].min_y, FFClassClusters[classCount].min_y);
					FFClassClusters[classCount].max_y = RYUMAX(FFStatistics[j].max_y, FFClassClusters[classCount].max_y);
					FFClassClusters[classCount].ffn_count++;
					continue;
				}
				codeinSimilar = WaybillSegm_codeinSimilarity(&FFStatistics[currentIndex], &FFStatistics[j]);
				// 包含有相同结果的条码，判定为同一类簇
				if(1 == codeinSimilar) {
					stackPoint++;
					ClassSeeds[stackPoint] = j;
					FFStatistics[j].class_idx = FFStatistics[currentIndex].class_idx;

					FFClassClusters[classCount].min_x = RYUMIN(FFStatistics[j].min_x, FFClassClusters[classCount].min_x);
					FFClassClusters[classCount].max_x = RYUMAX(FFStatistics[j].max_x, FFClassClusters[classCount].max_x);
					FFClassClusters[classCount].min_y = RYUMIN(FFStatistics[j].min_y, FFClassClusters[classCount].min_y);
					FFClassClusters[classCount].max_y = RYUMAX(FFStatistics[j].max_y, FFClassClusters[classCount].max_y);
					FFClassClusters[classCount].ffn_count++;
					continue;
				}
				// 包含结果不同且方向不一致的条码，判定存在多包裹，直接返回结果
				if(3 == codeinSimilar) {
					nRet = 3;
					goto nExit;
				// 包含结果不同，但方向一致的条码，判定可能存在多包裹，直接返回结果
				} else if(2 == codeinSimilar) {
					nRet = 2;
					goto nExit;
				}
				status = WaybillSegm_rectangleDistance(
					ryuRect(FFStatistics[currentIndex].min_x, FFStatistics[currentIndex].min_y, 
					FFStatistics[currentIndex].width, FFStatistics[currentIndex].height),
					ryuRect(FFStatistics[j].min_x, FFStatistics[j].min_y, FFStatistics[j].width, FFStatistics[j].height));
				// 相距较远，判定为不同类别
				if(status >= gnRectDistMinThresh)
					continue;
				// 大部分重叠或包含关系，判定为同一类簇，将该节点压入堆栈
				if(status <= 0 - FFStatistics[currentIndex].width * FFStatistics[currentIndex].height * gfRectOverlapThreshRatio
					|| status <= 0 - FFStatistics[j].width * FFStatistics[j].height * gfRectOverlapThreshRatio) {
						stackPoint++;
						ClassSeeds[stackPoint] = j;
						FFStatistics[j].class_idx = FFStatistics[currentIndex].class_idx;

						FFClassClusters[classCount].min_x = RYUMIN(FFStatistics[j].min_x, FFClassClusters[classCount].min_x);
						FFClassClusters[classCount].max_x = RYUMAX(FFStatistics[j].max_x, FFClassClusters[classCount].max_x);
						FFClassClusters[classCount].min_y = RYUMIN(FFStatistics[j].min_y, FFClassClusters[classCount].min_y);
						FFClassClusters[classCount].max_y = RYUMAX(FFStatistics[j].max_y, FFClassClusters[classCount].max_y);
						FFClassClusters[classCount].ffn_count++;
						continue;
				}

				// 相距较近或有小部分重叠，进一步判定轮廓距离
				status = WaybillSegm_contourDistance(ContourPts+FFStatistics[currentIndex].contour_idx, 
					FFStatistics[currentIndex].contour_cnt, ContourPts+FFStatistics[j].contour_idx, 
					FFStatistics[j].contour_cnt, gnContourDistMinThresh);

				// 轮廓贯通，判定为同一类簇，将该节点压入堆栈
				if(status > gnContourConnectedThresh) {
					stackPoint++;
					ClassSeeds[stackPoint] = j;
					FFStatistics[j].class_idx = FFStatistics[currentIndex].class_idx;
					FFClassClusters[classCount].min_x = RYUMIN(FFStatistics[j].min_x, FFClassClusters[classCount].min_x);
					FFClassClusters[classCount].max_x = RYUMAX(FFStatistics[j].max_x, FFClassClusters[classCount].max_x);
					FFClassClusters[classCount].min_y = RYUMIN(FFStatistics[j].min_y, FFClassClusters[classCount].min_y);
					FFClassClusters[classCount].max_y = RYUMAX(FFStatistics[j].max_y, FFClassClusters[classCount].max_y);
					FFClassClusters[classCount].ffn_count++;
					continue;
				}
			}
		}
	}

#ifdef	_DEBUG_
#ifdef  _DEBUG_FLOODFILL
	for(j = 1; j <= classCount; j++) {
		if(FFClassClusters[j].ffn_count > 0) {
			FFClassClusters[j].width = FFClassClusters[j].max_x - FFClassClusters[j].min_x + 1;
			FFClassClusters[j].height = FFClassClusters[j].max_y - FFClassClusters[j].min_y + 1;
			cvRectangle(iplimgProc3, cvPoint(FFClassClusters[j].min_x, FFClassClusters[j].min_y), 
				cvPoint(FFClassClusters[j].max_x, FFClassClusters[j].max_y), CV_RGB(255, 0, 0), 2);
		}
	}
	cvNamedWindow("漫水图像3");
	cvShowImage("漫水图像3", iplimgProc3);
	cvReleaseImage(&iplimgProc3);
	//cvWaitKey();
#endif
#endif

	if(classCount > 1) {
		nRet = 2;
		goto nExit;
	} else if(1 == classCount) {
		bound_box->x = 1.0 * FFClassClusters[1].min_x / zoomRatio;
		bound_box->y = 1.0 * FFClassClusters[1].min_y / zoomRatio;
		bound_box->width = 1.0 * FFClassClusters[1].width / zoomRatio;
		bound_box->height = 1.0 * FFClassClusters[1].height / zoomRatio;
	}

	nRet = classCount;

nExit:
	if(imageIn)
		ryuReleaseImageHeader(&imageIn);
	if(imageProc)
		ryuReleaseImage(&imageProc);
	if(imageProc2)
		ryuReleaseImage(&imageProc2);

	return nRet;
}


int ryuInitFloodFill(RyuSize sz, int max_count)
{
	if(sz.width <= 0 || sz.height <= 0 || max_count <= 0) {
		return -1;
	}

	gnFloodFillMatMaxSize = sz.width * sz.height;
	gnFloodFillStatMaxCount = max_count;

	gnFloodFillLabelMat = (int *)malloc(gnFloodFillMatMaxSize * sizeof(int));
	if(!gnFloodFillLabelMat) {
		return -2;
	}

	gptFloodFillSeeds = (RyuPoint *)malloc(gnFloodFillMatMaxSize * sizeof(RyuPoint));
	if(!gptFloodFillSeeds) {
		return -3;
	}

	gtFloodFillStatistics = (FFN *)malloc(gnFloodFillStatMaxCount * sizeof(FFN));
	if(!gtFloodFillStatistics) {
		return -4;
	}

	gnFloodFillContourMat = (int *)malloc(gnFloodFillMatMaxSize * sizeof(int));
	if(!gnFloodFillContourMat) {
		return -5;
	}

	gnFloodFillContourFlag = (int *)malloc(sz.width * sizeof(int));
	if(!gnFloodFillContourFlag) {
		return -6;
	}

	gnFloodFillClassSeeds = (int *)malloc(gnFloodFillStatMaxCount * sizeof(int));
	if(!gnFloodFillClassSeeds) {
		return -7;
	}

	gtFloodFillClassClusters = (FFC *)malloc(gnFloodFillStatMaxCount * sizeof(FFC));
	if(!gtFloodFillClassClusters) {
		return -8;
	}

	gtFloodFillCodeAreas = (CAN *)malloc(32 * sizeof(CAN));
	if(!gtFloodFillCodeAreas) {
		return -9;
	}

	gptFloodFillContourPts = gptFloodFillSeeds;

	return 1;
}

int ryuReleaseFloodFill()
{
	if(gnFloodFillLabelMat) {
		free(gnFloodFillLabelMat);
		gnFloodFillLabelMat = 0;
	}

	if(gptFloodFillSeeds) {
		free(gptFloodFillSeeds);
		gptFloodFillSeeds = 0;
	}

	if(gtFloodFillStatistics) {
		free(gtFloodFillStatistics);
		gtFloodFillStatistics = 0;
	}

	if(gnFloodFillContourMat) {
		free(gnFloodFillContourMat);
		gnFloodFillContourMat = 0;
	}

	if(gnFloodFillContourFlag) {
		free(gnFloodFillContourFlag);
		gnFloodFillContourFlag = 0;
	}

	if(gnFloodFillClassSeeds) {
		free(gnFloodFillClassSeeds);
		gnFloodFillClassSeeds = 0;
	}

	if(gtFloodFillClassClusters) {
		free(gtFloodFillClassClusters);
		gtFloodFillClassClusters = 0;
	}

	if(gtFloodFillCodeAreas) {
		free(gtFloodFillCodeAreas);
		gtFloodFillCodeAreas = 0;
	}
	return 1;
}

// 核心功能模块
int ryuFloodFill(RyuImage * img_in, RyuPoint seed, int gl_thre, int lo_diff, int up_diff, 
		int label_val, int conn_flag, int stat_flag, int hist_flag)
{
	// 当前像素位置
	RyuPoint * Seeds = gptFloodFillSeeds;
	int * LabelMat = gnFloodFillLabelMat;
	int * Hist = gnFloodFillHistogram;

	RyuPoint CurrentPixel;
	int StackPoint = 1;
	int PixelVal = 0, PixelCnt = 0;
	int LowThresh = 0, UpThresh = 0;

	int offset = 0;
	int * pMat = 0;
	unsigned char * pImg = 0;

	// 初始化种子
	Seeds[1] = seed;

	// 初始化统计节点
	gtFloodFillStatisticsNode.label = label_val;
	gtFloodFillStatisticsNode.min_x = gtFloodFillStatisticsNode.max_x = seed.x;
	gtFloodFillStatisticsNode.min_y = gtFloodFillStatisticsNode.max_y = seed.y;
	gtFloodFillStatisticsNode.pixel_count = gtFloodFillStatisticsNode.width 
		= gtFloodFillStatisticsNode.height = gtFloodFillStatisticsNode.flag = 0;
	gtFloodFillStatisticsNode.min_v = 255;
	gtFloodFillStatisticsNode.max_v = 0;
	gtFloodFillStatisticsNode.contour_idx = gtFloodFillStatisticsNode.contour_cnt = 0;
	gtFloodFillStatisticsNode.class_idx = 0;
	gtFloodFillStatisticsNode.code_in = gtFloodFillStatisticsNode.code_cnt = 0;
	gtFloodFillStatisticsNode.flag = 0;

	// 初始化直方图
	if(hist_flag) {
		memset(Hist, 0, 256 * sizeof(int));
	}

	while(StackPoint != 0) {
		// 取出种子
		CurrentPixel = Seeds[StackPoint];
		StackPoint--;

		offset = CurrentPixel.y * img_in->width + CurrentPixel.x;
		pImg = img_in->imagedata + offset;
		pMat = LabelMat + offset;

		//将当前点（继承种子点）标定，更新阈值
		LowThresh = *pImg - lo_diff;
		UpThresh = *pImg + up_diff;
		*pMat = label_val;
		PixelCnt++;

		gtFloodFillStatisticsNode.min_x = RYUMIN(gtFloodFillStatisticsNode.min_x, CurrentPixel.x);
		gtFloodFillStatisticsNode.max_x = RYUMAX(gtFloodFillStatisticsNode.max_x, CurrentPixel.x);
		gtFloodFillStatisticsNode.min_y = RYUMIN(gtFloodFillStatisticsNode.min_y, CurrentPixel.y);
		gtFloodFillStatisticsNode.max_y = RYUMAX(gtFloodFillStatisticsNode.max_y, CurrentPixel.y);
		gtFloodFillStatisticsNode.min_v = RYUMIN(gtFloodFillStatisticsNode.min_v, *pImg);
		gtFloodFillStatisticsNode.max_v = RYUMAX(gtFloodFillStatisticsNode.max_v, *pImg);

		if(hist_flag) {
			Hist[*pImg]++;
		}

		//判断左面的点，如果满足漫水条件，则压入堆栈
		//注意防止越界和重复记录
		if(CurrentPixel.x > 1 && *(pMat-1) == 0) {
			//取得当前指针处的像素值，判断漫水条件
			PixelVal = *(pImg - 1);
			if (PixelVal >= gl_thre && PixelVal >= LowThresh && PixelVal <= UpThresh) {
				StackPoint++;
				Seeds[StackPoint].y = CurrentPixel.y;
				Seeds[StackPoint].x = CurrentPixel.x - 1;
			}
		}

		//判断上面的点，如果满足漫水条件，则压入堆栈
		//注意防止越界和重复记录
		if(CurrentPixel.y > 1 && *(pMat-img_in->width) == 0) {
			//取得当前指针处的像素值，判断漫水条件
			PixelVal = *(pImg - img_in->width);
			if (PixelVal >= gl_thre && PixelVal >= LowThresh && PixelVal <= UpThresh) {
				StackPoint++;
				Seeds[StackPoint].y = CurrentPixel.y - 1;
				Seeds[StackPoint].x = CurrentPixel.x;
			}
		}

		//判断右面的点，如果满足漫水条件，则压入堆栈
		//注意防止越界和重复记录
		if(CurrentPixel.x < img_in->width-1 && *(pMat+1) == 0) {
			//取得当前指针处的像素值，判断漫水条件
			PixelVal = *(pImg + 1);
			if (PixelVal >= gl_thre && PixelVal >= LowThresh && PixelVal <= UpThresh) {
				StackPoint++;
				Seeds[StackPoint].y = CurrentPixel.y;
				Seeds[StackPoint].x = CurrentPixel.x + 1;
			}
		}

		//判断下面的点，如果满足漫水条件，则压入堆栈
		//注意防止越界和重复记录
		if(CurrentPixel.y < img_in->height-1 && *(pMat+img_in->width) == 0) {
			//取得当前指针处的像素值，判断漫水条件
			PixelVal = *(pImg + img_in->width);
			if (PixelVal >= gl_thre && PixelVal >= LowThresh && PixelVal <= UpThresh) {
				StackPoint++;
				Seeds[StackPoint].y = CurrentPixel.y + 1;
				Seeds[StackPoint].x = CurrentPixel.x;
			}
		}
	}

	gtFloodFillStatisticsNode.pixel_count = PixelCnt;
	gtFloodFillStatisticsNode.width = gtFloodFillStatisticsNode.max_x 
		- gtFloodFillStatisticsNode.min_x + 1;
	gtFloodFillStatisticsNode.height = gtFloodFillStatisticsNode.max_y 
		- gtFloodFillStatisticsNode.min_y + 1;

	if(stat_flag) {
		if(gnFloodFillStatCount >= gnFloodFillStatMaxCount)
			return -8;
		gtFloodFillStatistics[gnFloodFillStatCount++] = gtFloodFillStatisticsNode;
	}

	return PixelCnt;
}

void ryuResetFloodFill()
{
	memset(gnFloodFillLabelMat, 0, gnFloodFillMatMaxSize * sizeof(int));
	memset(gnFloodFillContourMat, 0, gnFloodFillMatMaxSize * sizeof(int));
	gnFloodFillStatCount = 0;
}
	
int * ryuGetFloodFillLabelMat()
{
	return gnFloodFillLabelMat;
}

int * ryuGetFloodFillHistogram()
{
	return gnFloodFillHistogram;
}

FFN ryuGetFloodFillStatisticsNode()
{
	return gtFloodFillStatisticsNode;
}

FFN * ryuGetFloodFillStatistics()
{
	return gtFloodFillStatistics;
}

int ryuGetFloodFillStatCount()
{
	return gnFloodFillStatCount;
}

int * ryuGetFloodFillContourMat()
{
	return gnFloodFillContourMat;
}

FFC * ryuGetFloodFillClassClusters()
{
	return gtFloodFillClassClusters;
}

RyuPoint * ryuGetFloodFillHoughSeeds()
{
	return gptFloodFillContourPts;
}


int WaybillSegm_findContour(FFN * StatisticsNode, int wid, int hei, RyuPoint * ContourPts)
{
	FFN * ffn = StatisticsNode;
	int * Label = gnFloodFillLabelMat;
	int * Contour = gnFloodFillContourMat;
	int * Flags = gnFloodFillContourFlag;

	int i = 0, j = 0;
	int offset = 0, val = ffn->label, fcount = 0;

	int * pLabel = 0, * pContour = 0;

	int ptCount = 0;

	// 此处应有输入参数有效性检验

	// 1-左侧遍历轮廓点
	offset = ffn->min_y * wid + ffn->min_x;
	pLabel = Label + offset;
	pContour = Contour + offset;
	for(j = 0; j < ffn->height; j++) {
		for(i = 0; i < ffn->width; i++) {
			if(pLabel[i] == val && pContour[i] != val) { 
				pContour[i] = val;
				ContourPts[ptCount++] = ryuPoint(i+ffn->min_x, j+ffn->min_y);
				break;
			}
		}
		pLabel += wid;
		pContour += wid;
	}

	// 2-右侧遍历轮廓点
	offset = ffn->min_y * wid + ffn->min_x;
	pLabel = Label + offset;
	pContour = Contour + offset;
	for(j = 0; j < ffn->height; j++) {
		for(i = ffn->width-1; i >= 0; i--) {
			if(pLabel[i] == val && pContour[i] != val) { 
				pContour[i] = val;
				ContourPts[ptCount++] = ryuPoint(i+ffn->min_x, j+ffn->min_y);
				break;
			}
		}
		pLabel += wid;
		pContour += wid;
	}

	// 3-上侧遍历轮廓点
	fcount = ffn->width;
	memset(Flags, 0, ffn->width * sizeof(int));
	offset = ffn->min_y * wid + ffn->min_x;
	pLabel = Label + offset;
	pContour = Contour + offset;
	for(j = 0; j < ffn->height; j++) {
		if(fcount <= 0)
			break;
		for(i = 0; i < ffn->width; i++) {
			if(pLabel[i] == val && pContour[i] != val && Flags[i] == 0) {
				pContour[i] = val;
				ContourPts[ptCount++] = ryuPoint(i+ffn->min_x, j+ffn->min_y);
				Flags[i] = 1;
				fcount--;
			}
		}
		pLabel += wid;
		pContour += wid;
	}

	// 3-下侧遍历轮廓点
	fcount = ffn->width;
	memset(Flags, 0, ffn->width * sizeof(int));
	offset = ffn->max_y * wid + ffn->min_x;
	pLabel = Label + offset;
	pContour = Contour + offset;
	for(j = ffn->height-1; j >= 0; j--) {
		if(fcount <= 0)
			break;
		for(i = 0; i < ffn->width; i++) {
			if(pLabel[i] == val && pContour[i] != val && Flags[i] == 0) {
				pContour[i] = val;
				ContourPts[ptCount++] = ryuPoint(i+ffn->min_x, j+ffn->min_y);
				Flags[i] = 1;
				fcount--;
			}
		}
		pLabel -= wid;
		pContour -= wid;
	}

	return ptCount;
}


int WaybillSegm_rectangleFitting(RyuPoint * points, int pt_count, int wid, int hei, FFN * StatisticsNode)
{
	return 1;
}

int WaybillSegm_rectangleDistance(RyuRect rc1, RyuRect rc2)
{
	int flagX = 0, flagY = 0;
	int overlapX = 0, overlapY = 0;
	int dist = 0;

	if(rc1.x >= rc2.x && rc1.x <= rc2.x+rc2.width) {
		if(rc1.x+rc1.width <= rc2.x+rc2.width) 
			overlapX = rc1.width;
		else 
			overlapX = rc2.x + rc2.width - rc1.x;
		flagX = 1;
	}
	else if(rc2.x >= rc1.x && rc2.x <= rc1.x+rc1.width) {
		if(rc2.x+rc2.width <= rc1.x+rc1.width)
			overlapX = rc2.width;
		else
			overlapX = rc1.x + rc1.width - rc2.x;	
		flagX = 1;
	}

	if(rc1.y >= rc2.y && rc1.y <= rc2.y+rc2.height) {
		if(rc1.y+rc1.height <= rc2.y+rc2.height)
			overlapY = rc1.height;
		else
			overlapY = rc2.y + rc2.height - rc1.y;
		flagY = 1;	
	}
	else if(rc2.y >= rc1.y && rc2.y <= rc1.y+rc1.height) {
		if(rc2.y+rc2.height <= rc1.y+rc1.height)
			overlapY = rc2.height;
		else
			overlapY = rc1.y + rc1.height - rc2.y;	
		flagY = 1;	
	}

	if(flagX > 0 && flagY > 0) {
		dist = 0 - overlapX * overlapY;
	} else if(flagX > 0) {
		dist = RYUMIN(abs(rc1.y+rc1.height-rc2.y), abs(rc2.y+rc2.height-rc1.y));
	} else if(flagY > 0) {
		dist = RYUMIN(abs(rc1.x+rc1.width-rc2.x), abs(rc2.x+rc2.width-rc1.x));
	} else {
		overlapX = abs(rc1.x+rc1.width-rc2.x) + abs(rc1.y+rc1.height-rc2.y);
		overlapY = abs(rc2.x+rc2.width-rc1.x) + abs(rc2.y+rc2.height-rc1.y);
		dist = RYUMIN(overlapX, overlapY);
	}

	return dist;
}

int WaybillSegm_contourDistance(RyuPoint * contour1, int count1, RyuPoint * contour2, int count2, int thresh)
{
	int i = 0, j = 0;
	int dist = 0, count = 0;

	for(i = 0; i < count1; i++) {
		for(j = 0; j < count2; j++) {
			dist = abs(contour1[i].x - contour2[j].x) + abs(contour1[i].y - contour2[j].y);
			if(dist < thresh) {
				count++;
				break;
			}
		}
	}

	return count;
}

void WaybillSegm_resetCodeAreaStack()
{
	gnFloodFillCodeAreaCount = 0;
	return;
}

CAN * WaybillSegm_getCodeAreas()
{
	return gtFloodFillCodeAreas;
}

int WaybillSegm_getCodeAreaCount()
{
	return gnFloodFillCodeAreaCount;
}

int WaybillSegm_pushCodeAreaStack(RyuPoint * corner, int angle, int flag, char * str_code)
{
	CAN * FFCodeAreas = gtFloodFillCodeAreas;
	int nTmp1 = 0, nTmp2 = 0;

	if(gnFloodFillCodeAreaCount >= 32)
		return 0;

	FFCodeAreas[gnFloodFillCodeAreaCount].angle = angle;
	FFCodeAreas[gnFloodFillCodeAreaCount].flag = flag;
	if(1 == flag) {
		memcpy(FFCodeAreas[gnFloodFillCodeAreaCount].str_code, str_code, CODE_RESULT_ARR_LENGTH);
	} else {
		memset(FFCodeAreas[gnFloodFillCodeAreaCount].str_code, 0, CODE_RESULT_ARR_LENGTH);
	}

	nTmp1 = RYUMIN(corner[0].x, corner[1].x);
	nTmp2 = RYUMIN(corner[2].x, corner[3].x);
	FFCodeAreas[gnFloodFillCodeAreaCount].min_x = RYUMIN(nTmp1, nTmp2);

	nTmp1 = RYUMAX(corner[0].x, corner[1].x);
	nTmp2 = RYUMAX(corner[2].x, corner[3].x);
	FFCodeAreas[gnFloodFillCodeAreaCount].max_x = RYUMAX(nTmp1, nTmp2);

	nTmp1 = RYUMIN(corner[0].y, corner[1].y);
	nTmp2 = RYUMIN(corner[2].y, corner[3].y);
	FFCodeAreas[gnFloodFillCodeAreaCount].min_y = RYUMIN(nTmp1, nTmp2);

	nTmp1 = RYUMAX(corner[0].y, corner[1].y);
	nTmp2 = RYUMAX(corner[2].y, corner[3].y);
	FFCodeAreas[gnFloodFillCodeAreaCount].max_y = RYUMAX(nTmp1, nTmp2);

	FFCodeAreas[gnFloodFillCodeAreaCount].width = FFCodeAreas[gnFloodFillCodeAreaCount].max_x 
		- FFCodeAreas[gnFloodFillCodeAreaCount].min_x + 1;
	FFCodeAreas[gnFloodFillCodeAreaCount].height = FFCodeAreas[gnFloodFillCodeAreaCount].max_y 
		- FFCodeAreas[gnFloodFillCodeAreaCount].min_y + 1;

	gnFloodFillCodeAreaCount++;

	return 1;
}

/************************************************************************/
/* 区域包含条码相似性检测													*/
/************************************************************************/
int WaybillSegm_codeinSimilarity(FFN * ffn1, FFN * ffn2)
{
	int i = 0, j = 0;
	int IndexArr1[32] =  {0}, IndexArr2[32] = {0};
	int pos1 = 0, pos2 = 0;
	int status = 0;

	CAN * FFCodeAreas = gtFloodFillCodeAreas;

	if(0 == ffn1->code_cnt && 0 == ffn2->code_cnt)
		return 5;

	if(0 == ffn1->code_cnt || 0 == ffn2->code_cnt) 
		return 4;

	for(i = 0; i < 32; i++) {
		if(ffn1->code_in & (1 << i))
			IndexArr1[pos1++] = i;
		if(ffn2->code_in & (1 << i))
			IndexArr2[pos2++] = i;
	}

	if(pos1 != ffn1->code_cnt || pos2 != ffn2->code_cnt) 
		return -1;

	for(i = 0; i < ffn1->code_cnt; i++) {
		if(IndexArr1[i] >= gnFloodFillCodeAreaCount)
			return -2;
		for(j = 0; j < ffn2->code_cnt; j++) {
			if(IndexArr2[j] >= gnFloodFillCodeAreaCount)
				return -2;
			status = strcmp(FFCodeAreas[IndexArr1[i]].str_code, FFCodeAreas[IndexArr2[j]].str_code);
			// 结果相同
			if(0 == status)
				return 1;
			status = ryuCycleDistance(FFCodeAreas[IndexArr1[i]].angle, FFCodeAreas[IndexArr2[j]].angle, 180);
			// 方向一致
			if(status <= 6)
				return 2;
		}
	}

	return 3;
}