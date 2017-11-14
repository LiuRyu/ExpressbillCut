#ifndef _OPENCV_DEBUGTOOL_H
#define _OPENCV_DEBUGTOOL_H

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "RyuCore.h"

#ifdef  EXPRESS_BARCODE_DETECT_LIBDLL
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllexport) 
#else
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllimport) 
#endif

#ifndef RYU_EXTERN_C
#  ifdef __cplusplus
#    define RYU_EXTERN_C extern "C"
#    define RYU_DEFAULT(val) = val
#  else
#    define RYU_EXTERN_C
#    define RYU_DEFAULT(val)
#  endif
#endif

/****************************************************************************************\
*									time operations								         *
\****************************************************************************************/
// ¼ÆÊ±Æ÷ÐòºÅ
enum {
	TIMER_SEQ_0 = 0,
	TIMER_SEQ_1 = 1,
	TIMER_SEQ_2 = 2,
	TIMER_SEQ_3 = 3,
	TIMER_SEQ_4 = 4,
	TIMER_SEQ_5 = 5,
	TIMER_SEQ_6 = 6,
	TIMER_SEQ_7 = 7,
	TIMER_SEQ_8 = 8,
	TIMER_SEQ_9 = 9
};


EXPRESS_BARCODE_DETECT_LIBDLL void uc2IplImageGray(unsigned char* src,IplImage *dst);

EXPRESS_BARCODE_DETECT_LIBDLL void Ipl2ucImageGray(IplImage* src, unsigned char *dst);

void ZoomPlusImageSimple( unsigned char * src, int width, int height, int multiple, 
		unsigned char * dst, int * zoom_width, int * zoom_height);

void GetImageRoiData( unsigned char * src, int width, int height, RyuPoint * corner,
		unsigned char * dst, int * dst_width, int * dst_height, RyuPoint * offset);

/****************************************************************************************\
*									time operations								         *
\****************************************************************************************/
EXPRESS_BARCODE_DETECT_LIBDLL void ryuTimerStart(int seq RYU_DEFAULT(TIMER_SEQ_0));

EXPRESS_BARCODE_DETECT_LIBDLL long ryuTimerStop(int seq RYU_DEFAULT(TIMER_SEQ_0));

/****************************************************************************************\
*									print operations								     *
\****************************************************************************************/
/* 
 * convert a long val to thousand-type val string. 
 * eg. 1589746 => 1,589,476
 */
EXPRESS_BARCODE_DETECT_LIBDLL void ryuThousandType(long val, char (&Dest)[32U]);

#endif





