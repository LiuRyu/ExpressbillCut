#include "stdafx.h"
#include "RyuType.h"


RyuPoint ryuPoint(int x, int y)
{
	RyuPoint p;

	p.x = x;
	p.y = y;

	return p;
}

RyuRect ryuRect(int x, int y, int width, int height)
{
	RyuRect r;

	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;

	return r;
}


RyuSize ryuSize(int width, int height)
{
	RyuSize s;

	s.width = width;
	s.height = height;

	return s;
}

