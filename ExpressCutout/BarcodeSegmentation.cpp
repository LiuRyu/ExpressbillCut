#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BarcodeSegmentation.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#include "OpenCv_debugTools.h"
#endif
#endif

#ifdef	_PRINT_TIME
//#define _PRINT_TIME_SEGMENT
#ifdef	_PRINT_TIME_SEGMENT
#include "OpenCv_debugTools.h"
#include <time.h>
long TimeCost = 0;
LARGE_INTEGER m_frequency = {0}, m_time1 = {0}, m_time2 = {0};
#endif
#endif

// 文件内宏定义
#define SEGMENT_USE_5X5KERNAL	// 高斯平滑核大小

#define PARALLEL_CAST_THRESHOLD_BASIC	(38)	// 条码方向梯度特征点累计阈值1
#define PARALLEL_CAST_THRESHOLD_RATIO	(0.6)	// 条码方向梯度特征点累计阈值2

#define BARCODE_ONTERCEPT_EXTEND		(5)		// 条形码区域基本扩展宽度1
#define BARCODE_INTERCEPT_EXTEND		(3)		// 条形码区域基本扩展宽度2

#define SEGMENT_ZOOM_WIDTH_THRESH		(300)	// 放大标志判断阈值1
#define SEGMENT_ZOOM_RATIO_THRESH		(0.6F)	// 放大标志判断阈值2

#define SEGMENT_EXTEND_WIDTH_RATIO		(0.1F)	// 条形码区域额外扩展宽度比率

#define SEGMENT_ANGLE_SCAN_RADIUS		(18)	// 霍夫直线检测角度半径

const int gnSgmInterceptMaxCount = 8;

static int gnSgmImgMaxWidth = 0, gnSgmImgMaxHeight = 0;

static unsigned char * gucSgmParallelMask = 0, * gucSgmVerticalMask = 0;

static RyuPoint * gpptSgmImgEffcRowRanges = 0;

static int gnSgmHoughSeqMaxSize = 0;
static RyuPoint * gpptSgmVerticalSeq = 0;
static int gnSgmVerticalSeqCnt = 0;
static RyuPoint * gpptSgmParallelSeq = 0;
static int gnSgmParallelSeqCnt = 0;

static int * gnSgmHoughAccum = 0;
//const  int gnSgmNumangle = 19;
const  int gnSgmNumangle = SEGMENT_ANGLE_SCAN_RADIUS * 2 + 1;
const  int gnSgmNumangleH = SEGMENT_ANGLE_SCAN_RADIUS;
static int gnSgmMaxNumrho = 0;

static int * gpnSgmCastAccumVerti = 0;
static int * gpnSgmCastAccumParal = 0;

static int * gpnSgmMedianArray1 = 0, * gpnSgmMedianArray2 = 0;

SegmentBarcodeArea * gstSegmentBarcodeArea = 0;

static int gnSgmInitFlag = 0;

int FindRectangleEffectiveRange( RyuPoint * corner, int * LTRB, RyuPoint * row_ranges );

int RectRangeAdaptImageFigure( int * LTRB, RyuPoint * row_ranges, int width, int height );

int GetSpecifiedGradientMasks( unsigned char * in_data, int width, int height, int * LTRB, RyuPoint * row_ranges, 
		int apprx_angle, int ref_grad, unsigned char * parallel_mask, unsigned char * vertical_mask );

int CorrectAngleViaStandardHoughTrans( RyuPoint * hough_seq, int seq_count, int * LTRB, 
		int apprx_angle, int * exact_angle );

void UpdateAreaCornerIntercept( RyuPoint * corner, RyuPoint * intercept, RyuPoint * ontercept, 
		int correct_angle );

int CastPointsAlongSpecifiedOrientation(RyuPoint * pnt_seq, int seq_count, int * LTRB, int angle, 
	int * cast_array1, int * cast_array2);

int PressUponBarcodeIntercept( int * cast_array, int * LTRB, RyuPoint * intercept, int * param );

int PressUponBarcodeOntercept( int * cast_array, int * LTRB, int reference, RyuPoint * ontercept, int * param );

int GetBarcodePureAreaFigures( RyuPoint * intercept, int * param1, int count1, 
	RyuPoint * ontercept, int * param2, int count2, int * LTRB, int angle, 
	RyuPoint main_intrcpt, RyuPoint main_ontrcpt, SegmentBarcodeArea * barcode_area, int barcode_area_idx);

// 全局调试变量
#ifdef _DEBUG_
#ifdef _DEBUG_SEGMENT
IplImage * giplSrcSlice = 0;
IplImage * giplSrcSlice3C = 0;
IplImage * giplCurve = 0;
CvFont cvSgmFont;
char cvSgmTxt[50];
void debugDrawLineAcrossImage(IplImage * img3C, int angle, CvScalar rgb);
#endif
#endif


SegmentBarcodeArea * GetSegmentBarcodeAreaPtr()
{
	return gstSegmentBarcodeArea;
}

int SegmentBarcode( unsigned char * in_data, int width, int height, 
		RyuPoint * corner, RyuPoint * intercept, RyuPoint * ontercept, int * angle, int ref_grad, 
		SegmentBarcodeArea * barcode_area, int barcode_area_idx)
{
	int nRet = 0, status = 0;
	int i = 0;

	int LTRB[4] = {0};

	int apprx_angle = 0, exact_angle = 0;

	int line_thresh = abs( ontercept->x-ontercept->y ) / 6;

	RyuPoint barcodeIntercept[gnSgmInterceptMaxCount];
	RyuPoint barcodeOntercept[gnSgmInterceptMaxCount];
	int intercept_cnt = 0, ontercept_cnt = 0;
	int reference = 0;
	int param1[gnSgmInterceptMaxCount] = {0};
	int param2[gnSgmInterceptMaxCount] = {0};

	if( 0 == in_data || width <= 0 || height <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf( "ERROR! Invalid input of SegmentBarcode, in_data=0x%x, width=%d, height=%d\n", 
			in_data, width, height );
#endif
		nRet = -1;
		goto nExit;
	}

	if(0 == corner || 0 == intercept || 0 == ontercept || 0 == angle) {
#ifdef	_PRINT_PROMPT
		printf( "ERROR! Invalid input of SegmentBarcode, corner=0x%x, intercept=0x%x, \
			ontercept=0x%x, angle=0x%x\n", corner, intercept, ontercept, angle );
#endif
		nRet = -1;
		goto nExit;
	}

	if( *angle < 0 || *angle >= 180 || ref_grad < 0 ) {
#ifdef	_PRINT_PROMPT
		printf( "ERROR! Bad input of SegmentBarcode, *angle=%d, ref_grad=%d\n", 
			*angle, ref_grad );
#endif
		nRet = -1;
		goto nExit;
	}

	apprx_angle = exact_angle = *angle;

	// 计算有效检测区域，corner, intercept, ontercept一直都是绝对坐标
	status = FindRectangleEffectiveRange( corner, LTRB, gpptSgmImgEffcRowRanges );
	if( status <= 0 ) {
		nRet = 0;
		goto nExit;
	}

	// 有效检测区域适应图像参数，过程执行完毕后LTRB调整为图像范围，gpptSgmImgEffcRowRanges变为LTRB内相对坐标
	status = RectRangeAdaptImageFigure( LTRB, gpptSgmImgEffcRowRanges, width, height );
	if( status <= 0 ) {
		nRet = 0;
		goto nExit;
	}

#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_SEGMENT
	TimeCost = 0;
	QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_time2);
#endif
#endif
	// 获取梯度方向特征点，记录特征点坐标为LTRB内相对坐标
	status = GetSpecifiedGradientMasks( in_data, width, height, LTRB, gpptSgmImgEffcRowRanges, 
				apprx_angle, ref_grad, gucSgmParallelMask, gucSgmVerticalMask );
	if( status <= 0 ) {
		nRet = 0;
		goto nExit;
	}
#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_SEGMENT
	QueryPerformanceCounter((LARGE_INTEGER*) &m_time1);
	TimeCost = 1000.0*1000.0*(m_time1.QuadPart-m_time2.QuadPart)/m_frequency.QuadPart;
	printf("\n-GetSpecifiedGradientMasks 耗时: %ldus\n", TimeCost);
#endif
#endif

#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_SEGMENT
	TimeCost = 0;
	QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_time2);
#endif
#endif
	// 矫正条码倾斜角度
	// Ryu注：此函数检测精确角度的方法不可靠，只是取巧
	status = CorrectAngleViaStandardHoughTrans( gpptSgmVerticalSeq, gnSgmVerticalSeqCnt, LTRB, apprx_angle, &exact_angle );
	if(status < 0) {
		nRet = 0;
		goto nExit;
	} 
	else if(status > 0) {
		// 更新条码区域参数
		// 根据exact_angle重算corner, intercept, ontercept
		UpdateAreaCornerIntercept( corner, intercept, ontercept, exact_angle );
	}
#ifdef	_PRINT_TIME
#ifdef  _PRINT_TIME_SEGMENT
	QueryPerformanceCounter((LARGE_INTEGER*) &m_time1);
	TimeCost = 1000.0*1000.0*(m_time1.QuadPart-m_time2.QuadPart)/m_frequency.QuadPart;
	printf("\n-CorrectAngleViaStandardHoughTrans 耗时: %ldus\n", TimeCost);
#endif
#endif

	status = CastPointsAlongSpecifiedOrientation( gpptSgmParallelSeq, gnSgmParallelSeqCnt, LTRB, exact_angle,
				gpnSgmCastAccumParal, gpnSgmCastAccumVerti );
	if(status <= 0) {
		nRet = 0;
		goto nExit;
	}

	// 定位条码上下界
	status = PressUponBarcodeIntercept( gpnSgmCastAccumParal, LTRB, barcodeIntercept, param1 );
	if(status <= 0) {
		nRet = 0;
		goto nExit;
	}
	intercept_cnt = status;

	for( i = 0; i < intercept_cnt; i++ ) {
		reference += (barcodeIntercept[i].y-barcodeIntercept[i].x+1);
	}

	// 定位条码左右界
	status = PressUponBarcodeOntercept( gpnSgmCastAccumVerti, LTRB, reference, barcodeOntercept, param2 );
	if(status <= 0) {
		nRet = 0;
		goto nExit;
	}
	ontercept_cnt = status;

	status = GetBarcodePureAreaFigures( barcodeIntercept, param1, intercept_cnt, 
		barcodeOntercept, param2, ontercept_cnt, LTRB, exact_angle, *intercept, *ontercept, barcode_area, barcode_area_idx);

	*angle = exact_angle;
	nRet = status;

#ifdef _DEBUG_
#ifdef _DEBUG_SEGMENT
	if( giplSrcSlice ) cvReleaseImage( &giplSrcSlice );
	if( giplSrcSlice3C ) cvReleaseImage( &giplSrcSlice3C );
	if( giplCurve ) cvReleaseImage( &giplCurve );
#endif
#endif

nExit:
	return nRet;
}

void UpdateCodeCorner( SegmentBarcodeArea * codeArea, int leftOffset, int rightOffset )
{
	int A = 0, B = 0, AT = 0, BT = 0;
	int tmpMaxAB = 0;
	int dist = 0;
	float ratio = 0.0F;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	A = ryuCosShift( codeArea->angle + 90 );
	B = ryuSinShift( codeArea->angle + 90 );
	AT = ryuCosShift( codeArea->angle + 180 );
	BT = ryuSinShift( codeArea->angle + 180 );
	tmpMaxAB = RYUMAX( abs(A), abs(B) );
	ratio = tmpMaxAB * 1.0 / (1 << TRIGONOMETRIC_SHIFT_DIGIT);

	leftOffset = (int)(leftOffset * ratio);
	rightOffset = (int)(rightOffset * ratio);

	codeArea->max_ontcpt -= leftOffset;
	codeArea->min_ontcpt += rightOffset;
	
	codeArea->corner[0].x = (codeArea->min_intcpt * BT - codeArea->max_ontcpt * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[0].y = (codeArea->max_ontcpt * A - codeArea->min_intcpt * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[1].x = (codeArea->max_intcpt * BT - codeArea->max_ontcpt * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[1].y = (codeArea->max_ontcpt * A - codeArea->max_intcpt * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[2].x = (codeArea->min_intcpt * BT - codeArea->min_ontcpt * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[2].y = (codeArea->min_ontcpt * A - codeArea->min_intcpt * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[3].x = (codeArea->max_intcpt * BT - codeArea->min_ontcpt * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	codeArea->corner[3].y = (codeArea->min_ontcpt * A - codeArea->max_intcpt * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;

}


int BarcodeSegmentation_init(int max_width, int max_height, int seq_size)
{
	int nRet = 0;

	if(max_width <= 0 || max_height <= 0 || seq_size <= 0) {
		nRet = -1;
		goto nExit;
	}

	gnSgmImgMaxWidth = max_width;
	gnSgmImgMaxHeight = max_height;
	gnSgmHoughSeqMaxSize = seq_size;
	gnSgmMaxNumrho = (max_width + max_height) * 2 + 1;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
	gucSgmParallelMask = (unsigned char *) malloc(gnSgmImgMaxWidth * gnSgmImgMaxHeight * sizeof(unsigned char));
	if(!gucSgmParallelMask) {
		nRet = -1;
		goto nExit;
	}

	gucSgmVerticalMask = (unsigned char *) malloc(gnSgmImgMaxWidth * gnSgmImgMaxHeight * sizeof(unsigned char));
	if(!gucSgmVerticalMask) {
		nRet = -1;
		goto nExit;
	}
#endif
#endif
#endif

	gpptSgmImgEffcRowRanges = (RyuPoint *) malloc(gnSgmImgMaxHeight * sizeof(RyuPoint));
	if(!gpptSgmImgEffcRowRanges) {
		nRet = -1;
		goto nExit;
	}

	gpptSgmVerticalSeq = (RyuPoint *) malloc(gnSgmHoughSeqMaxSize * sizeof(RyuPoint));
	if(!gpptSgmVerticalSeq) {
		nRet = -1;
		goto nExit;
	}

	gpptSgmParallelSeq = (RyuPoint *) malloc(gnSgmHoughSeqMaxSize * sizeof(RyuPoint));
	if(!gpptSgmParallelSeq) {
		nRet = -1;
		goto nExit;
	}

	gnSgmHoughAccum = (int *) malloc(gnSgmNumangle * gnSgmMaxNumrho * sizeof(int));
	if(!gnSgmHoughAccum) {
		nRet = -1;
		goto nExit;
	}

	gpnSgmCastAccumVerti = (int *) malloc(gnSgmMaxNumrho * sizeof(int));
	if(!gpnSgmCastAccumVerti) {
		nRet = -1;
		goto nExit;
	}

	gpnSgmCastAccumParal = (int *) malloc(gnSgmMaxNumrho * sizeof(int));
	if(!gpnSgmCastAccumParal) {
		nRet = -1;
		goto nExit;
	}

	gpnSgmMedianArray1 = (int *) malloc(gnSgmMaxNumrho * sizeof(int));
	if(!gpnSgmMedianArray1) {
		nRet = -1;
		goto nExit;
	}

	gpnSgmMedianArray2 = (int *) malloc(gnSgmMaxNumrho * sizeof(int));
	if(!gpnSgmMedianArray2) {
		nRet = -1;
		goto nExit;
	}

	gstSegmentBarcodeArea = (SegmentBarcodeArea *) malloc( SGM_BARCODE_AREA_MAX_COUNT * sizeof(SegmentBarcodeArea) );
	if(!gstSegmentBarcodeArea) {
		nRet = -1;
		goto nExit;
	}

	gnSgmInitFlag = 1;
	nRet = 1;

nExit:
	return nRet;
}


void BarcodeSegmentation_release()
{
	gnSgmInitFlag = 0;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if     1
	if(gucSgmParallelMask) {
		free(gucSgmParallelMask);
		gucSgmParallelMask = 0;
	}

	if(gucSgmVerticalMask) {
		free(gucSgmVerticalMask);
		gucSgmVerticalMask = 0;
	}
#endif
#endif
#endif

	if(gpptSgmImgEffcRowRanges) {
		free(gpptSgmImgEffcRowRanges);
		gpptSgmImgEffcRowRanges = 0;
	}

	if(gpptSgmVerticalSeq) {
		free(gpptSgmVerticalSeq);
		gpptSgmVerticalSeq = 0;
	}

	if(gpptSgmParallelSeq) {
		free(gpptSgmParallelSeq);
		gpptSgmParallelSeq = 0;
	}

	if(gnSgmHoughAccum) {
		free(gnSgmHoughAccum);
		gnSgmHoughAccum = 0;
	}

	if(gpnSgmCastAccumVerti) {
		free(gpnSgmCastAccumVerti);
		gpnSgmCastAccumVerti = 0;
	}

	if(gpnSgmCastAccumParal) {
		free(gpnSgmCastAccumParal);
		gpnSgmCastAccumParal = 0;
	}

	if(gpnSgmMedianArray1) {
		free(gpnSgmMedianArray1);
		gpnSgmMedianArray1 = 0;
	}

	if(gpnSgmMedianArray2) {
		free(gpnSgmMedianArray2);
		gpnSgmMedianArray2 = 0;
	}

	if(gstSegmentBarcodeArea) {
		free(gstSegmentBarcodeArea);
		gstSegmentBarcodeArea = 0;
	}

	return;
}

// LTRB标示图像位置信息，row ranges标示每行有效区域范围，以便后续的逐行处理
int FindRectangleEffectiveRange(RyuPoint * corner, int * LTRB, RyuPoint * row_ranges)
{
	int nRet = 0, i = 0, j = 0;

	int flag = 0;
	RyuPoint ptLTRB[4];

	int nRowCount, nTmp = 0;

	float offset = 0, current = 0;

	RyuPoint * pPt = 0;

	LTRB[0] = RYUMIN(RYUMIN(corner[0].x, corner[1].x), RYUMIN(corner[2].x, corner[3].x));
	LTRB[1] = RYUMIN(RYUMIN(corner[0].y, corner[1].y), RYUMIN(corner[2].y, corner[3].y));
	LTRB[2] = RYUMAX(RYUMAX(corner[0].x, corner[1].x), RYUMAX(corner[2].x, corner[3].x));
	LTRB[3] = RYUMAX(RYUMAX(corner[0].y, corner[1].y), RYUMAX(corner[2].y, corner[3].y));

	if(LTRB[0] >= LTRB[2] || LTRB[1] >= LTRB[3]) {
		nRet = 0;
		goto nExit;
	}

	nRowCount = LTRB[3] - LTRB[1] + 1;
	memset(row_ranges, 0, nRowCount * sizeof(RyuPoint));

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			nTmp = (j % 2) ? corner[i].y : corner[i].x;
			if(LTRB[j] == nTmp) {
				ptLTRB[j] = corner[i];
				flag++;
			}
		}
	}

	// flag > 4 特殊角度：有效矩形与外接矩形重合，可直接给出结果
	// flag < 4 无法判断时，按照最大外接矩形给出结果
	if(4 != flag) {
		for(i = 0; i < nRowCount; i++) {
			row_ranges[i].x = LTRB[0];
			row_ranges[i].y = LTRB[2];
		}
		nRet = 1;
		goto nExit;
	}

	// 遍历计算左界
	current = ptLTRB[1].x * 1.0 + 0.5;
	offset = (ptLTRB[0].x - ptLTRB[1].x) * 1.0 / (ptLTRB[0].y - ptLTRB[1].y);
	nTmp = ptLTRB[0].y - ptLTRB[1].y + 1;
	pPt = row_ranges;
	for(i = 0; i <= nTmp; i++) {
		pPt->x = (int)current;
		current += offset;
		pPt++;
	}

	current = ptLTRB[0].x * 1.0 + 0.5;
	offset = (ptLTRB[3].x - ptLTRB[0].x) * 1.0 / (ptLTRB[3].y - ptLTRB[0].y);
	nTmp = ptLTRB[3].y - ptLTRB[0].y + 1;
	for(i = 0; i < nTmp; i++) {
		pPt->x = (int)current;
		current += offset;
		pPt++;
	}

	// 遍历计算右界
	current = ptLTRB[1].x * 1.0 + 0.5;
	offset = (ptLTRB[2].x - ptLTRB[1].x) * 1.0 / (ptLTRB[2].y - ptLTRB[1].y);
	nTmp = ptLTRB[2].y - ptLTRB[1].y + 1;
	pPt = row_ranges;
	for(i = 0; i <= nTmp; i++) {
		pPt->y = (int)current;
		current += offset;
		pPt++;
	}

	current = ptLTRB[2].x * 1.0 + 0.5;
	offset = (ptLTRB[3].x - ptLTRB[2].x) * 1.0 / (ptLTRB[3].y - ptLTRB[2].y);
	nTmp = ptLTRB[3].y - ptLTRB[2].y + 1;
	for(i = 0; i < nTmp; i++) {
		pPt->y = (int)current;
		current += offset;
		pPt++;
	}

	nRet = 2;

nExit:
	return nRet;
}

// 令条码位置信息与图像参数相适应，更新LTRB，该row_ranges为相对坐标
int RectRangeAdaptImageFigure(int * LTRB, RyuPoint * row_ranges, int width, int height)
{
	int nRet = 0, i = 0;

	RyuPoint rglrColRng, rglrRowRng, tmpRowRng;

	int col_offset = 0, nColCount = 0;

	if( LTRB[0] >= 0 && LTRB[0] < width && LTRB[2] >= 0 && LTRB[2] < width
		&& LTRB[1] >= 0 && LTRB[1] < height && LTRB[3] >= 0 && LTRB[3] < height ) {
			nColCount = LTRB[3] - LTRB[1] + 1;
			for( i = 0; i < nColCount; i++ ) {
				row_ranges[i].x = RYUMAX( LTRB[0], RYUMIN(LTRB[2], row_ranges[i].x) );
				row_ranges[i].y = RYUMAX( LTRB[0], RYUMIN(LTRB[2], row_ranges[i].y) );
				row_ranges[i].x -= LTRB[0];
				row_ranges[i].y -= LTRB[0];
			}
			nRet = 1;
			goto nExit;
	}

	rglrRowRng.x = RYUMAX(0, RYUMIN(width-1, LTRB[0]));
	rglrRowRng.y = RYUMAX(0, RYUMIN(width-1, LTRB[2]));

	rglrColRng.x = RYUMAX(0, RYUMIN(height-1, LTRB[1]));
	rglrColRng.y = RYUMAX(0, RYUMIN(height-1, LTRB[3]));

	if(rglrRowRng.x >= rglrRowRng.y || rglrColRng.x >= rglrColRng.y) {
		nRet = 0;
		goto nExit;
	}

	col_offset = rglrColRng.x - LTRB[1];
	if(col_offset < 0) {
		nRet = 0;
		goto nExit;
	}

	nColCount = rglrColRng.y - rglrColRng.x + 1;
	for(i = 0; i < nColCount; i++) {
		tmpRowRng.x = RYUMAX(rglrRowRng.x, RYUMIN(rglrRowRng.y, row_ranges[i+col_offset].x));
		tmpRowRng.y = RYUMAX(rglrRowRng.x, RYUMIN(rglrRowRng.y, row_ranges[i+col_offset].y));

		// 由绝对坐标化为相对坐标
		tmpRowRng.x -= rglrRowRng.x;
		tmpRowRng.y -= rglrRowRng.x;

		row_ranges[i] = tmpRowRng;
	}

	LTRB[0] = rglrRowRng.x;
	LTRB[1] = rglrColRng.x;
	LTRB[2] = rglrRowRng.y;
	LTRB[3] = rglrColRng.y;

	nRet = 2;
nExit:
	return nRet;
}


int GetSpecifiedGradientMasks( unsigned char * in_data, int width, int height, int * LTRB, RyuPoint * row_ranges,
							  int apprx_angle, int ref_grad, unsigned char * parallel_mask, unsigned char * vertical_mask )
{
	int nRet = 0;

	int i = 0, j = 0;

	int nColCount = 0, nRowCount = 0, nPixelCount = 0;

	int gradthres = 0;
	const float gradthres_ratio = 0.8;

	//////////////////////////////////////////////////////////////////////////
	// 为了适应Hough检测角度范围，这里也要增大检索范围
	//const int anglthres = 10;
	const int anglthres = SEGMENT_ANGLE_SCAN_RADIUS;

	unsigned char * pData = 0, * pParal = 0, * pVerti = 0;

	int base = 0, dx = 0, dy = 0, theta = 0;

	RyuPoint * verticalSeq = gpptSgmVerticalSeq, * parallelSeq = gpptSgmParallelSeq;

	RyuPoint range1, range2;
	int tmp1 = 0, tmp2 = 0;

	// 角度划分区间，皆为左闭右开
	int angle_range[10] = { 0, 14, 36, 55, 77, 104, 126, 145, 167, 180 };
	int offset1[9] = { width,  width-1, width-1,  -1,		1,  1,		 width+1,	width,    width },
		offset2[9] = { -width, -width,  -width+1, -width+1, -1, -width-1, -width-1, -width-1, -width };
	int offset1_t[9] = { 1,  1,			width+1,  width,	width,  width-1, width-1,  -1,		 1 },
		offset2_t[9] = { -1, -width-1,  -width-1, -width-1, -width, -width,  -width+1, -width+1, -1 };
	int seq = -1;

	gradthres = ref_grad * RYUMAX( abs(ryuSinShift(apprx_angle)), abs(ryuCosShift(apprx_angle)) );
	gradthres >>= TRIGONOMETRIC_SHIFT_DIGIT;
	gradthres = RYUMAX( 10, gradthres * gradthres_ratio );	// 0.8为微调参数

	// 确定角度划分区间
	for( i = 0; i < 9; i++ ) {
		if( apprx_angle >= angle_range[i] && apprx_angle < angle_range[i+1] ) {
			seq = i;
			break;
		}
	}
	if( seq < 0 || seq >= 9 ) {
		nRet = -1;
		goto nExit;
	}

	// 确定取值区间
	tmp1 = apprx_angle - anglthres;
	tmp2 = apprx_angle + anglthres;
	if( 0 >= tmp1 && 0 < tmp2) {
		range1.x = range2.x = -cnRyuAtanLUTHalf[-tmp1];
		range1.y = range2.y = cnRyuAtanLUTHalf[tmp2];
	} else if( 0 < tmp1 && 90 > tmp2 ) {
		range1.x = range2.x = cnRyuAtanLUTHalf[tmp1-1];
		range1.y = range2.y = cnRyuAtanLUTHalf[tmp2];
	} else if( 90 >= tmp1 && 90 <= tmp2 ) {
		range1.x = cnRyuAtanLUTHalf[tmp1-1];
		range1.y = 256 << TRIGONOMETRIC_SHIFT_DIGIT;
		range2.x = -range1.y;
		range2.y = -cnRyuAtanLUTHalf[179-tmp2];
	} else if( 90 < tmp1 && 180 > tmp2 ) {
		range1.x = range2.x = -cnRyuAtanLUTHalf[179-tmp1];
		range1.y = range2.y = -cnRyuAtanLUTHalf[179-tmp2];
	} else if( 180 > tmp1 && 180 <= tmp2) {
		range1.x = range2.x = -cnRyuAtanLUTHalf[179-tmp1];
		range1.y = range2.y = cnRyuAtanLUTHalf[tmp2-180];
	} else {
		nRet = -1;
		goto nExit;
	}

	nColCount = LTRB[3] - LTRB[1] + 1;
	gnSgmVerticalSeqCnt = gnSgmParallelSeqCnt = 0;

	for( j = 1; j < nColCount - 1; j++ ) {
		base = (j+LTRB[1]) * width + LTRB[0] + row_ranges[j].x + 1;
		pData = in_data + base;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
		pParal = parallel_mask + base;
		pVerti = vertical_mask + base;
#endif
#endif
#endif
		for( i = row_ranges[j].x + 1; i < row_ranges[j].y; i++ ) {
			// 条码方向敏感梯度点标记
			dx = *(pData+1) - *(pData-1);
			dy = *(pData+width) - *(pData-width);
// 			theta = ryuArctan180Shift( dy, dx );
// 			if( anglthres >= ryuCycleDistance( theta, apprx_angle, 180 ) 
			tmp1 = ( 0 == dx ) ? (255 << TRIGONOMETRIC_SHIFT_DIGIT) : (dy << TRIGONOMETRIC_SHIFT_DIGIT) / dx;
			if( ((tmp1 >= range1.x && tmp1 <= range1.y) || (tmp1 >= range2.x && tmp1 <= range2.y))
				&& (gradthres <= abs(dx) || gradthres <= abs(dy)) 
				&& gnSgmParallelSeqCnt < gnSgmHoughSeqMaxSize) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
				*pParal = 0xff;
#endif
#endif
#endif
				parallelSeq[gnSgmParallelSeqCnt].x = i;
				parallelSeq[gnSgmParallelSeqCnt].y = j;
				gnSgmParallelSeqCnt++;
// 				// 膨胀操作
// 				*(pParal-width-1) = *(pParal-width) = *(pParal-width+1) = 0xff;
// 				*(pParal-1) = *(pParal+1) = 0xff;
// 				*(pParal+width-1) = *(pParal+width) = *(pParal+width+1) = 0xff;
			}
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
			else {
				*pParal = 0;
			}
#endif
#endif
#endif

			// 条码法方向敏感梯度点标记，写入mask用于调试显示，写入HoughSeq用于后续Hough检测
			dx = *(pData+offset1[seq]) - *(pData+offset2[seq]);
			dy = *(pData+offset1_t[seq]) - *(pData+offset2_t[seq]);
			if( gradthres <= abs(dx) && gradthres > abs(dy) 
				&& gnSgmVerticalSeqCnt < gnSgmHoughSeqMaxSize ) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
				*pVerti = 0xff;
#endif
#endif
#endif
				verticalSeq[gnSgmVerticalSeqCnt].x = i;
				verticalSeq[gnSgmVerticalSeqCnt].y = j;
				gnSgmVerticalSeqCnt++;
			}
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
			else {
				*pVerti = 0;
			}
			pParal++;
			pVerti++;
#endif
#endif
#endif
			pData++;
			nPixelCount++;
		}
	}

	nRet = nPixelCount;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
#if		1
	printf("Find %d effective Parallel points, and %d effective Vertical points\n",
		gnSgmParallelSeqCnt, gnSgmVerticalSeqCnt);
	CvSize sz = cvSize( LTRB[2]-LTRB[0]+1, LTRB[3]-LTRB[1]+1 );
	IplImage * iplSrcSlice = cvCreateImage( sz, 8, 1 );
	IplImage * iplMaskSlice = cvCreateImage( sz, 8, 3 );
	for( i = 0; i < sz.height; i++ ) {
		base = (i+LTRB[1]) * width + LTRB[0];
		memcpy( iplSrcSlice->imageData+i*iplSrcSlice->widthStep, in_data+base, sz.width * sizeof(unsigned char) );
	}
	cvCvtColor( iplSrcSlice, iplMaskSlice, CV_GRAY2BGR );

	//cvNamedWindow("ImageSlice");
	//cvShowImage("ImageSlice", iplSrcSlice);

	// 绘制有效区域图像边框
	for( j = 0; j < sz.height - 1; j++ ) {
		cvLine( iplMaskSlice, cvPoint(row_ranges[j].x, j), cvPoint(row_ranges[j+1].x, j+1), CV_RGB(0,0,255) );
		cvLine( iplMaskSlice, cvPoint(row_ranges[j].y, j), cvPoint(row_ranges[j+1].y, j+1), CV_RGB(0,0,255) );
	}

	for(j = 1; j < sz.height - 1; j++) {
		base = (j+LTRB[1]) * width + LTRB[0] + row_ranges[j].x + 1;
		pParal = parallel_mask + base;
		pVerti = vertical_mask + base;
		int dbg_base = j * iplMaskSlice->widthStep + row_ranges[j].x * 3;
		unsigned char * pIplMask = (unsigned char *)iplMaskSlice->imageData + dbg_base;
		for(int i = 1; i < row_ranges[j].y - row_ranges[j].x; i++) {
			if(*pParal && *pVerti) {
				*pIplMask = 0;
				*(pIplMask+1) = 255;
				*(pIplMask+2) = 255;
			}
			else if(*pParal) {
				*pIplMask = 0;
				*(pIplMask+1) = 255;
				*(pIplMask+2) = 0;
			}
			else if(*pVerti) {
				*pIplMask = 0;
				*(pIplMask+1) = 0;
				*(pIplMask+2) = 255;
			}
			pParal++;	pVerti++;	pIplMask+=3;
		}
	}

	cvNamedWindow("MaskSlice");
	cvShowImage("MaskSlice", iplMaskSlice);
	//cvWaitKey();

	// 全局调试图像拷贝
	giplSrcSlice = cvCreateImage(sz, 8, 1);
	cvCopy(iplSrcSlice, giplSrcSlice);
	giplSrcSlice3C = cvCreateImage(sz, 8, 3);
	cvCopy(iplMaskSlice, giplSrcSlice3C);

	if(iplSrcSlice)
		cvReleaseImage(&iplSrcSlice);
	if(iplMaskSlice)
		cvReleaseImage(&iplMaskSlice);
#endif
#endif
#endif

nExit:
	return nRet;
}


int CorrectAngleViaStandardHoughTrans( RyuPoint * hough_seq, int seq_count, int * LTRB, 
		int apprx_angle, int * exact_angle )
{
	int nRet = 0;
	int line_count = 0;

	int i = 0, x = 0, y = 0;

	int width = LTRB[2] - LTRB[0] + 1;
	int height = LTRB[3] - LTRB[1] + 1;

	const int numrho = ( (width+height) * 2 + 1 );    //  定义hough图像中的长度;
	const int rhoffset = width + height;
	int r = 0, n = 0, count = 0;
	int max_count = 0;
	int line_thresh = 0;

	int * adata0 = gnSgmHoughAccum, *adata = 0;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);
	const float lineThreshRt = 0.5;

	int angle[gnSgmNumangle] = {0};
	int sine[gnSgmNumangle] = {0};
	int cosine[gnSgmNumangle] = {0};

	int linecnt[gnSgmNumangle] = {0};
	int elemcnt[gnSgmNumangle] = {0};

	int maxLinecnt = 0, maxLinecntIdx = 0, maxElemcnt = 0, maxElemcntIdx = 0;

	memset( adata0, 0, gnSgmNumangle * numrho * sizeof(int) );

	for( n = 0; n < gnSgmNumangle; n++ ) {
		angle[n] = ryuCycle( apprx_angle-gnSgmNumangleH+n, 180 );
		sine[n] = ryuSinShift( angle[n] + 90 );
		cosine[n] = ryuCosShift( angle[n] + 90 );
	}

	for( i = 0; i < seq_count; i++ ) {
		adata = adata0;
		// 对此点逐行更新累加器
		for( n = 0; n < gnSgmNumangle; n++, adata += numrho ) {
			r = ( hough_seq[i].x * cosine[n] + hough_seq[i].y * sine[n] + fixedhalf ) 
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			//r = ( hough_seq[i].x * cosine[n] + hough_seq[i].y * sine[n] ) 
			//	>> TRIGONOMETRIC_SHIFT_DIGIT;
			r += rhoffset;		// 所有r都加上图像的行数和列数，让r的结果永远为正
			adata[r]++;
			max_count = RYUMAX( adata[r], max_count );
		}
	}

	// 不信服的结果
	if( max_count < PARALLEL_CAST_THRESHOLD_BASIC ) {
		*exact_angle = apprx_angle;
		nRet = 0;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "未找到有效矫正直线, max_count=%d(<38)\n", max_count );
#endif
#endif
	}

	line_thresh = max_count * lineThreshRt;

	// 遍历累加器，找出有效直线
	for( n = 0; n < gnSgmNumangle; n++ ) {
		adata = adata0 + n * numrho;
		for( r = 0; r < numrho; r++ ) {
			count = adata[r];
			if( count > line_thresh ) {
				linecnt[n]++;
				elemcnt[n] += count;
				line_count++;
			}
		}

		if( linecnt[n] > maxLinecnt ) {
			maxLinecnt = linecnt[n];
			maxLinecntIdx = n;
		} else if( linecnt[n] == maxLinecnt && elemcnt[n] > elemcnt[maxLinecntIdx] ) {
			maxLinecnt = linecnt[n];
			maxLinecntIdx = n;
		}

		if( elemcnt[n] > maxElemcnt ) {
			maxElemcnt = elemcnt[n];
			maxElemcntIdx = n;
		}
	}

	if(line_count <= 0) {
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "未找到有效矫正直线, threshold=%d, max_count=%d\n", line_thresh, max_count );
#endif
#endif
		*exact_angle = apprx_angle;
		nRet = line_count;
		goto nExit;
	}

	if( maxLinecnt == linecnt[maxElemcntIdx] && maxLinecnt > 0 ) {
		*exact_angle = angle[maxElemcntIdx];
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "匹配类型1(双匹配): 直线%d条, 元素%d个\n", maxLinecnt, maxElemcnt );
#endif
#endif
	} else if( maxElemcnt > 3 * (elemcnt[maxLinecntIdx]>>1) ) {
		*exact_angle = angle[maxElemcntIdx];
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "匹配类型2(点数显著): 直线%d条, 元素%d个(最多%d条，元素%d个)\n", 
			linecnt[maxElemcntIdx], maxElemcnt, maxLinecnt, elemcnt[maxLinecntIdx] );
#endif
#endif
	} else if( abs(maxLinecntIdx - maxElemcntIdx) <= 4 ) {
		*exact_angle = angle[(maxLinecntIdx + maxElemcntIdx)>>1];
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "匹配类型3(距离近取平均): 直线%d条, 元素%d个 && 直线%d条，元素%d个)\n", 
			linecnt[maxElemcntIdx], maxElemcnt, maxLinecnt, elemcnt[maxLinecntIdx] );
#endif
#endif
	} else {
		*exact_angle = apprx_angle;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf( "直线无法匹配\n" );
#endif
#endif
	}

	nRet = line_count;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	printf( "Find %d effective hough line(s), threshold=%d, apprx_angle=%d, exact_angle=%d\n", 
		line_count, line_thresh, apprx_angle, *exact_angle );
	IplImage * iplSrcSlice = cvCreateImage( cvGetSize(giplSrcSlice), 8, 1 );
	IplImage * iplSrcSlice3C = cvCreateImage( cvGetSize(giplSrcSlice), 8, 3 );
// 	cvCopy( giplSrcSlice, iplSrcSlice );
// 	cvCvtColor( iplSrcSlice, iplSrcSlice3C, CV_GRAY2BGR );
	cvCopy( giplSrcSlice3C, iplSrcSlice3C );
	debugDrawLineAcrossImage(iplSrcSlice3C, apprx_angle, CV_RGB(0,0,255));
	debugDrawLineAcrossImage(iplSrcSlice3C, *exact_angle, CV_RGB(255,0,255));
	cvNamedWindow("MaskSlice");
	cvShowImage("MaskSlice", iplSrcSlice3C);
	//cvWaitKey();
	cvReleaseImage( &iplSrcSlice );
	cvReleaseImage( &iplSrcSlice3C );
#endif
#endif

nExit:
	return nRet;
}


#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
void debugDrawLineAcrossImage(IplImage * img3C, int angle, CvScalar rgb)
{
	if(angle == 0) {
		cvLine( img3C, cvPoint(0, img3C->height/2), cvPoint(img3C->width, img3C->height/2), rgb );
	} else if(angle == 90) {
		cvLine( img3C, cvPoint(img3C->width/2, 0), cvPoint(img3C->width/2, img3C->height), rgb );
	} else {
		int r = ryuCosShift(angle+90) * img3C->width / 2 + ryuSinShift(angle+90) * img3C->height / 2;
		CvPoint pt1, pt2;
		pt1.x = 0;
		pt1.y = r / ryuSinShift(angle+90);
		if(pt1.y < 0 || pt1.y > img3C->height) {
			pt1.y = 0;
			pt1.x = r / ryuCosShift(angle+90);
		}
		pt2.x = img3C->width;
		pt2.y = (r - pt2.x * ryuCosShift(angle+90)) / ryuSinShift(angle+90);
		if(pt2.y < 0 || pt1.y > img3C->height) {
			pt2.y = img3C->height;
			pt2.x = (r - pt2.y * ryuSinShift(angle+90)) / ryuCosShift(angle+90);
		}
		cvLine( img3C, pt1, pt2, rgb );
	}
}
#endif
#endif

void UpdateAreaCornerIntercept( RyuPoint * corner, RyuPoint * intercept, RyuPoint * ontercept, 
		int correct_angle )
{
	int i = 0;
	int A = 0, B = 0, AT = 0, BT = 0;
	int C[4] = {0}, CT[4] = {0};

	int min_intcpt = 0, max_intcpt = 0, min_ontcpt = 0, max_ontcpt = 0;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	A = ryuCosShift( correct_angle + 90 );
	B = ryuSinShift( correct_angle + 90 );
	AT = ryuCosShift( correct_angle + 180 );
	BT = ryuSinShift( correct_angle + 180 );

	C[0]  = A *  corner[0].x + B *  corner[0].y;
	CT[0] = AT * corner[0].x + BT * corner[0].y;

// 	min_intcpt = max_intcpt = C[0];
// 	min_ontcpt = max_ontcpt = CT[0];
	min_intcpt = C[0] - fixedhalf;
	max_intcpt = C[0] + fixedhalf;
	min_ontcpt = CT[0] - fixedhalf;
	max_ontcpt = CT[0] + fixedhalf;

//	for( i = 1; i < 4; i++ ) {
//		C[i]  = A *  corner[i].x + B *  corner[i].y + fixedhalf;
//		CT[i] = AT * corner[i].x + BT * corner[i].y + fixedhalf;
//		min_intcpt = RYUMIN( min_intcpt, C[i] );
//		max_intcpt = RYUMAX( max_intcpt, C[i] );
//		min_ontcpt = RYUMIN( min_ontcpt, CT[i] );
//		max_ontcpt = RYUMAX( max_ontcpt, CT[i] );
//	}

	for( i = 1; i < 4; i++ ) {
		C[i]  = A *  corner[i].x + B *  corner[i].y;
		CT[i] = AT * corner[i].x + BT * corner[i].y;
		min_intcpt = RYUMIN( min_intcpt, C[i]-fixedhalf );
		max_intcpt = RYUMAX( max_intcpt, C[i]+fixedhalf );
		min_ontcpt = RYUMIN( min_ontcpt, CT[i]-fixedhalf );
		max_ontcpt = RYUMAX( max_ontcpt, CT[i]+fixedhalf );
	}

	min_intcpt >>= TRIGONOMETRIC_SHIFT_DIGIT;
	max_intcpt >>= TRIGONOMETRIC_SHIFT_DIGIT;
	min_ontcpt >>= TRIGONOMETRIC_SHIFT_DIGIT;
	max_ontcpt >>= TRIGONOMETRIC_SHIFT_DIGIT;

	intercept->x = min_intcpt;
	intercept->y = max_intcpt;
	ontercept->x = min_ontcpt;
	ontercept->y = max_ontcpt;

	corner[0].x = (min_intcpt * BT - max_ontcpt * B + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	corner[0].y = (max_ontcpt * A - min_intcpt * AT + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;

	corner[1].x = (max_intcpt * BT - max_ontcpt * B + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	corner[1].y = (max_ontcpt * A - max_intcpt * AT + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;

	corner[2].x = (min_intcpt * BT - min_ontcpt * B + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	corner[2].y = (min_ontcpt * A - min_intcpt * AT + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;

	corner[3].x = (max_intcpt * BT - min_ontcpt * B + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	corner[3].y = (min_ontcpt * A - max_intcpt * AT + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;

	return;
}


int CastPointsAlongSpecifiedOrientation(RyuPoint * pnt_seq, int seq_count, int * LTRB, int angle, 
		int * cast_array1, int * cast_array2)
{
	int i = 0, nRet = 0;

	int width = LTRB[2] - LTRB[0] + 1;
	int height = LTRB[3] - LTRB[1] + 1;

	const int numrho = ( (width+height) * 2 + 1 );    //  定义hough图像中的长度;
	const int rhoffset = width + height;
	int r = 0;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);
	int A = ryuCosShift( angle + 90 );
	int B = ryuSinShift( angle + 90 );
	int AT = ryuCosShift( angle + 180 );
	int BT = ryuSinShift( angle + 180 );

	memset( cast_array1, 0, numrho * sizeof(int) );
	memset( cast_array2, 0, numrho * sizeof(int) );

	for( i = 0; i < seq_count; i++ ) {
		r = ( pnt_seq[i].x * A + pnt_seq[i].y * B + fixedhalf )
			>> TRIGONOMETRIC_SHIFT_DIGIT;
		//r = ( pnt_seq[i].x * A + pnt_seq[i].y * B )
		//	>> TRIGONOMETRIC_SHIFT_DIGIT;
		r += rhoffset;		// 所有r都加上图像的行数和列数，让r的结果永远为正
		cast_array1[r]++;

		r = ( pnt_seq[i].x * AT + pnt_seq[i].y * BT + fixedhalf )
			>> TRIGONOMETRIC_SHIFT_DIGIT;
		//r = ( pnt_seq[i].x * AT + pnt_seq[i].y * BT )
		//	>> TRIGONOMETRIC_SHIFT_DIGIT;
		r += rhoffset;
		cast_array2[r]++;
	}

	nRet = 1;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	int max_cnt1 = 0, max_cnt2 = 0;
	for(r = 0; r < numrho; r++) {
		max_cnt1 = RYUMAX(max_cnt1, cast_array1[r]);
		max_cnt2 = RYUMAX(max_cnt2, cast_array2[r]);
	}

	if(max_cnt1 <= 0 || max_cnt2 <= 0) {
		nRet = -1;
		goto nExit;
	}
	// 绘制曲线图
	printf("cast_array1 max_cnt=%d, cast_array2 max_cnt=%d\n", max_cnt1, max_cnt2);
	giplCurve = cvCreateImage(cvSize(numrho, 400), 8, 3);
	cvZero(giplCurve);
	for(r = 0; r < numrho; r++) {
		cvLine(giplCurve, cvPoint(r, 200-cast_array1[r]*200/max_cnt1), cvPoint(r, 200),
			CV_RGB(0,255,0));
		cvLine(giplCurve, cvPoint(r, 400-cast_array2[r]*200/max_cnt2), cvPoint(r, 400),
			CV_RGB(255,0,0));
	}
	cvNamedWindow("CastCurve");
	cvShowImage("CastCurve", giplCurve);
	cvWaitKey();
#endif
#endif
nExit:
	return nRet;
}

// 定位条码上下界
int PressUponBarcodeIntercept( int * cast_array, int * LTRB, RyuPoint * intercept, int * param)
{
	int i = 0, j = 0, nRet = 0;

	int width = LTRB[2] - LTRB[0] + 1;
	int height = LTRB[3] - LTRB[1] + 1;

	const int rhoffset = width + height;
	const int numrho = rhoffset * 2 + 1;    //  定义hough图像中的长度;
	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	int max_val = 0, val_thresh = 0;
	int count = 0, state = 0, dist = 0;

	const int max_count = gnSgmInterceptMaxCount;	// 最多支持8个，再多则抛弃
	RyuPoint ptStartstop[max_count];
	int avg_vals[max_count] = {0};
	RyuPoint ptTemp;

	// 可调算法参数
	const int	basic_thresh = 38;
	const int	fuse_dist = 8;
	const int	min_dist = 8;
	const float val_thresh_rt = 0.6;
	const float fuse_ratio = 0.8;

	for( i = 0; i < numrho; i++ ) {
		max_val = RYUMAX( max_val, cast_array[i] );
	}

	// 最大值小于阈值，直接判定不为条码
	if( max_val < basic_thresh ) {
		nRet = 0;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
		printf("条码方向投影最值，判定区域无条码特征段，最大投影%d (阈值%d)\n", max_val, basic_thresh);
#endif
#endif
		goto nExit;
	}

	val_thresh = max_val * val_thresh_rt;
	for( i = 0; i < numrho; i++ ) {
		if( cast_array[i] < val_thresh ) {
			if( state ) {			// 状态结束，计算本段平均值
				state = 0;
				ptStartstop[count].y = i - 1;
				dist = ptStartstop[count].y - ptStartstop[count].x + 1;
				avg_vals[count] /= dist;
				if( ptStartstop[count].x - ptStartstop[count-1].y < fuse_dist
					&& RYUMIN(avg_vals[count], avg_vals[count-1]) 
						> RYUMAX(avg_vals[count], avg_vals[count-1]) * fuse_ratio
					&& count > 0 ) {	// 对比上一段进行融合/分离
					ptStartstop[count-1].y = ptStartstop[count].y;
					ptStartstop[count].x = ptStartstop[count].y = 0;
					avg_vals[count-1] = RYUMAX(avg_vals[count], avg_vals[count-1]);
					avg_vals[count] = 0;
				} else {
					count++;
					if( count >= max_count )
						break;
				}
			} 
		} else {
			if( state ) {		// 状态持续，累计区间内投影值
				avg_vals[count] += cast_array[i];
			} else {		// 状态开始，记录开始参数
				state = 1;
				ptStartstop[count].x = i;
				avg_vals[count] = cast_array[i];
			}
		}
	}

	// 过滤无效的区间
	for( i = 0; i < count; i++ ) {
		if( 0 == avg_vals[i] ) 
			break;
		dist = ptStartstop[i].y - ptStartstop[i].x + 1;
		if( dist < min_dist ) {
			ptStartstop[i] = ptStartstop[count-1];
			avg_vals[i] = avg_vals[count-1];
			ptStartstop[count-1].x = ptStartstop[count-1].y = avg_vals[count-1] = 0;
			count--;
			i--;
		}
	}

	// 排序（宽度）
	for( i = 0; i < count; i++ ) {
		for( j = i+1; j < count; j++ ) {
			if( (ptStartstop[i].y-ptStartstop[i].x) * avg_vals[i] 
				< (ptStartstop[j].y-ptStartstop[j].x) * avg_vals[j] ) {
				ptTemp = ptStartstop[i];
				ptStartstop[i] = ptStartstop[j];
				ptStartstop[j] = ptTemp;
				state = avg_vals[i];
				avg_vals[i] = avg_vals[j];
				avg_vals[j] = state;
			}
		}
		intercept[i].x = ptStartstop[i].x - rhoffset;
		intercept[i].y = ptStartstop[i].y - rhoffset;
		param[i] = avg_vals[i];
	}

	nRet = count;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	if( count <= 0 ) 
		printf( "条码方向投影搜索后，判定区域无条码特征段\n" );
	cvLine( giplCurve, cvPoint(0, 200-basic_thresh*200/max_val), cvPoint(numrho, 200-basic_thresh*200/max_val),
		CV_RGB(0,255,0) );
	cvLine( giplCurve, cvPoint(0, 200-val_thresh*200/max_val), cvPoint(numrho, 200-val_thresh*200/max_val),
		CV_RGB(0,255,255) );
	cvInitFont( &cvSgmFont, CV_FONT_HERSHEY_PLAIN, 1, 1, 0.0, 2, CV_AA );
	for( i = 0; i < count; i++ ) {
		printf( "条码方向投影区间%d：intercept=(%d, %d), param=%d\n", i, intercept[i].x, intercept[i].y, param[i] );
		CvScalar color = CV_RGB( 0, 255, rand()%255);
		for( j = ptStartstop[i].x; j <= ptStartstop[i].y; j++ ) {
			cvLine( giplCurve, cvPoint(j, 200-val_thresh*200/max_val), cvPoint(j, 200-cast_array[j]*200/max_val), color );
		}
		sprintf(cvSgmTxt, "%d", i);
		cvPutText( giplCurve, cvSgmTxt, cvPoint((ptStartstop[i].x+ptStartstop[i].y)/2, 100), &cvSgmFont, CV_RGB(255,0,0) );
	}
// 	cvShowImage( "CastCurve", giplCurve );
// 	cvWaitKey();
#endif
#endif
nExit:
	return nRet;
}

// 定位条码左右界
int PressUponBarcodeOntercept( int * cast_array, int * LTRB, int reference, RyuPoint * ontercept, int * param )
{
	int i = 0, j = 0, nRet = 0;

	int width = LTRB[2] - LTRB[0] + 1;
	int height = LTRB[3] - LTRB[1] + 1;

	const int rhoffset = width + height;
	const int numrho = rhoffset * 2 + 1;    //  定义hough图像中的长度;
	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	int max_val = 0, valThreshLow = 0, valThreshHigh = 0;

	int * medianArrayVal = gpnSgmMedianArray1;
	int * medianArrayDist = gpnSgmMedianArray2;
	int medianCntVal = 0, medianCntDist = 0;
	int nArrayVal = 0, nArrayDist = 0;

	int previous = 0, dist = 0;
	int distThreshLow = 0, distThreshHigh = 0;
	int diffThresh = 0;

	const int maxCountGlobal = gnSgmInterceptMaxCount;
	const int nMaxCount = 32;	// 最多支持32个，再多则抛弃
	RyuPoint ptStartstop[nMaxCount];
	int max_vals[nMaxCount] = {0};
	int pnEffCnt[nMaxCount] = {0};
	int count = 0;
	RyuPoint ptTemp;

	const float valThreshLowRt = 0.2;
	const float valThreshHighRt = 0.5;
	const float distThreshLowRt = 3.0;
	const float distThreshHighRt = 5.0;
	const float diffThreshRt = 0.5;
	const int	min_width = 46;

	valThreshLow = (int)(reference * valThreshLowRt);
	valThreshHigh = (int)(reference * valThreshHighRt);

	// 利用高阈值寻找有效距离，幅度
	for( i = 0; i < numrho; i++ ) {
		max_val = RYUMAX( max_val, cast_array[i] );
		if( cast_array[i] >= valThreshHigh ) {
			if( previous ) {
				dist = i - previous;
				if( dist > 1 ) {
					medianArrayDist[medianCntDist] = dist;
					nArrayDist += dist;
					medianCntDist++;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
					printf("(%d) ", dist);
#endif
#endif
				}
			}
			previous = i;

			medianArrayVal[medianCntVal] = cast_array[i];
			nArrayVal += medianArrayVal[medianCntVal++];
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
			printf("%d ", cast_array[i]);
#endif
#endif
		}

	}

	if(medianCntDist <= 0 || medianCntVal <= 0) {
		nRet = -1;
	}

	// 求取阈值
	diffThresh = (int)(nArrayVal * diffThreshRt / medianCntVal + 0.5);
	distThreshLow = (int)(nArrayDist * distThreshLowRt / medianCntDist + 0.5);
	distThreshHigh = (int)(nArrayDist * distThreshHighRt / medianCntDist + 0.5);
	distThreshLow = RYUMAX( distThreshLow, 10 );	// 最小空白区不得低于10像素点

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	printf("\nArrayDist avg = %.2f, medianCntDist = %d\n", 
		nArrayDist*1.0/medianCntDist, medianCntDist);
	printf("ArrayVal avg = %.2f, medianCntVal = %d\n", 
		nArrayVal*1.0/medianCntVal, medianCntVal);
	printf("distThreshLow = %d, distThreshHigh = %d, diffThresh = %d\n", 
		distThreshLow, distThreshHigh, diffThresh);
#endif
#endif

	// 利用低阈值寻找有效连通(条码左右界)
	// 初步分块
	previous = 0;
	for( i = 0; i < numrho; i++ ) {
		if( cast_array[i] > valThreshLow ) {
			if( previous ) {
				dist = i - previous;
				if( dist > distThreshLow ) {
					ptStartstop[count++].y = previous;
					ptStartstop[count].x = i;
				}
			} else {	// 第一个值
				ptStartstop[count].x = i;
			}
			previous = i;
			max_vals[count] = RYUMAX( max_vals[count], cast_array[i] );
			pnEffCnt[count]++;
		}
	}
	ptStartstop[count++].y = previous;
	
	// 融合
	for( i = 0; i < count - 1; i++ ) {
		dist = ptStartstop[i+1].x - ptStartstop[i].y;
		if( abs(max_vals[i] - max_vals[i+1]) < diffThresh 
			&& dist < distThreshHigh ) {
			ptStartstop[i+1].x = ptStartstop[i].x;
			max_vals[i+1] = RYUMAX( max_vals[i], max_vals[i+1] );
			pnEffCnt[i+1] += pnEffCnt[i];
			ptStartstop[i].x = ptStartstop[i].y = 0;
		}
	}
	
	// 筛选
	for( i = 0; i < count; i++ ) {
		if( 0 == max_vals[i] )
			break;

		if( 0 >= ptStartstop[i].x || 0 >= ptStartstop[i].y ) {
			ptStartstop[i] = ptStartstop[count-1];
			max_vals[i] = max_vals[count-1];
			pnEffCnt[i] = pnEffCnt[count-1];
			max_vals[count-1] = pnEffCnt[count-1] = 0;
			ptStartstop[count-1].x = ptStartstop[count-1].y = 0;
			count--;
			i--;
			continue;
		}

		if( ptStartstop[i].y - ptStartstop[i].x + 1 < min_width) {
			ptStartstop[i] = ptStartstop[count-1];
			max_vals[i] = max_vals[count-1];
			pnEffCnt[i] = pnEffCnt[count-1];
			max_vals[count-1] = pnEffCnt[count-1] = 0;
			ptStartstop[count-1].x = ptStartstop[count-1].y = 0;
			count--;
			i--;
			continue;
		}
	}

	// 排序（宽度）
	for( i = 0; i < count; i++ ) {
		if( i >= maxCountGlobal ) {
			count = maxCountGlobal;
			break;
		}
		for( j = i+1; j < count; j++ ) {
			if( ptStartstop[i].y-ptStartstop[i].x < ptStartstop[j].y-ptStartstop[j].x ) {
				ptTemp = ptStartstop[i];
				ptStartstop[i] = ptStartstop[j];
				ptStartstop[j] = ptTemp;

				previous = pnEffCnt[i];
				pnEffCnt[i] = pnEffCnt[j];
				pnEffCnt[j] = previous;
			}
		}
		ontercept[i].x = ptStartstop[i].x - rhoffset;
		ontercept[i].y = ptStartstop[i].y - rhoffset;
		param[i] = pnEffCnt[i];
	}

	nRet = count;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	cvLine(giplCurve, cvPoint(0, 400-valThreshLow*200/max_val), cvPoint(numrho, 400-valThreshLow*200/max_val),
		CV_RGB(255,128,0));
	cvLine(giplCurve, cvPoint(0, 400-valThreshHigh*200/max_val), cvPoint(numrho, 400-valThreshHigh*200/max_val),
		CV_RGB(255,255,0));
	cvInitFont( &cvSgmFont, CV_FONT_HERSHEY_PLAIN, 1, 1, 0.0, 2, CV_AA );
	for( i = 0; i < count; i++) {
		CvScalar color = CV_RGB( 255, rand()%255, rand()%255);
		for( j = ptStartstop[i].x; j <= ptStartstop[i].y; j++ ) {
			cvLine( giplCurve, cvPoint(j, 400-cast_array[j]*200/max_val), cvPoint(j, 400), color );
		}
		sprintf(cvSgmTxt, "%d", i);
		cvPutText( giplCurve, cvSgmTxt, cvPoint((ptStartstop[i].x+ptStartstop[i].y)/2, 300), &cvSgmFont, CV_RGB(0,255,0) );
		dist = ontercept[i].y - ontercept[i].x + 1;
		printf( "条码法方向投影区间%d：ontercept=(%d, %d), width=%d, param=%d, rate=%.3f\n", 
			i, ontercept[i].x, ontercept[i].y, dist, param[i], param[i]*1.0/dist );
	}
	cvShowImage("CastCurve", giplCurve);
	//cvWaitKey();
#endif
#endif
nExit:
	return nRet;
}

// 	corners->x = (intercept.x * BT - ontercept.y * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 	corners->y = (ontercept.y * A - intercept.x * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 
// 	corners->x = (intercept.y * BT - ontercept.y * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 	corners->y = (ontercept.y * A - intercept.y * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 
// 	corners->x = (intercept.x * BT - ontercept.x * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 	corners->y = (ontercept.x * A - intercept.x * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 
// 	corners->x = (intercept.y * BT - ontercept.x * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
// 	corners->y = (ontercept.x * A - intercept.y * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;


int GetBarcodePureAreaFigures( RyuPoint * intercept, int * param1, int count1, 
		RyuPoint * ontercept, int * param2, int count2, int * LTRB, int angle, 
		RyuPoint main_intrcpt, RyuPoint main_ontrcpt, SegmentBarcodeArea * barcode_area, int barcode_area_idx)
{
	int i = 0, j = 0, nRet = 0;

	int totalCnt = 0;

	int A = 0, B = 0, AT = 0, BT = 0;

	int offsetIn = 0, offsetOn = 0;

	SegmentBarcodeArea tempSegment;
	float ratio = 0.0f;
	int dist = 0, codeW = 0;

	RyuPoint tmpIntrcpt, tmpOntrcpt;
	int tmpMaxAB = 0;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	if( count1 <= 0 || count2 <= 0 ) {
		nRet = 0;
		goto nExit;
	}

	A = ryuCosShift( angle + 90 );
	B = ryuSinShift( angle + 90 );
	AT = ryuCosShift( angle+ 180 );
	BT = ryuSinShift( angle+ 180 );
	tmpMaxAB = RYUMAX( abs(A), abs(B) );

	//////////////////////////////////////////////////////////////////////////
	// [v2.6.1] 注释为
	//ratio = tmpMaxAB * 1.0 / (1 << TRIGONOMETRIC_SHIFT_DIGIT)-SEGMENT_EXTEND_WIDTH_RATIO;

	// [v2.6.1] 更改为
	ratio = tmpMaxAB * 1.0 / (1 << TRIGONOMETRIC_SHIFT_DIGIT);
	ratio = ratio - (1 - ratio) / 2;
	//////////////////////////////////////////////////////////////////////////

	offsetIn = (LTRB[0] * A + LTRB[1] * B + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	offsetOn = (LTRB[0] * AT + LTRB[1] * BT + fixedhalf) >> TRIGONOMETRIC_SHIFT_DIGIT;
	//offsetIn = (LTRB[0] * A + LTRB[1] * B) >> TRIGONOMETRIC_SHIFT_DIGIT;
	//offsetOn = (LTRB[0] * AT + LTRB[1] * BT) >> TRIGONOMETRIC_SHIFT_DIGIT;
	
	if( count1 > 1 && count2 > 1 ) {
		count1 = count2 = 1;
	}

	for( i = 0; i < count1; i++ ) {
		for( j = 0; j < count2; j++ ) {
			tempSegment.min_intcpt = intercept[i].x + offsetIn;
			tempSegment.max_intcpt = intercept[i].y + offsetIn;
			tempSegment.min_ontcpt = ontercept[j].x + offsetOn;
			tempSegment.max_ontcpt = ontercept[j].y + offsetOn;
			if( tempSegment.min_intcpt < main_intrcpt.x
				|| tempSegment.max_intcpt > main_intrcpt.y
				|| tempSegment.min_ontcpt < main_ontrcpt.x
				|| tempSegment.max_ontcpt > main_ontrcpt.y ) {
					continue;
			}
			codeW = tempSegment.max_ontcpt - tempSegment.min_ontcpt + 1;
			if(codeW <= CODE_IMAGE_MIN_WIDTH) {
				continue;
			}

			tempSegment.min_intcpt -= BARCODE_INTERCEPT_EXTEND;
			tempSegment.max_intcpt += BARCODE_INTERCEPT_EXTEND;
			tempSegment.min_ontcpt -= BARCODE_ONTERCEPT_EXTEND;
			tempSegment.max_ontcpt += BARCODE_ONTERCEPT_EXTEND;

			tempSegment.corner[0].x = (tempSegment.min_intcpt * BT - tempSegment.max_ontcpt * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[0].y = (tempSegment.max_ontcpt * A - tempSegment.min_intcpt * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[1].x = (tempSegment.max_intcpt * BT - tempSegment.max_ontcpt * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[1].y = (tempSegment.max_ontcpt * A - tempSegment.max_intcpt * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[2].x = (tempSegment.min_intcpt * BT - tempSegment.min_ontcpt * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[2].y = (tempSegment.min_ontcpt * A - tempSegment.min_intcpt * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[3].x = (tempSegment.max_intcpt * BT - tempSegment.min_ontcpt * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner[3].y = (tempSegment.min_ontcpt * A - tempSegment.max_intcpt * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;

			dist = (int)((tempSegment.max_intcpt - tempSegment.min_intcpt + 1) * (1 - ratio));
			dist = (dist + 1) / 2;
			tmpIntrcpt.x = tempSegment.min_intcpt - dist;
			tmpIntrcpt.y = tempSegment.max_intcpt + dist;
			dist = (int)((tempSegment.max_ontcpt - tempSegment.min_ontcpt + 1) * (1 - ratio));
			dist = (dist + 1) / 2;
			tmpOntrcpt.x = tempSegment.min_ontcpt - dist;
			tmpOntrcpt.y = tempSegment.max_ontcpt + dist;

			tempSegment.corner_ext[0].x = (tmpIntrcpt.x * BT - tmpOntrcpt.y * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[0].y = (tmpOntrcpt.y * A - tmpIntrcpt.x * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[1].x = (tmpIntrcpt.y * BT - tmpOntrcpt.y * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[1].y = (tmpOntrcpt.y * A - tmpIntrcpt.y * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[2].x = (tmpIntrcpt.x * BT - tmpOntrcpt.x * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[2].y = (tmpOntrcpt.x * A - tmpIntrcpt.x * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[3].x = (tmpIntrcpt.y * BT - tmpOntrcpt.x * B + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;
			tempSegment.corner_ext[3].y = (tmpOntrcpt.x * A - tmpIntrcpt.y * AT + fixedhalf)
				>> TRIGONOMETRIC_SHIFT_DIGIT;

			tempSegment.angle = angle;
			//////////////////////////////////////////////////////////////////////////
			// [v2.6.1] 放大标志在后续代码中被废弃使用，故此处
			// [v2.6.1] 注释为
// 			ratio = param2[j] * 1.0 / codeW;
// 			if( codeW > SEGMENT_ZOOM_WIDTH_THRESH && ratio < SEGMENT_ZOOM_RATIO_THRESH ) {
// 				tempSegment.flag = 0;
// 			} else {
// 				tempSegment.flag = 1;
// 			}

			// [v2.6.1] 更改为
			tempSegment.flag = 0;

			tempSegment.score = gnSgmParallelSeqCnt;
			//////////////////////////////////////////////////////////////////////////

			barcode_area[totalCnt++] = tempSegment;
#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
			printf("条码区域%d: ratio=%.3f, dist=%d, flag=%d\n", totalCnt, ratio, dist, tempSegment.flag);
			printf("Barcode intercept: (%d,%d) Area intercept: (%d,%d)\n", 
				tempSegment.min_intcpt, tempSegment.max_intcpt,
				main_intrcpt.x, main_intrcpt.y);
			printf("Barcode ontercept: (%d,%d) Area ontercept: (%d,%d)\n", 
				tempSegment.min_ontcpt, tempSegment.max_ontcpt,
				main_ontrcpt.x, main_ontrcpt.y);
#endif
#endif

			if(totalCnt + barcode_area_idx >= SGM_BARCODE_AREA_MAX_COUNT) 
				break;
		}
		if(totalCnt + barcode_area_idx >= SGM_BARCODE_AREA_MAX_COUNT) 
			break;
	}

	nRet = totalCnt;

#ifdef	_DEBUG_
#ifdef  _DEBUG_SEGMENT
	// 绘制一下
	IplImage * iplSrcSlice3C = cvCreateImage(cvGetSize(giplSrcSlice), 8, 3);
	cvCvtColor(giplSrcSlice, iplSrcSlice3C, CV_GRAY2BGR);
	cvInitFont( &cvSgmFont, CV_FONT_HERSHEY_PLAIN, 1, 1, 0.0, 1, CV_AA );
	for(i = 0; i < totalCnt; i++) {
		for(j = 0; j < 4; j++) {
			cvCircle(iplSrcSlice3C, cvPoint(barcode_area[i].corner[j].x-LTRB[0], barcode_area[i].corner[j].y-LTRB[1])
				, 3, CV_RGB(0,255,0), CV_FILLED);

			sprintf(cvSgmTxt, "%d", j);
			cvPutText( iplSrcSlice3C, cvSgmTxt, cvPoint(barcode_area[i].corner[j].x-LTRB[0], barcode_area[i].corner[j].y-LTRB[1]), 
				&cvSgmFont, CV_RGB(255,255,0) );

			cvCircle(iplSrcSlice3C, cvPoint(barcode_area[i].corner_ext[j].x-LTRB[0], barcode_area[i].corner_ext[j].y-LTRB[1])
				, 3, CV_RGB(0,0,255), CV_FILLED);
		}

		sprintf(cvSgmTxt, "%d:min_in", i);
		cvPutText( iplSrcSlice3C, cvSgmTxt, cvPoint((barcode_area[i].corner[0].x+barcode_area[i].corner[2].x)/2-LTRB[0], 
			(barcode_area[i].corner[0].y+barcode_area[i].corner[2].y)/2-LTRB[1]), 
			&cvSgmFont, CV_RGB(255,255,0) );
		sprintf(cvSgmTxt, "%d:max_in", i);
		cvPutText( iplSrcSlice3C, cvSgmTxt, cvPoint((barcode_area[i].corner[1].x+barcode_area[i].corner[3].x)/2-LTRB[0], 
			(barcode_area[i].corner[1].y+barcode_area[i].corner[3].y)/2-LTRB[1]), 
			&cvSgmFont, CV_RGB(255,255,0) );
		sprintf(cvSgmTxt, "%d:min_on", i);
		cvPutText( iplSrcSlice3C, cvSgmTxt, cvPoint((barcode_area[i].corner[2].x+barcode_area[i].corner[3].x)/2-LTRB[0], 
			(barcode_area[i].corner[2].y+barcode_area[i].corner[3].y)/2-LTRB[1]), 
			&cvSgmFont, CV_RGB(255,255,0) );
		sprintf(cvSgmTxt, "%d:max_on", i);
		cvPutText( iplSrcSlice3C, cvSgmTxt, cvPoint((barcode_area[i].corner[0].x+barcode_area[i].corner[1].x)/2-LTRB[0], 
			(barcode_area[i].corner[0].y+barcode_area[i].corner[1].y)/2-LTRB[1]), 
			&cvSgmFont, CV_RGB(255,255,0) );
	}
	cvNamedWindow("ImageSlice");
	cvShowImage("ImageSlice", iplSrcSlice3C);
	cvWaitKey();
	cvReleaseImage(&iplSrcSlice3C);
#endif
#endif

nExit:
	return nRet;
}


int InterceptCvt2Corners( RyuPoint intercept, RyuPoint ontercept, int angle, RyuPoint * corners )
{
	int A = 0, B = 0, AT = 0, BT = 0;
	int tmpMaxAB = 0, dist = 0;
	double ratio = 0.0F;

	const int fixedhalf = 1 << (TRIGONOMETRIC_SHIFT_DIGIT - 1);

	A = ryuCosShift( angle + 90 );
	B = ryuSinShift( angle + 90 );
	AT = ryuCosShift( angle+ 180 );
	BT = ryuSinShift( angle+ 180 );

	tmpMaxAB = RYUMAX( abs(A), abs(B) );
	ratio = tmpMaxAB * 1.0 / (1 << TRIGONOMETRIC_SHIFT_DIGIT);

	dist = (int)((intercept.y - intercept.x + 1) * (1 - ratio));
	dist = (dist + 1) / 2;
	intercept.x -= dist;
	intercept.y += dist;

	dist = (int)((ontercept.y - ontercept.x + 1) * (1 - ratio));
	dist = (dist + 1) / 2;
	ontercept.x -= dist;
	ontercept.y += dist;

	corners[0].x = (intercept.x * BT - ontercept.y * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[0].y = (ontercept.y * A - intercept.x * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[1].x = (intercept.y * BT - ontercept.y * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[1].y = (ontercept.y * A - intercept.y * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[2].x = (intercept.x * BT - ontercept.x * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[2].y = (ontercept.x * A - intercept.x * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[3].x = (intercept.y * BT - ontercept.x * B + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;
	corners[3].y = (ontercept.x * A - intercept.y * AT + fixedhalf)
		>> TRIGONOMETRIC_SHIFT_DIGIT;

	return 1;
}

