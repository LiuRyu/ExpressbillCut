#include "stdafx.h"

#include "RyuCore.h"

#define PI		(3.1415926) 
#define KSIZE	(7)  
#define sigma	(1.5)

const int cnRyuSin_3n[361] = {
	0,17,35,53,71,89,107,124,142,160,177,195,212,230,247,265,282,299,316,333,
	350,366,383,400,416,432,448,464,480,496,511,527,542,557,572,587,601,616,
	630,644,658,671,685,698,711,724,736,748,760,772,784,795,806,817,828,838,
	848,858,868,877,886,895,904,912,920,928,935,942,949,955,962,968,973,979,
	984,989,993,997,1001,1005,1008,1011,1014,1016,1018,1020,1021,1022,1023,
	1023,1024,1023,1023,1022,1021,1020,1018,1016,1014,1011,1008,1005,1001,
	997,993,989,984,979,973,968,962,955,949,942,935,928,920,912,904,895,
	886,877,868,858,848,838,828,817,806,795,784,772,760,748,736,724,711,
	698,685,671,658,644,630,616,601,587,572,557,542,527,511,496,480,464,
	448,432,416,400,383,366,350,333,316,299,282,265,247,230,212,195,177,
	160,142,124,107,89,71,53,35,17,0, 
	-17,-35,-53,-71,-89,-107,-124,-142,-160,-177,-195,-212,-230,-247,-265,-282,-299,-316,-333,
	-350,-366,-383,-400,-416,-432,-448,-464,-480,-496,-511,-527,-542,-557,-572,-587,-601,-616,
	-630,-644,-658,-671,-685,-698,-711,-724,-736,-748,-760,-772,-784,-795,-806,-817,-828,-838,
	-848,-858,-868,-877,-886,-895,-904,-912,-920,-928,-935,-942,-949,-955,-962,-968,-973,-979,
	-984,-989,-993,-997,-1001,-1005,-1008,-1011,-1014,-1016,-1018,-1020,-1021,-1022,-1023,
	-1023,-1024,-1023,-1023,-1022,-1021,-1020,-1018,-1016,-1014,-1011,-1008,-1005,-1001,
	-997,-993,-989,-984,-979,-973,-968,-962,-955,-949,-942,-935,-928,-920,-912,-904,-895,
	-886,-877,-868,-858,-848,-838,-828,-817,-806,-795,-784,-772,-760,-748,-736,-724,-711,
	-698,-685,-671,-658,-644,-630,-616,-601,-587,-572,-557,-542,-527,-511,-496,-480,-464,
	-448,-432,-416,-400,-383,-366,-350,-333,-316,-299,-282,-265,-247,-230,-212,-195,-177,
	-160,-142,-124,-107,-89,-71,-53,-35,-17,0
};

const int cnRyuCos_3n[361] = {
	1024,1023,1023,1022,1021,1020,1018,1016,1014,1011,1008,1005,1001,
	997,993,989,984,979,973,968,962,955,949,942,935,928,920,912,904,
	895,886,877,868,858,848,838,828,817,806,795,784,772,760,748,736,
	724,711,698,685,671,658,644,630,616,601,587,572,557,542,527,512,
	496,480,464,448,432,416,400,383,366,350,333,316,299,282,265,247,
	230,212,195,177,160,142,124,107,89,71,53,35,17,0,-17,-35,-53,-71,
	-89,-107,-124,-142,-160,-177,-195,-212,-230,-247,-265,-282,-299,
	-316,-333,-350,-366,-383,-400,-416,-432,-448,-464,-480,-496,-511,
	-527,-542,-557,-572,-587,-601,-616,-630,-644,-658,-671,-685,-698,
	-711,-724,-736,-748,-760,-772,-784,-795,-806,-817,-828,-838,-848,
	-858,-868,-877,-886,-895,-904,-912,-920,-928,-935,-942,-949,-955,
	-962,-968,-973,-979,-984,-989,-993,-997,-1001,-1005,-1008,-1011,
	-1014,-1016,-1018,-1020,-1021,-1022,-1023,-1023,-1024,
	-1023,-1023,-1022,-1021,-1020,-1018,-1016,-1014,-1011,-1008,-1005,-1001,
	-997,-993,-989,-984,-979,-973,-968,-962,-955,-949,-942,-935,-928,-920,-912,-904,
	-895,-886,-877,-868,-858,-848,-838,-828,-817,-806,-795,-784,-772,-760,-748,-736,
	-724,-711,-698,-685,-671,-658,-644,-630,-616,-601,-587,-572,-557,-542,-527,-512,
	-496,-480,-464,-448,-432,-416,-400,-383,-366,-350,-333,-316,-299,-282,-265,-247,
	-230,-212,-195,-177,-160,-142,-124,-107,-89,-71,-53,-35,-17,0,17,35,53,71,
	89,107,124,142,160,177,195,212,230,247,265,282,299,
	316,333,350,366,383,400,416,432,448,464,480,496,511,
	527,542,557,572,587,601,616,630,644,658,671,685,698,
	711,724,736,748,760,772,784,795,806,817,828,838,848,
	858,868,877,886,895,904,912,920,928,935,942,949,955,
	962,968,973,979,984,989,993,997,1001,1005,1008,1011,
	1014,1016,1018,1020,1021,1022,1023,1023,1024
};

// const int cnRyuAtanLUTHalf[90] = {
// 	9, 27, 45, 63, 81, 99, 117, 135, 153, 171, 190, 208, 227, 246, 265, 284, 303, 
// 	323, 343, 363, 383, 403, 424, 445, 467, 488, 511, 533, 556, 579, 603, 628, 652, 
// 	678, 704, 730, 758, 786, 815, 844, 875, 906, 938, 972, 1006, 1042, 1079, 1117, 
// 	1157, 1199, 1242, 1287, 1335, 1384, 1436, 1490, 1547, 1607, 1671, 1738, 1810, 
// 	1886, 1967, 2054, 2147, 2247, 2355, 2472, 2600, 2739, 2892, 3060, 3248, 3457, 
// 	3692, 3960, 4265, 4619, 5033, 5525, 6119, 6852, 7778, 8988, 10635, 13011, 16742, 
// 	23453, 39105, 117339
// };


int ryuSinShift(int angel)
{
	return cnRyuSin_3n[angel];
}

int ryuCosShift(int angel)
{
	return cnRyuCos_3n[angel];
}

int ryuCycle(int a, int range)
{
	int d = a;
	while(d < 0) {
		d += range;
	}

	while(d >= range) {
		d -= range;
	}

	return d;
}

int ryuCycleDistance(int a, int b, int cycle)
{
	int d = abs(a - b);

	if(a < 0 || b < 0 || a >= cycle || b >= cycle) {
#ifdef	_PRINT_PROMPT
		printf("\nWARN! Bad input of ryuCycleDistance, a=%d, b=%d, cycle=%d\n", 
			a, b, cycle);
#endif
		return -1;
	}

	d = (d < cycle - d) ? d : (cycle - d);

	return d;
}

// windlyu opt 20131223
int  ryuArctan180Shift(int dy, int dx)
{
	int i = 0;

	int start = 0, end = 0;

	int t_sign = 0, t_abs = 0;

	if(dy == 0)
		return 0;

	if(dx == 0)
		return 90;
		
	t_sign = ((dy ^ dx) >= 0) ? 1 : (-1);
	t_abs = ((abs(dy))<<TRIGONOMETRIC_SHIFT_DIGIT) / abs(dx);

	if(t_abs < cnRyuAtanLUTHalf[0]) {
		return 0;
	}

	if(t_abs >= cnRyuAtanLUTHalf[89]) {
		return 90;
	}
	
	for(start = 0, end = 89; start < end; ) {
		i = start + ((end - start)>>1);
		if(t_abs >= cnRyuAtanLUTHalf[i] && t_abs < cnRyuAtanLUTHalf[i+1]) {
			if(t_sign == 1) 
				return (i + 1);
			else if(t_sign == -1)
				return (179 - i);
		}
		else if(t_abs < cnRyuAtanLUTHalf[i]) {
			end = i;
		}
		else {
			start = i + 1;
		}
	}

	return -1;
}

RyuPoint ryuDivideIntPoint(int point)
{
	int dx = 0, dy = 0;
	int s = 0;

	RyuPoint pt;

	dx = point >> 16;
	s = dx & 0x8000;
	pt.x = (0 == s) ? (dx & 0x0000ffff) : (dx | 0xffff0000);

	dy = point & 0x0000ffff;
	s = dy & 0x8000;
	pt.y = (0 == s) ? (dy) : (dy | 0xffff0000);

	return pt;
}

int ryuDistanceBtPoints(RyuPoint pt1, RyuPoint pt2)
{
	int d = 0;

	d = (pt1.x - pt2.x) * (pt1.x - pt2.x) + (pt1.y - pt2.y) * (pt1.y - pt2.y);
	d = (int)(sqrt(d * 1.0) + 0.5);

	return d;
}

 
void ryuMakeGaussianKernal()
{  
	int i = 0, j = 0;
	int center_x = KSIZE/2, center_y = KSIZE/2;  

	float result[KSIZE][KSIZE];  
	float distance;  
	float temp;  
	float h_sum = 0;

	for(i = 0; i < KSIZE; i++) {  
		for(j = 0; j < KSIZE; j++) {  
			distance = (center_x - i) * (center_x - i)+ (center_y - j) * (center_y - j);  
			temp = exp((0-distance)/(2*sigma*sigma))/(2*PI*sigma*sigma);
			result[i][j] = temp; 
			h_sum += temp;   
		}  
	}  

	printf("sum = %.6f\n", h_sum);

	//归一化  
	for(i = 0; i < KSIZE; i++) {  
		for(j = 0; j < KSIZE; j++) {  
			result[i][j] /= h_sum;
			printf("%.6f  ",result[i][j]);  
		}  
		printf("\n");
	}

	// 定点化
	h_sum = 0;
	temp = 1 / result[0][0];
	for(i = 0; i < KSIZE; i++) {  
		for(j = 0; j < KSIZE; j++) {  
			result[i][j] *= temp;
			h_sum += result[i][j];
			printf("%2.4f  ",result[i][j]);  
		}  
		printf("\n");
	}

	printf("sum = %.4f\n", h_sum);

	return;
}  


RyuROI * ryuCreateROI( int xOffset, int yOffset, int width, int height )
{
	RyuROI * roi = 0;

	roi = (RyuROI *) malloc( sizeof(RyuROI) );
	if( !roi ) {
		printf("ERROR! Bad alloc_ptr of ryuCreateROI, roi = 0x%x\n", roi);
		return 0;
	}

	roi->xOffset = xOffset;
	roi->yOffset = yOffset;
	roi->width = width;
	roi->height = height;

	return roi;
}

void ryuSetImageROI( RyuImage* image, RyuRect rect )
{
	if( !image ) {
		printf( "ERROR! Invalid input of ryuSetImageROI, image = 0x%x\n", image );
		return;
	}

	rect.width += rect.x;
	rect.height += rect.y;

	rect.x = RYUMAX( rect.x, 0 );
	rect.y = RYUMAX( rect.y, 0 );
	rect.width = RYUMIN( rect.width, image->width );
	rect.height = RYUMIN( rect.height, image->height );

	rect.width -= rect.x;
	rect.height -= rect.y;

	if( image->roi ) {
		image->roi->xOffset = rect.x;
		image->roi->yOffset = rect.y;
		image->roi->width = rect.width;
		image->roi->height = rect.height;
	}
	else
		image->roi = ryuCreateROI( rect.x, rect.y, rect.width, rect.height );

	return;
}

RyuRect ryuGetImageROI( const RyuImage * image )
{
	RyuRect rect = ryuRect( 0, 0, 0, 0 );

	if( !image ) {
		printf( "ERROR! Invalid input of ryuGetImageROI, image = 0x%x\n", image );
		return rect;
	}

	if( image->roi )
		rect = ryuRect( image->roi->xOffset, image->roi->yOffset,
			image->roi->width, image->roi->height );
	else
		rect = ryuRect( 0, 0, image->width, image->height );

	return rect;
}

void cvResetImageROI( RyuImage * image )
{
	if( !image ) {
		printf( "ERROR! Invalid input of cvResetImageROI, image = 0x%x\n", image );
		return;
	}

	if( image->roi ) {
		free( image->roi );
		image->roi = 0;
	}
	return;
}


RyuImage * ryuCreateImageHeader( RyuSize size, int depth, int channels )
{
	RyuImage * img = 0;

	img = ( RyuImage * ) malloc( sizeof(RyuImage) );
	if( !img ) {
		printf("ERROR! Bad alloc_ptr of ryuCreateImageHeader, img = 0x%x\n", img);
		return 0;
	}

	ryuInitImageHeader( img, size, depth, channels );

	return img;
}

void * ryuInitImageHeader( RyuImage * image, RyuSize size, int depth, int channels )
{
	const char * colorModel, * channelSeq;

	if( !image ) {
		printf( "ERROR! Invalid input of ryuInitImageHeader, image = 0x%x\n", image );
		return 0;
	}

	memset( image, 0, sizeof(RyuImage) );

	if( size.width < 0 || size.height < 0 ) {
		printf( "ERROR! Bad input of ryuInitImageHeader, size.width = %d, size.height = %d\n", 
				size.width, size.height );
		return 0;
	}

	if( (depth != (int)RYU_DEPTH_8C && depth != (int)RYU_DEPTH_16S &&
		depth != (int)RYU_DEPTH_32N) || channels < 0 ) {
			printf( "ERROR! Bad input of ryuInitImageHeader, depth = %d, channels = %d\n", 
				depth, channels );
			return 0;
	}

	image->width = size.width;
	image->height = size.height;

	if( image->roi ) {
		image->roi->xOffset = image->roi->yOffset = 0;
		image->roi->width = size.width;
		image->roi->height = size.height;
	}

	image->channel = RYUMAX( channels, 1 );
	image->depth = depth;
	image->widthstep = image->width * image->channel * (depth >> 3);

	image->alloc_size = image->widthstep * image->height;

	return image;
}

RyuImage * ryuCreateImage(RyuSize size, int depth, int channels)
{
	RyuImage * img = ryuCreateImageHeader( size, depth, channels );
	if( !img ) {
		printf( "ERROR! Bad alloc_ptr of ryuCreateImage, img = 0x%x\n", img );
		return 0;
	}
	
	img->imagedata = (unsigned char *) malloc( img->alloc_size );
	if( !img->imagedata ) {
		printf( "ERROR! Bad alloc_ptr of ryuCreateImage, imagedata = 0x%x\n", img->imagedata );
		return 0;
	}

	return img;
}

void ryuReleaseImageHeader( RyuImage ** image )
{
	if( !image ) {
		printf( "ERROR! Invalid input of ryuReleaseImageHeader, image = 0x%x\n", image );
		return;
	}

	if( *image ) {
		RyuImage * img = *image;
		*image = 0;
		if( img->roi ) {
			free( img->roi );
			img->roi = 0;
		}
		free( img );
		img = 0;
	}
	return;
}

void ryuReleaseImage( RyuImage ** image )
{
	if( !image ) {
		printf( "ERROR! Invalid input of ryuReleaseImage, image = 0x%x\n", image );
		return;
	}

	if( *image ) {
		RyuImage * img = *image;
		*image = 0;
		free( img->imagedata );
		ryuReleaseImageHeader( &img );
	}
}

void * ryuSetImage( RyuImage * image, RyuSize size, int depth, int channels )
{
	int step = 0;
	if( !image ) {
		printf( "ERROR! Invalid input of ryuSetImage, image = 0x%x\n", image );
		return 0;
	}

	if( size.width < 0 || size.height < 0 ) {
		printf( "ERROR! Bad input of ryuSetImage, size.width = %d, size.height = %d\n", 
			size.width, size.height );
		return 0;
	}

	if( (depth != (int)RYU_DEPTH_8C && depth != (int)RYU_DEPTH_16S &&
		depth != (int)RYU_DEPTH_32N) || channels <= 0 ) {
			printf( "ERROR! Bad input of ryuSetImage, depth = %d, channels = %d\n", 
				depth, channels );
			return 0;
	}

	step = size.width * channels * (depth >> 3);
	if( step * size.height > image->alloc_size ) {
		printf( "ERROR! Bad value, too large size for imagedata, set size = %d\n", 
			step * size.height );
		return 0;
	}

	image->width = size.width;
	image->height = size.height;
	image->channel = channels;
	image->depth = depth;
	image->widthstep = step;
	
	return image;
}

void ryuZero( RyuImage * image )
{
	RyuRect rect = ryuRect( 0, 0, 0, 0 );

	int i = 0, base = 0, setcount = 0;

	if( !image ) {
		printf( "ERROR! Invalid input of ryuSetImage, image = 0x%x\n", image );
		return;
	}

	if( !image->imagedata ) {
		printf( "ERROR! Bad address of ryuSetImage, imagedata = 0x%x\n", image->imagedata );
		return;
	}

	if( !image->roi ) {
		memset( image->imagedata, 0, image->widthstep * image->height );
	} else {
		base = image->roi->yOffset * image->widthstep + image->roi->xOffset * image->channel * (image->depth>>3);
		setcount = image->roi->width* image->channel * (image->depth>>3);
		for( i = 0; i < rect.height; i++ ) {
			memset( image->imagedata+base, 0, setcount );
			base += image->widthstep;
		}
	}
	return;
}

int ryuGetPixel(RyuImage * image, RyuPoint pt)
{
	int val = 0;

	if( !image ) {
		printf( "ERROR! Invalid input of ryuSetImage, image = 0x%x\n", image );
		return -1;
	}

	if( !image->imagedata ) {
		printf( "ERROR! Bad address of ryuSetImage, imagedata = 0x%x\n", image->imagedata );
		return -2;
	}
	
	return ((int)(image->imagedata + pt.y * image->widthstep + pt.x));
}


int ryuSetPixel(RyuImage * image, RyuPoint pt, unsigned char val)
{
	if( !image ) {
		printf( "ERROR! Invalid input of ryuSetImage, image = 0x%x\n", image );
		return -1;
	}

	if( !image->imagedata ) {
		printf( "ERROR! Bad address of ryuSetImage, imagedata = 0x%x\n", image->imagedata );
		return -2;
	}

	*(image->imagedata + pt.y * image->widthstep + pt.x) = val;

	return 1;
}

// 调整图像大小，目前只具备缩小功能
int ryuResizeImage(RyuImage * img_in, RyuImage * img_out)
{
	double ZoomRatioW = 0.0, ZoomRatioH = 0.0;
	double ZoomAccW = 0.0, ZoomAccH = 0.0;

	int i = 0, j = 0;
	unsigned char * pIn = 0, * pOut = 0;
	unsigned char * pInL = 0, * pOutL = 0;

	if( !img_in || !img_out ) {
		printf( "ERROR! Invalid input of ryuSetImage, img_in = 0x%x, img_out = 0x%x\n", 
			img_in, img_out );
		return -1;
	}

	if( !img_in->imagedata || !img_out->imagedata ) {
		printf( "ERROR! Bad address of ryuSetImage, img_in data = 0x%x, img_out data = 0x%x\n",
			img_in->imagedata, img_out->imagedata );
		return -2;
	}

	ZoomRatioW = img_in->width * 1.0 / img_out->width;
	ZoomRatioH = img_in->height * 1.0 / img_out->height;

	for(j = 0; j < img_out->height; j++) {
		ZoomAccW = 0.0;
		pIn = img_in->imagedata + (int)(ZoomAccH+0.5) * img_in->widthstep;
		pOut = img_out->imagedata + j * img_out->widthstep;
		for(i = 0; i < img_out->width; i++) {
			pOut[i] = pIn[(int)(ZoomAccW+0.5)];
			ZoomAccW = ZoomAccW + ZoomRatioW;
		}
		ZoomAccH = ZoomAccH + ZoomRatioW;
	}

	return 1;
}

int ryuDilate(RyuImage * src, RyuImage * dst)
{
	int nh = src->height - 2; 
	int nw = src->width - 2;

	int i = 0, j = 0;
	unsigned char t = 0, t_l = 0, t_r = 0;

	unsigned char * pIn = src->imagedata;
	unsigned char * pOut = dst->imagedata;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b;
	unsigned char * poffset, * poffset_t, * poffset_b;

	// 初始化
	poffset   = loffset		= pIn;
	poffset_t = loffset_t	= pIn - src->width;
	poffset_b = loffset_b	= pIn + src->width;
	lOut	  = pOut;

	// 首行处理
	// -左上角
	t = RYUMAX(poffset[0], poffset_b[0]);
	t_r = RYUMAX(poffset[1], poffset_b[1]);
	*pOut = RYUMAX(t, t_r);
	t_l = t;
	t = t_r;
	poffset++;	
	poffset_b++;	
	pOut++;
	// -中间部分
	for(j = nw; j > 0; j--) {
		t_r = RYUMAX(poffset[1], poffset_b[1]);
		*pOut = RYUMAX(t_l, RYUMAX(t, t_r));
		t_l = t;
		t = t_r;
		poffset++;	
		poffset_b++;	
		pOut++;
	}
	// -右上角
	*pOut = RYUMAX(t_l, t);

	// 中间部分
	for(i = nh; i > 0; i--)	
	{
		loffset		+= src->width;
		loffset_t	+= src->width;
		loffset_b	+= src->width;
		poffset		= loffset;
		poffset_t	= loffset_t;
		poffset_b	= loffset_b;

		lOut += dst->width;
		pOut = lOut;

		// -行首
		t = RYUMAX(poffset_t[0], RYUMAX(poffset[0], poffset_b[0]));
		t_r = RYUMAX(poffset_t[1], RYUMAX(poffset[1], poffset_b[1]));
		*pOut = RYUMAX(t, t_r);
		t_l = t;
		t = t_r;
		poffset_t++;
		poffset++;	
		poffset_b++;	
		pOut++;
		// -中间部分
		for(j = nw; j > 0; j--)
		{
			t_r = RYUMAX(poffset_t[1], RYUMAX(poffset[1], poffset_b[1]));
			*pOut = RYUMAX(t_l, RYUMAX(t, t_r));
			t_l = t;
			t = t_r;
			poffset_t++;
			poffset++;	
			poffset_b++;	
			pOut++;
		}
		// -行末
		*pOut = RYUMAX(t_l, t);
	}

	// 末行处理
	poffset		= loffset;
	poffset_b	= loffset_b;
	lOut += dst->width;
	pOut = lOut;
	// -左下角
	t = RYUMAX(poffset[0], poffset_b[0]);
	t_r = RYUMAX(poffset[1], poffset_b[1]);
	*pOut = RYUMAX(t, t_r);
	t_l = t;
	t = t_r;
	poffset++;	
	poffset_b++;	
	pOut++;
	// -中间部分
	for(j = nw; j > 0; j--) {
		t_r = RYUMAX(poffset[1], poffset_b[1]);
		*pOut = RYUMAX(t_l, RYUMAX(t, t_r));
		t_l = t;
		t = t_r;
		poffset++;	
		poffset_b++;	
		pOut++;
	}
	// -右上角
	*pOut = RYUMAX(t_l, t);

	return 1;
}



/*
// 旋转卡壳算法
//Created by pritry
struct Point
{
	int x;
	int y;
};

Point cull[MAX];
int n;

int dis2(Point& a, Point& b)  //距离平方
{
	return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}
int Area2(int p, int q, int r)  //三角形面积的2倍
{
	return abs((cull[r].x - cull[p].x) * (cull[q].y - cull[p].y) - (cull[q].x - cull[p].x) * (cull[r].y - cull[p].y));
}

int NEXT(int p)
{
	return (p+1) % n;
}

int rotating_calipers()
{
	if(n == 2) return dis2(cull[0], cull[1]);
	int p0 = 0;
	int p = 0;
	int distance = 0;
	int q = NEXT(p);

	while(Area2(p, NEXT(p), NEXT(q)) > Area2(p, NEXT(p), q))
		q = NEXT(q);
	int q0 = q;
	distance = max(dis2(cull[p], cull[q]), distance);

	while(q != p0 || p != q0)
	{
		p = NEXT(p);
		if(q == p0 && p == q0) break;
		distance = max(dis2(cull[p], cull[q]), distance);
		while(Area2(p, NEXT(p), NEXT(q)) > Area2(p, NEXT(p), q))
		{
			q = NEXT(q);
			if(p != q0 || q != p0)
			{
				distance = max(dis2(cull[p], cull[q]), distance);
			}
			else
				return distance;
		}

		if(Area2(p, NEXT(p), NEXT(q)) == Area2(p, NEXT(p), q))
		{
			if(p != q0 || q != p0)
			{
				distance = max(dis2(cull[p], cull[NEXT(q)]), distance);
			}
			else
			{
				distance = max(dis2(cull[NEXT(p)], cull[q]), distance);
			}
		}
	}
	return distance;
}
*/
