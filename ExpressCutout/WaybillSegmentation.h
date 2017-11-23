#ifndef WAYBILL_SEGMENT_H
#define WAYBILL_SEGMENT_H

#include "RyuCore.h"

typedef struct code_area_node
{
	int		angle;
	int		min_x, max_x;
	int		min_y, max_y;
	int		width;
	int		height;

	int		flag;
	char	str_code[CODE_RESULT_ARR_LENGTH];
} CAN;

typedef struct flood_fill_node
{
	int		label;
	int		min_x, max_x;
	int		min_y, max_y;
	int		pixel_count;

	int		width;
	int		height;

	int		min_v, max_v;

	int		contour_idx;
	int		contour_cnt;
	int		class_idx;
	int		code_in;
	int		code_cnt;

	int		flag;	// For processing,代表图片中的第i个联通区域存在
} FFN;

typedef struct flood_fill_classcluster
{
	int		index;
	int		min_x, max_x;
	int		min_y, max_y;
	int		ffn_count;

	int		width;
	int		height;

	int		flag;	// For processing,代表图片中的第i个联通区域存在
} FFC;

int WaybillSegment(unsigned char * img, int wid, int hei, RyuRect * bound_box);

int ryuInitFloodFill(RyuSize sz, int max_count);

int ryuReleaseFloodFill();

int ryuFloodFill(RyuImage * img_in, RyuPoint seed, int gl_thre, int lo_diff, int up_diff, 
	int label_val, int conn_flag, int stat_flag, int hist_flag);

int WaybillSegm_findContour(FFN * StatisticsNode, int wid, int hei, RyuPoint * ContourPts);

int WaybillSegm_rectangleDistance(RyuRect rc1, RyuRect rc2);

int WaybillSegm_contourDistance(RyuPoint * contour1, int count1, RyuPoint * contour2, int count2, int thresh);

int WaybillSegm_pushCodeAreaStack(RyuPoint * corner, int angle, int flag, char * str_code);

int WaybillSegm_codeinSimilarity(FFN * ffn1, FFN * ffn2);

void ryuResetFloodFill();

int * ryuGetFloodFillLabelMat();

int * ryuGetFloodFillHistogram();

FFN ryuGetFloodFillStatisticsNode();

FFN * ryuGetFloodFillStatistics();

int ryuGetFloodFillStatCount();

int * ryuGetFloodFillContourMat();

FFC * ryuGetFloodFillClassClusters();

RyuPoint * ryuGetFloodFillHoughSeeds();

void WaybillSegm_resetCodeAreaStack();

CAN * WaybillSegm_getCodeAreas();

int WaybillSegm_getCodeAreaCount();

#endif