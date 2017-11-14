#ifndef _RYU_TYPE_H
#define _RYU_TYPE_H


#ifndef RYUPOINT
#define RYUPOINT
typedef struct RyuPoint
{
	int x;
	int y;
}
RyuPoint;
#endif

RyuPoint ryuPoint(int x, int y);

#ifndef RYULINE
#define RYULINE
typedef struct RyuLine
{
	RyuPoint pt0;
	RyuPoint pt1;
}
RyuLine;
#endif

#ifndef RYUHOUGHLINE
#define RYUHOUGHLINE
typedef struct RyuHoughLine
{
	int		 theta;
	int		 rho;
	int		 ptCnt;
}
RyuHoughLine;
#endif

#ifndef RYUFULLLINE
#define RYUFULLLINE
typedef struct RyuFullLine
{
	RyuPoint pt0;
	RyuPoint pt1;
	int		 theta;
	int		 rho;
	int		 ptCnt;
}
RyuFullLine;
#endif

#ifndef RYURECT
#define RYURECT
typedef struct RyuRect
{
	int x;
	int y;
	int width;
	int height;
}
RyuRect;
#endif

RyuRect ryuRect(int x, int y, int width, int height);

#ifndef RYUSIZE
#define RYUSIZE
typedef struct RyuSize
{
	int width;
	int height;
}
RyuSize;
#endif

RyuSize ryuSize(int width, int height);

#ifndef RYUIMAGEROI
#define RYUIMAGEROI
typedef struct RyuROI
{
	int  xOffset;
	int  yOffset;
	int  width;
	int  height;
}
RyuROI;
#endif

#ifndef RYUIMAGE
#define RYUIMAGE
typedef struct RyuImage
{
	unsigned char * imagedata;
	int alloc_size;
	int width;
	int height;
	int depth;
	int channel;
	int widthstep;	// 行所占字节数
	RyuROI * roi;
}
RyuImage;
#endif

#define RYU_DEPTH_8C	8
#define RYU_DEPTH_16S	16
#define RYU_DEPTH_32N	32
#define RYU_DEPTH_SIGN	0x80000000



#endif