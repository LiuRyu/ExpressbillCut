#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ImageRotation.h"

#ifdef	_DEBUG_
#ifdef  _DEBUG_ROTATE
#include "OpenCv_debugTools.h"
#endif
#endif

const short icv8x32fTab_cv[] =
{
	-256, -255, -254, -253, -252, -251, -250, -249, -248, -247, -246, -245, -244, -243, -242, -241,
	-240, -239, -238, -237, -236, -235, -234, -233, -232, -231, -230, -229, -228, -227, -226, -225,
	-224, -223, -222, -221, -220, -219, -218, -217, -216, -215, -214, -213, -212, -211, -210, -209,
	-208, -207, -206, -205, -204, -203, -202, -201, -200, -199, -198, -197, -196, -195, -194, -193,
	-192, -191, -190, -189, -188, -187, -186, -185, -184, -183, -182, -181, -180, -179, -178, -177,
	-176, -175, -174, -173, -172, -171, -170, -169, -168, -167, -166, -165, -164, -163, -162, -161,
	-160, -159, -158, -157, -156, -155, -154, -153, -152, -151, -150, -149, -148, -147, -146, -145,
	-144, -143, -142, -141, -140, -139, -138, -137, -136, -135, -134, -133, -132, -131, -130, -129,
	-128, -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, -117, -116, -115, -114, -113,
	-112, -111, -110, -109, -108, -107, -106, -105, -104, -103, -102, -101, -100,  -99,  -98,  -97,
	-96,  -95,  -94,  -93,  -92,  -91,  -90,  -89,	-88,  -87,  -86,  -85,  -84,  -83,  -82,  -81,
	-80,  -79,  -78,  -77,  -76,  -75,  -74,  -73,	-72,  -71,  -70,  -69,  -68,  -67,  -66,  -65,
	-64,  -63,  -62,  -61,  -60,  -59,  -58,  -57,	-56,  -55,  -54,  -53,  -52,  -51,  -50,  -49,
	-48,  -47,  -46,  -45,  -44,  -43,  -42,  -41,	-40,  -39,  -38,  -37,  -36,  -35,  -34,  -33,
	-32,  -31,  -30,  -29,  -28,  -27,  -26,  -25,	-24,  -23,  -22,  -21,  -20,  -19,  -18,  -17,
	-16,  -15,  -14,  -13,  -12,  -11,  -10,   -9,	-8,   -7,   -6,   -5,   -4,   -3,   -2,   -1,
	0,    1,    2,    3,    4,    5,    6,    7,	8,    9,   10,   11,   12,   13,   14,   15,
	16,   17,   18,   19,   20,   21,   22,   23,	24,   25,   26,   27,   28,   29,   30,   31,
	32,   33,   34,   35,   36,   37,   38,   39,	40,   41,   42,   43,   44,   45,   46,   47,
	48,   49,   50,   51,   52,   53,   54,   55,	56,   57,   58,   59,   60,   61,   62,   63,
	64,   65,   66,   67,   68,   69,   70,   71,	72,   73,   74,   75,   76,   77,   78,   79,
	80,   81,   82,   83,   84,   85,   86,   87,	88,   89,   90,   91,   92,   93,   94,   95,
	96,   97,   98,   99,  100,  101,  102,  103,	104,  105,  106,  107,  108,  109,  110,  111,
	112,  113,  114,  115,  116,  117,  118,  119,	120,  121,  122,  123,  124,  125,  126,  127,
	128,  129,  130,  131,  132,  133,  134,  135,	136,  137,  138,  139,  140,  141,  142,  143,
	144,  145,  146,  147,  148,  149,  150,  151,	152,  153,  154,  155,  156,  157,  158,  159,
	160,  161,  162,  163,  164,  165,  166,  167,	168,  169,  170,  171,  172,  173,  174,  175,
	176,  177,  178,  179,  180,  181,  182,  183,	184,  185,  186,  187,  188,  189,  190,  191,
	192,  193,  194,  195,  196,  197,  198,  199,	200,  201,  202,  203,  204,  205,  206,  207,
	208,  209,  210,  211,  212,  213,  214,  215,	216,  217,  218,  219,  220,  221,  222,  223,
	224,  225,  226,  227,  228,  229,  230,  231,	232,  233,  234,  235,  236,  237,  238,  239,
	240,  241,  242,  243,  244,  245,  246,  247,	248,  249,  250,  251,  252,  253,  254,  255,
	256,  257,  258,  259,  260,  261,  262,  263,	264,  265,  266,  267,  268,  269,  270,  271,
	272,  273,  274,  275,  276,  277,  278,  279,	280,  281,  282,  283,  284,  285,  286,  287,
	288,  289,  290,  291,  292,  293,  294,  295,	296,  297,  298,  299,  300,  301,  302,  303,
	304,  305,  306,  307,  308,  309,  310,  311,	312,  313,  314,  315,  316,  317,  318,  319,
	320,  321,  322,  323,  324,  325,  326,  327,	328,  329,  330,  331,  332,  333,  334,  335,
	336,  337,  338,  339,  340,  341,  342,  343,	344,  345,  346,  347,  348,  349,  350,  351,
	352,  353,  354,  355,  356,  357,  358,  359,	360,  361,  362,  363,  364,  365,  366,  367,
	368,  369,  370,  371,  372,  373,  374,  375,	376,  377,  378,  379,  380,  381,  382,  383,
	384,  385,  386,  387,  388,  389,  390,  391,	392,  393,  394,  395,  396,  397,  398,  399,
	400,  401,  402,  403,  404,  405,  406,  407,	408,  409,  410,  411,  412,  413,  414,  415,
	416,  417,  418,  419,  420,  421,  422,  423,	424,  425,  426,  427,  428,  429,  430,  431,
	432,  433,  434,  435,  436,  437,  438,  439,	440,  441,  442,  443,  444,  445,  446,  447,
	448,  449,  450,  451,  452,  453,  454,  455,	456,  457,  458,  459,  460,  461,  462,  463,
	464,  465,  466,  467,  468,  469,  470,  471,	472,  473,  474,  475,  476,  477,  478,  479,
	480,  481,  482,  483,  484,  485,  486,  487,	488,  489,  490,  491,  492,  493,  494,  495,
	496,  497,  498,  499,  500,  501,  502,  503,	504,  505,  506,  507,  508,  509,  510,  511,
};

#define MY_SHIFT_BIT 10
#define MY_SHIFT_BIT_HALF 9
#define MY_SHIFT_ALL 1024
#define MY_SHIFT_HALF 512

static int gnImgRttMaxWidth = 0;
static int gnImgRttMaxHeight = 0;
static int gnImgRttMaxLineSize = 0;

static int * gpnImgRttBideL = 0;
static int * gpnImgRttBideR = 0;
static int * gpnImgRttLinePts[4] = {0};

unsigned char * gucImgRttZoom = 0;
unsigned char * gucImgRttImage = 0;
unsigned char * gucImgRttImage2 = 0;

static int gnImgRttInitFlag=0;

static int GetImageRoiZoomInData( unsigned char * src, int width, int height, RyuPoint * corner, int zoom, 
	unsigned char * dst, int * dst_width, int * dst_height, RyuPoint * offset);

static void GetQuadrangleSubPix_8u_C1R(unsigned char *const src,const short sSrcImgWidth,const short sSrcImgHeight,
				const int nStartY,const int nEndY,unsigned char *const dst,const short sDstImgWidth,const short sDstImgHeight,
				const int *const matrix);
static void IPA_findTwoPointLineAllPts(const int nPt0,const int nPt1,const int nLineNum,short * const sPtsNum);

int RotateImage(unsigned char* ucSrcImg, short sSrcImgWidth, short sSrcImgHeight,
				 RyuPoint * corner, int cAngle, int cZoom, short * usDstW,short * usDstH)
{
	int status = 0;
	int nMatrix[6];
	int srcPts[4];
	short sPtsNum[4]={0},i,j,sX,sY,sX1,sY1,sMaxY=0,sMinY=0x7fff,sMax1,sMax2,sW=sSrcImgWidth-1,sH=sSrcImgHeight-1;
	int nZoomW = 0, nZoomH = 0;
	RyuPoint offset, zoomCorner[4];

	if(gnImgRttInitFlag==0 || sSrcImgHeight>gnImgRttMaxHeight || sSrcImgWidth>gnImgRttMaxWidth 
		|| cAngle < 0 || cAngle > 360)
	{
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of RotateImage\n");
#endif
		return -1;
	}

	/*****************************0、取出并放大条码区域图像******************************/
	status = GetImageRoiZoomInData(ucSrcImg, (int)sSrcImgWidth, (int)sSrcImgHeight, corner, cZoom, 
				gucImgRttZoom, &nZoomW, &nZoomH, &offset);
	if(1 != status) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of GetImageRoiZoomData, return=%d\n", status);
#endif
		return 0;
	}

	for(i = 0; i < 4; i++) {
		zoomCorner[i].x = (corner[i].x - offset.x) * cZoom;
		zoomCorner[i].y = (corner[i].y - offset.y) * cZoom;
	}
	srcPts[0] = (zoomCorner[0].x << 16) | (zoomCorner[0].y & 0xffff);
	srcPts[1] = (zoomCorner[2].x << 16) | (zoomCorner[2].y & 0xffff);
	srcPts[2] = (zoomCorner[3].x << 16) | (zoomCorner[3].y & 0xffff);
	srcPts[3] = (zoomCorner[1].x << 16) | (zoomCorner[1].y & 0xffff);
	sW = nZoomW - 1;
	sH = nZoomH - 1;
#ifdef	_DEBUG_
#ifdef  _DEBUG_ROTATE
	IplImage * iplZoomSlice = cvCreateImage(cvSize(nZoomW, nZoomH), 8, 1);
	IplImage * iplZoomSlice3C = cvCreateImage(cvSize(nZoomW, nZoomH), 8, 3);
	uc2IplImageGray(gucImgRttZoom, iplZoomSlice);
	cvCvtColor(iplZoomSlice, iplZoomSlice3C, CV_GRAY2BGR);
	for(i = 0; i < 4; i++) {
		cvCircle(iplZoomSlice3C, cvPoint(zoomCorner[i].x, zoomCorner[i].y), 3, CV_RGB(0,0,255), CV_FILLED);
	}
	cvNamedWindow("ZoomSlice");
	cvShowImage("ZoomSlice", iplZoomSlice3C);
	cvWaitKey();
	cvReleaseImage(&iplZoomSlice);
	cvReleaseImage(&iplZoomSlice3C);
#endif
#endif

	/*****************************1、根据四点，确定有效区域边界******************************/
	for (i=sH;i>=0;i--)
	{
		gpnImgRttBideL[i]=0x7fffffff;
		gpnImgRttBideR[i]=0;
	}
	for (i=3; i>=0; i--)
	{
		IPA_findTwoPointLineAllPts(srcPts[i],srcPts[(i+1)%4],i,&sPtsNum[i]);	//点存的方式是"xy"，即sX在高16位，sY在低16位
		for (j=sPtsNum[i]-1;j>=0;j--)
		{
			sX=gpnImgRttLinePts[i][j]>>16;
			sY=gpnImgRttLinePts[i][j]&0xffff;
			sX=sX<0?0:(sX>sW?sW:sX);
			sY=sY<0?0:(sY>sH?sH:sY);	
			gpnImgRttBideL[sY]=(sX<<MY_SHIFT_BIT)<gpnImgRttBideL[sY]?(sX<<MY_SHIFT_BIT):gpnImgRttBideL[sY];
			gpnImgRttBideR[sY]=(sX<<MY_SHIFT_BIT)>gpnImgRttBideR[sY]?(sX<<MY_SHIFT_BIT):gpnImgRttBideR[sY];
			sMaxY=sY>sMaxY?sY:sMaxY;
			sMinY=sY<sMinY?sY:sMinY;
		}
	}

	/******************************2、确定仿射矩阵和目标图像大小*******************************************/
	// Matrix nMatrix looks like:
	//
	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
	// [ m3  m4  m5 ]       [ A21  A22   b2 ]
	sX=srcPts[0]>>16;
	sX1=srcPts[2]>>16;
	sY=srcPts[0]&0xffff;
	sY1=srcPts[2]&0xffff;
	nMatrix[0] = ryuCosShift(cAngle);
	nMatrix[1] = -ryuSinShift(cAngle);
	nMatrix[3] = -nMatrix[1];
	nMatrix[4] = nMatrix[0];
	nMatrix[2] =(sX+sX1)<<MY_SHIFT_BIT_HALF;
	nMatrix[5] =(sY+sY1)<<MY_SHIFT_BIT_HALF;

	//有效区域旋转后图像大小
	sMax1=sPtsNum[0]>sPtsNum[2]?sPtsNum[0]:sPtsNum[2];
	sMax2=sPtsNum[1]>sPtsNum[3]?sPtsNum[1]:sPtsNum[3];
	sMax1>sMax2?(sW=sMax1,sH=sMax2):(sW=sMax2,sH=sMax1);

	if(sW > gnImgRttMaxLineSize || sH > gnImgRttMaxLineSize
//		||sW > sSrcImgWidth || sH > sSrcImgHeight 
		|| sW <= 0 || sH <= 0) {
			*usDstW=sW;
			*usDstH=sH;
			return 0;
	}

	/******************************3、进行旋转*******************************/
	memset(gucImgRttImage,0,sW*sH);
// 	GetQuadrangleSubPix_8u_C1R(ucSrcImg,sSrcImgWidth,sSrcImgHeight,sMinY<<MY_SHIFT_BIT,
// 		sMaxY<<MY_SHIFT_BIT,gucImgRttImage,sW,sH,nMatrix);
	GetQuadrangleSubPix_8u_C1R(gucImgRttZoom,(short)nZoomW,(short)nZoomH,sMinY<<MY_SHIFT_BIT,
		sMaxY<<MY_SHIFT_BIT,gucImgRttImage,sW,sH,nMatrix);

	*usDstW=sW;
	*usDstH=sH;

#ifdef	_DEBUG_
#ifdef  _DEBUG_ROTATE
	IplImage * iplRotateSlice = cvCreateImage(cvSize(sW, sH), 8, 1);
	uc2IplImageGray(gucImgRttImage, iplRotateSlice);
	cvNamedWindow("RotateSlice");
	cvShowImage("RotateSlice", iplRotateSlice);
	cvWaitKey();
	cvReleaseImage(&iplRotateSlice);
#endif
#endif

	return 1;
}

unsigned char * GetRotateImage()
{
	return gucImgRttImage;
}

int GetImageRoiZoomInData( unsigned char * src, int width, int height, RyuPoint * corner, int zoom, 
	unsigned char * dst, int * dst_width, int * dst_height, RyuPoint * offset)
{
	int i = 0, j = 0, k = 0;
	int LTRB[4], LTRB_data[4];
	int dst_wid = 0, dst_hei = 0;
	int zoom_wid = 0, zoom_hei = 0;
	unsigned char * pSrc = 0, * pDst = 0;
	unsigned char * pSrcL = 0, * pDstL = 0;

	LTRB[0] = RYUMIN( RYUMIN(corner[0].x, corner[1].x), RYUMIN(corner[2].x, corner[3].x) );
	LTRB[1] = RYUMIN( RYUMIN(corner[0].y, corner[1].y), RYUMIN(corner[2].y, corner[3].y) );
	LTRB[2] = RYUMAX( RYUMAX(corner[0].x, corner[1].x), RYUMAX(corner[2].x, corner[3].x) );
	LTRB[3] = RYUMAX( RYUMAX(corner[0].y, corner[1].y), RYUMAX(corner[2].y, corner[3].y) );

	LTRB_data[0] = RYUMAX( 0, RYUMIN(width-1, LTRB[0]) );
	LTRB_data[1] = RYUMAX( 0, RYUMIN(height-1, LTRB[1]) );
	LTRB_data[2] = RYUMAX( 0, RYUMIN(width-1, LTRB[2]) );
	LTRB_data[3] = RYUMAX( 0, RYUMIN(height-1, LTRB[3]) );

	if( LTRB_data[0] >= LTRB_data[2] || LTRB_data[1] >= LTRB_data[3] ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Bad corner of GetImageRoiData\n");
#endif
		return -1;
	}

	if(zoom <= 0) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Bad zoom of GetImageRoiData\n");
#endif
		return -1;
	}

	dst_wid = LTRB_data[2] - LTRB_data[0] + 1;
	dst_hei = LTRB_data[3] - LTRB_data[1] + 1;
	zoom_wid = dst_wid * zoom;
	zoom_hei = dst_hei * zoom;

	if(zoom_wid > gnImgRttMaxLineSize || zoom_hei > gnImgRttMaxLineSize) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Too large size of zoomed image in GetImageRoiData, width=%d, height=%d\n",
			zoom_wid, zoom_hei);
#endif
		return 0;
	}

	pSrcL = src + LTRB_data[1] * width + LTRB_data[0];
	pDstL = dst;
	for(j = 0; j < dst_hei; j++) {
		pSrc = pSrcL;
		pDst = pDstL;
		for(i = 0; i < dst_wid; i++) {
			for(k = 0; k < zoom; k++) {
				*(pDst++) = *pSrc;
			}
			pSrc++;
		}
		for(k = 0; k < zoom-1; k++) {
			memcpy(pDstL+zoom_wid, pDstL, zoom_wid * sizeof(unsigned char));
			pDstL += zoom_wid;
		}
		pSrcL += width;
		pDstL += zoom_wid;
	}

	*dst_width = zoom_wid;
	*dst_height = zoom_hei;
	offset->x = LTRB_data[0];
	offset->y = LTRB_data[1];

#ifdef	_DEBUG_
#ifdef  _DEBUG_ROTATE
	IplImage * iplImage = cvCreateImage(cvSize(width, height), 8, 1);
	IplImage * iplImageRoi = cvCreateImage(cvSize(dst_wid, dst_hei), 8, 1);
	uc2IplImageGray(src, iplImage);
	cvSetImageROI(iplImage, cvRect(LTRB_data[0], LTRB_data[1], dst_wid, dst_hei));
	cvCopy(iplImage, iplImageRoi);
	cvNamedWindow("SrcSlice");
	cvShowImage("SrcSlice", iplImageRoi);
	//cvWaitKey();
	cvReleaseImage(&iplImage);
	cvReleaseImage(&iplImageRoi);
#endif
#endif

	return 1;
}

int ImageRotation_init(int max_wid, int max_hei)
{
	if(gnImgRttInitFlag)
		return -9;

	gnImgRttMaxWidth = max_wid;
	gnImgRttMaxHeight = max_hei;
	gnImgRttMaxLineSize = (int)sqrt(max_wid*max_wid*1.0 + max_hei*max_hei*1.0 + 0.5);

	if(gnImgRttMaxWidth <= 0 || gnImgRttMaxHeight <= 0 || gnImgRttMaxLineSize <= 0) {
		return -1;
	}

	gucImgRttZoom = (unsigned char *) malloc(gnImgRttMaxLineSize * gnImgRttMaxLineSize * sizeof(unsigned char));
	gucImgRttImage = (unsigned char *) malloc(gnImgRttMaxLineSize * gnImgRttMaxLineSize * sizeof(unsigned char));
	gucImgRttImage2 = (unsigned char *) malloc(gnImgRttMaxLineSize * gnImgRttMaxLineSize * sizeof(unsigned char));

	gpnImgRttBideL= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));
	gpnImgRttBideR= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));
	gpnImgRttLinePts[0]= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));
	gpnImgRttLinePts[1]= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));
	gpnImgRttLinePts[2]= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));
	gpnImgRttLinePts[3]= (int *)malloc(gnImgRttMaxLineSize*sizeof(int));

	gnImgRttInitFlag=1;

	return 1;
}

void ImageRotation_release()
{
	int i = 0;

	gnImgRttInitFlag=0;

	if(gpnImgRttBideL) {
		free(gpnImgRttBideL);
		gpnImgRttBideL=0;
	}
	if(gpnImgRttBideR) {
		free(gpnImgRttBideR);
		gpnImgRttBideR=0;
	}
	for(i = 0; i < 4; i++) {
		if(gpnImgRttLinePts[i]) {
			free(gpnImgRttLinePts[i]);
			gpnImgRttLinePts[i]=0;
		}
	}
	if(gucImgRttZoom) {
		free(gucImgRttZoom);
		gucImgRttZoom=0;
	}
	if(gucImgRttImage) {
		free(gucImgRttImage);
		gucImgRttImage=0;
	}
	if(gucImgRttImage2) {
		free(gucImgRttImage2);
		gucImgRttImage2=0;
	}
}


int IPA_sub2(const int a,const int b)
{
	short ah, al, bh, bl;

	ah = a >> 16;
	al = a & 0xFFFF;

	bh = b >> 16;
	bl = b & 0xFFFF;

	ah -= bh;
	al -= bl;

	return (int)((unsigned int)(ah << 16) | ((unsigned int)(al&0xFFFF)));
}

int IPA_ext(const int a,const int b,const int c)
{
	return (a << b) >> c;
}

int IPA_max2(const int a,const int b)
{
	short ah, al, bh, bl;

	ah = a >> 16;
	al = a & 0xFFFF;

	bh = b >> 16;
	bl = b & 0xFFFF;

	ah =ah < bh? bh:ah;
	al =al < bl? bl :al;

	return (int)((unsigned int)(ah << 16) |(unsigned int)(al&0xffff));
}

int IPA_min2(const int a,const int b)
{
	short ah, al, bh, bl;

	ah = a >> 16;
	al = a & 0xFFFF;

	bh = b >> 16;
	bl = b & 0xFFFF;

	ah =ah > bh? bh:ah;
	al =al > bl? bl:al;

	return (int)((unsigned int)(ah << 16) |(unsigned int)(al&0xffff));
}

char IPA_cmpgt2(const int a,const int b)
{
	short ah, al, bh, bl;
	char rt = 0;

	ah = a >> 16;
	al = a & 0xFFFF;

	bh = b >> 16;
	bl = b & 0xFFFF;


	rt=ah>bh?rt|2:rt;
	rt=al>bl?rt|1:rt;
	return rt;
}
//找两点连线上的所有点
void IPA_findTwoPointLineAllPts(const int nPt0,const int nPt1,const int nLineNum,short * const sPtsNum)
{
	int nK,nDXY,nDXY_abs,nStartPt,nDX,nDY;
	short i,j,sDX_abs,sDY_abs,sNum=0,sDx,sDy;

	int mina,maxa;
	mina=IPA_min2(nPt0, nPt1);
	maxa=IPA_max2(nPt0, nPt1);
	nDXY_abs = IPA_sub2(maxa,mina);//带绝对值
	sDY_abs = nDXY_abs & 0xFFFF;
	sDX_abs = nDXY_abs >> 16;

	nDXY = IPA_sub2(nPt1, nPt0);//直接相减
	nDY = IPA_ext(nDXY, 16, 16);//低16位
	nDX = IPA_ext(nDXY, 0, 16);//高16位

	//两点是同一点
	if(sDX_abs == sDY_abs && sDX_abs == 0)
	{
		gpnImgRttLinePts[nLineNum][sNum++]=nPt0;
	}
	else if(sDX_abs > sDY_abs)
	{
		nK = (nDY&0x80000000)|(((nDY<<8)+(nDX>>1))/nDX);
		nStartPt=IPA_cmpgt2(nPt1, nPt0)&2?nPt0:nPt1;
		sDx=nStartPt>>16;
		sDy=nStartPt&0xffff;
		for(i = sDX_abs; i >= 0; i --)
		{
			j = nK * i >> 8;	
			gpnImgRttLinePts[nLineNum][sNum++]=((sDx+i)<<16)|((sDy+j)&0xffff);
		}
	}
	else
	{
		nK =(nDX&0x80000000)|(((nDX<<8)+(nDY>>1))/nDY);
		nStartPt=IPA_cmpgt2(nPt1, nPt0)&1?nPt0:nPt1;
		sDx=nStartPt>>16;
		sDy=nStartPt&0xffff;
		for(j = sDY_abs; j >= 0; j --)
		{
			i = nK * j >> 8;
			gpnImgRttLinePts[nLineNum][sNum++]=((sDx+i)<<16)|((sDy+j)&0xffff);
		}
	}
	*sPtsNum=sNum;
}
//对灰度图进行旋转
void GetQuadrangleSubPix_8u_C1R(unsigned char *const src,const short sSrcImgWidth,const short sSrcImgHeight,const int nStartY,const int nEndY,unsigned char *const dst,const short sDstImgWidth,const short sDstImgHeight,const int *const matrix)
{
	int dist=0,xs,ys,xsF,ysF,xe,ye,xeF,yeF,A11,A12,A13,A21,A22,A23,nLeft,nRight,a,b,a1,p0, p1,ixs,iys,nX,nY,nW1Dst,nH1Dst,nW1Src,nH1Src;
	unsigned int usW,usH;
	short x,y;
	unsigned char *ptr=0, *ptr0=0, *ptr1=0;
	nW1Dst=sDstImgWidth-1;
	nH1Dst=sDstImgHeight-1;
	nW1Src=sSrcImgWidth-1;
	nH1Src=sSrcImgHeight-1;
	nX = nW1Dst<<MY_SHIFT_BIT_HALF;
	nY = nH1Dst<<MY_SHIFT_BIT_HALF;
	A11 = matrix[0];
	A12 = matrix[1];
	A13 = matrix[2]-(A11*nX+A12*nY>>MY_SHIFT_BIT);
	A21 = matrix[3];
	A22 = matrix[4];
	A23 = matrix[5]-(A21*nX+A22*nY>>MY_SHIFT_BIT);
	usW=(unsigned)(sSrcImgWidth-3<<MY_SHIFT_BIT);
	usH=(unsigned)(sSrcImgHeight-3<<MY_SHIFT_BIT);

	for( y = 0; y < sDstImgHeight; y++, dist += sDstImgWidth )
	{
		xs = A12*y + A13;
		ys = A22*y + A23;
		xe = A11*nW1Dst+ A12*y + A13;
		ye = A21*nW1Dst + A22*y + A23;
		xsF=xs-xs%MY_SHIFT_ALL-MY_SHIFT_ALL;
		ysF=ys-ys%MY_SHIFT_ALL-MY_SHIFT_ALL;
		xeF=xe-xe%MY_SHIFT_ALL-MY_SHIFT_ALL;
		yeF=ye-ye%MY_SHIFT_ALL-MY_SHIFT_ALL;
		if( (unsigned)xsF< usW &&
			(unsigned)ysF < usH &&
			(unsigned)xeF < usW &&
			(unsigned)yeF < usH)
		{
			for( x = 0; x < sDstImgWidth; x++ )
			{
				ixs =xs-xs%MY_SHIFT_ALL;
				iys =ys-ys%MY_SHIFT_ALL;
				if (iys>=nStartY&&iys<=nEndY)
				{
					nX=ixs>>MY_SHIFT_BIT;
					nY=iys>>MY_SHIFT_BIT;
					nLeft=gpnImgRttBideL[nY];
					nRight=gpnImgRttBideR[nY];
					if (ixs>=nLeft&&ixs<=nRight)
					{			
						ptr = src + sSrcImgWidth*nY + nX;     
						a = xs - ixs;
						b = ys - iys;
						a1 =MY_SHIFT_ALL - a;
						p0 = icv8x32fTab_cv[ptr[0]+256]*a1 + icv8x32fTab_cv[ptr[1]+256]*a;
						p1 = icv8x32fTab_cv[ptr[sSrcImgWidth]+256]*a1 + icv8x32fTab_cv[ptr[sSrcImgWidth+1]+256]*a;

						dst[dist+x] = (unsigned char)((((p0<<MY_SHIFT_BIT) + b * (p1 - p0)>>MY_SHIFT_BIT)+MY_SHIFT_HALF)>>MY_SHIFT_BIT);
					}
				}
				xs += A11;
				ys += A21;

			}
		}
		else
		{
			for( x = 0; x < sDstImgWidth; x++ )
			{
				ixs =xs-xs%MY_SHIFT_ALL;
				iys =ys-ys%MY_SHIFT_ALL;
				a = xs - ixs;
				b = ys - iys;
				a1 = MY_SHIFT_ALL - a;
				
				nY=iys>>MY_SHIFT_BIT;
				nX=ixs>>MY_SHIFT_BIT;

				//nY>0&&nY<nH1Src?(ptr0 = src + sSrcImgWidth*nY,ptr1 = ptr0 + sSrcImgWidth):(ptr0 = ptr1 = src + (nY < 0 ? 0 : nH1Src)*sSrcImgWidth);
				nY>=0&&nY<nH1Src?(ptr0 = src + sSrcImgWidth*nY,ptr1 = ptr0 + sSrcImgWidth):(ptr0 = ptr1 = src + (nY < 0 ? 0 : nH1Src)*sSrcImgWidth);

				//if(nX< nW1Src)
				if(nX >= 0 && nX < nW1Src)
				{
					// 若nY<0, ptr0 = ptr1 = src, 若此时nX<0, 则会访问src指针之前的数据，发生访问冲突!!!
					// 修改上面的条件判断, 另外这里nY>0和nX>0是不是应该改为nY>=0和nX>=0
					p0 = icv8x32fTab_cv[ptr0[nX]+256]*a1 + icv8x32fTab_cv[ptr0[nX+1]+256]*a;
					p1 = icv8x32fTab_cv[ptr1[nX]+256]*a1 + icv8x32fTab_cv[ptr1[nX+1]+256]*a;
				}
				else
				{
					nX = nX < 0 ? 0 : nW1Src;
					p0 = icv8x32fTab_cv[ptr0[nX]+256];
					p1 = icv8x32fTab_cv[ptr1[nX]+256];
				}
				dst[dist+x] = (unsigned char)((((p0<<MY_SHIFT_BIT) + b * (p1 - p0)>>MY_SHIFT_BIT)+MY_SHIFT_HALF)>>MY_SHIFT_BIT);

				xs += A11;
				ys += A21;
			}
		}
	}
}


unsigned char * GetRotateZoomImage()
{
	return gucImgRttZoom;
}

unsigned char * GetRotateImage2()
{
	return gucImgRttImage2;
}

