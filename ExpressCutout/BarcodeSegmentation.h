#ifndef BARCODE_SEGMENTATION_H
#define BARCODE_SEGMENTATION_H

#include "RyuCore.h"

#define SGM_BARCODE_AREA_MAX_COUNT	(128)

typedef struct tagSegmentBarcodeArea
{
	int			index;
	RyuPoint	corner[4];
	RyuPoint	corner_ext[4];
	int			angle;
	int			flag;			// 是否需要做放大处理
	int			min_intcpt;
	int			max_intcpt;
	int			min_ontcpt;
	int			max_ontcpt;
} SegmentBarcodeArea;

int BarcodeSegmentation_init(int max_width, int max_height, int seq_size);

void BarcodeSegmentation_release();

SegmentBarcodeArea * GetSegmentBarcodeAreaPtr();

int SegmentBarcode( unsigned char * in_data, int width, int height, 
		RyuPoint * corner, RyuPoint * intercept, RyuPoint * ontercept, int * angle, int ref_grad, 
		SegmentBarcodeArea * barcode_area, int barcode_area_idx);

void UpdateCodeCorner( SegmentBarcodeArea * codeArea, int leftOffset, int rightOffset );

int InterceptCvt2Corners( RyuPoint intercept, RyuPoint ontercept, int angle, RyuPoint * corners );


#endif

