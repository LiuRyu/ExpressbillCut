#include "stdafx.h"
 
#include <stdio.h>  
#include <stdlib.h> 
#include <string.h> 
#include <windows.h>

#include <time.h> 
#include "OpenCv_debugTools.h"

LARGE_INTEGER s_time[10] = {{0}}, e_time[10] = {{0}};

void uc2IplImageGray(unsigned char* src,IplImage *dst)
{
	for (int y=0;y<dst->height;y++)
	{
		for (int x=0;x<dst->width;x++)
		{
			((unsigned char *)(dst->imageData + dst->widthStep*y))[x]=src[y*dst->width+x];
		}
	}
}

void Ipl2ucImageGray(IplImage* src, unsigned char *dst)
{
	for (int y=0;y<src->height;y++)
	{
		for (int x=0;x<src->width;x++)
		{
			dst[y*src->width+x] = ((unsigned char *)(src->imageData + src->widthStep*y))[x];
		}
	}
}

void GetImageRoiData( unsigned char * src, int width, int height, RyuPoint * corner,
		unsigned char * dst, int * dst_width, int * dst_height, RyuPoint * offset)
{
	int i = 0, j = 0;
	int LTRB[4], LTRB_data[4];
	int dst_wid = 0, dst_hei = 0;
	unsigned char * pSrc = 0, * pDst = 0;

	LTRB[0] = RYUMIN( RYUMIN(corner[0].x, corner[1].x), RYUMIN(corner[2].x, corner[3].x) );
	LTRB[1] = RYUMIN( RYUMIN(corner[0].y, corner[1].y), RYUMIN(corner[2].y, corner[3].y) );
	LTRB[2] = RYUMAX( RYUMAX(corner[0].x, corner[1].x), RYUMAX(corner[2].x, corner[3].x) );
	LTRB[3] = RYUMAX( RYUMAX(corner[0].y, corner[1].y), RYUMAX(corner[2].y, corner[3].y) );

	LTRB_data[0] = RYUMAX( 0, RYUMIN(width-1, LTRB[0]) );
	LTRB_data[1] = RYUMAX( 0, RYUMIN(height-1, LTRB[1]) );
	LTRB_data[2] = RYUMAX( 0, RYUMIN(width-1, LTRB[2]) );
	LTRB_data[3] = RYUMAX( 0, RYUMIN(height-1, LTRB[3]) );

	if( LTRB_data[0] >= LTRB_data[2] || LTRB_data[1] >= LTRB_data[3] ) {
		printf("ERROR! Bad corner of GetImageRoiData\n");
		return;
	}

	dst_wid = LTRB[2] - LTRB[0] + 1;
	dst_hei = LTRB[3] - LTRB[1] + 1;
	memset( dst, 0, dst_wid * dst_hei * sizeof(unsigned char) );

	for(j = 0; j < LTRB_data[3]-LTRB_data[1]+1; j++) {
		pSrc = src + (j+LTRB_data[1]) * width + LTRB_data[0];
		pDst = dst + (j+LTRB_data[1]-LTRB[1]) * dst_wid + (LTRB_data[0]-LTRB[0]);
		for(i = 0; i < LTRB_data[2]-LTRB_data[0]+1; i++) {
			*(pDst++) = *(pSrc++);
		}
	}

	*dst_width = dst_wid;
	*dst_height = dst_hei;
	offset->x = LTRB[0];
	offset->y = LTRB[1];
}

void ZoomPlusImageSimple( unsigned char * src, int width, int height, int multiple, 
		unsigned char * dst, int * zoom_width, int * zoom_height)
{
	unsigned char * pSrc = 0, * pDst = 0;
	int dstWidth = width * multiple;
	int dstHeight = height * multiple;

	for( int y = 0; y < height; y++ ) {
		pSrc = src + y * width;
		pDst = dst + y * multiple * dstWidth;
		for( int x = 0; x < width; x++ ) {
			for(int n = 0; n < multiple; n++) {
				*(pDst++) = *pSrc;
			}
			pSrc++;
		}
		for(int n = 0; n < multiple - 1; n++) {
			memcpy( pDst, pDst-dstWidth, dstWidth * sizeof(unsigned char));
			pDst += dstWidth;
		}
	}
	*zoom_width = dstWidth;
	*zoom_height = dstHeight;
}

void ryuTimerStart(int seq) 
{
	if(TIMER_SEQ_0 > seq || TIMER_SEQ_9 < seq) {
#if _PRINT_GENERAL_UTILITIES_
		printf("Error! Invalid input. [GeneralUti../time../TimerStart]. seq=%d\n", seq);
#endif
		return;
	}
	LARGE_INTEGER frequency = {0};
	QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
	QueryPerformanceCounter((LARGE_INTEGER *)&s_time[seq]);
	return;
}

long ryuTimerStop(int seq)
{
	long TimeCost = 0;

	if(TIMER_SEQ_0 > seq || TIMER_SEQ_9 < seq) {
#if _PRINT_GENERAL_UTILITIES_
		printf("Error! Invalid input. [GeneralUti../time../TimerStart]. seq=%d\n", seq);
#endif
		return 0;
	}
	QueryPerformanceCounter((LARGE_INTEGER*) &e_time[seq]);

	LARGE_INTEGER frequency = {0};
	QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
	TimeCost = 1000000 * (e_time[seq].QuadPart - s_time[seq].QuadPart) / frequency.QuadPart;

	return TimeCost;
}

void ryuThousandType(long val, char (&Dest)[32U])
{
	if(NULL == Dest) {
#if _PRINT_GENERAL_UTILITIES_
		printf("Error! Invalid input. [GeneralUti../print../ThousandType]. Dest=0x%x\n", Dest);
#endif
		return;
	}
	char stmp[32] = "0";
	int i = 0, tic = 0, len = 0;
	sprintf_s(stmp, "%d", val);

	for(i = strlen(stmp) - 1; i >= 0; i--) {
		Dest[len++] = stmp[i];
		if(!((++tic) % 3))
			Dest[len++] = ',';
	}
	if(',' == Dest[len-1]) {
		Dest[len-1] = '\0';
		len -= 1;
	}
	else
		Dest[len] = '\0';

	for(i = 0; i < (len >> 1); i++) {
		char c = Dest[i];
		Dest[i] = Dest[len-i-1];
		Dest[len-i-1] = c;
	}
	return;
}

