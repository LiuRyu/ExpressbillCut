#include "stdafx.h"
#include "cv.h"
#include "highgui.h"
#include "cxcore.h"

#include <math.h>
#include "RecFrame.h"
#define Pi (3.1415926/180)
#define Ga 10
#define Gb 15

typedef struct ssss
{
		Pot l1;
		Pot l2;
		float alpha;
}sss;

sss * nong=0;
 int s=0;
 int nn=0;

int linenum=0;
li *lines=0;


int len(Pot a,Pot b)
{
	int s=(a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);
	 s=sqrt(double(s));
	 return s;
}
int GetProjectivePoint_len(Pot pt1, Pot pt2, Pot pOut)
{
	double A,B,C,D;
	
	A=pt1.y-pt2.y;
	B=pt2.x-pt1.x;
	C=pt1.x*pt2.y-pt2.x*pt1.y;
	D = fabs(A * pOut.x + B * pOut.y + C) / sqrt(A * A + B * B);
	return D;
}
int  RecFrame_init0()
{
	s=0;
	nn=0;
    linenum=0;
	return 1;
}
int RecFrame_init(int imageWidth, int  imageHeight)
{
	s=0;
	nn=0;
    linenum=0;
	nong = (sss *)malloc(300 * sizeof(sss));
	if(!nong) {
		return -11806001;
	}
	lines=(li*)malloc(300*sizeof(li));

	
	return 1;
}

void RecFrame_release()
{
	if(nong)
	{
		free(nong);
		nong=0;
	}
	if(lines)
	{
		free(lines);
		lines=0;
	}
	linenum=0;
	nn=0;
	s=0;
}


int RecFrame_InputImg( unsigned char * imageData,int width,int height)
{
	
 
	double alp=0;
	li line1;
	if(!imageData) {
		return -11806006;
	}

	s=0;
	nn=0;
	
  	
	icvHoughLinesProbabilistic(imageData,width,height,1,PI/180, 60, 30, 10,lines,&linenum, 250 )  ;
  
  
   if(linenum>250)
	   s=250;
   else
	   s=linenum;
   
	
    for (int i = 0; i < s; nn++,i++)  
    {  
       
		 line1 = lines[i];
		 alp=(line1.line0.y-line1.line1.y)/((line1.line0.x-line1.line1.x)+0.00000001);
		 if(atan(alp)>0)
		  {
			nong[nn].alpha=atan(alp)*180/3.1415926;

		  }
		  else
		  {
			nong[nn].alpha=180+atan(alp)*180/3.1415926;
		  }
		
		

		nong[nn].l1=line1.line0;
		nong[nn].l2=line1.line1;

       // cvLine(s1, cvPoint(line1.line0.x,line1.line0.y),  cvPoint(line1.line1.x,line1.line1.y), CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
		
		 
    }  
	//cvNamedWindow("s",1);
	//cvShowImage("s",s1);
	//cvWaitKey();



	return 1;
}


int RecFrame_Judge(int ptx[4],int pty[4],double angle)  
{
	for(int i=0;i<4;i++)
	{
		ptx[i]=ptx[i]*MUL;
		pty[i]=pty[i]*MUL;
	}
	
	int score =0;
	if(angle>180)
		angle=angle-180;
	

	IplImage *s1;
   IplImage *s2;
   

	CvSize sz=cvSize(2448*MUL,1501*MUL);
	s1=cvCreateImage(sz,8,3);
	s2=cvCreateImage(sz,8,3);
	
	cvLine(s1,cvPoint(ptx[0],pty[0]),cvPoint(ptx[1],pty[1]),CV_RGB(0,255,0),1,8,0);
	cvLine(s1,cvPoint(ptx[0],pty[0]),cvPoint(ptx[2],pty[2]),CV_RGB(0,255,0),1,8,0);
	cvLine(s1,cvPoint(ptx[2],pty[2]),cvPoint(ptx[3],pty[3]),CV_RGB(0,255,0),1,8,0);
	cvLine(s1,cvPoint(ptx[3],pty[3]),cvPoint(ptx[1],pty[1]),CV_RGB(0,255,0),1,8,0);

	int ptx1=0;
	int ptx2=0;
	int leng=0;
	int sco1=0;
	int sco2=0;
	int sco3=0;
	int sco4=0;
	for(int i=0;i<nn-1;i++)
	{
		if((nong[i].alpha>(angle-5))&&(nong[i].alpha<(angle+5)))
		{
			 cvLine(s1, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y), CV_RGB(0, 0, 255), 0.5, CV_AA, 0);  
			
			leng=GetProjectivePoint_len(pot(ptx[0],pty[0]), pot(ptx[1],pty[1]), nong[i].l1);
			if(leng<Ga)
			{
				if(abs(angle-90)<10)
				{
					if(pty[0]<pty[1])
			      {
				    ptx1=pty[0]-Gb;
				    ptx2=pty[1]+Gb;
			      }
			    else
			      {
			        ptx1=pty[0]+Gb;
				    ptx2=pty[1]-Gb;
			      }
					if(((nong[i].l1.y-ptx1)*(nong[i].l1.y-ptx2)<0)&&((nong[i].l2.y-ptx1)*(nong[i].l2.y-ptx2)<0))
						{
						 cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco1++;
					}
				}
				else
				{
				if(ptx[0]<ptx[1])
			      {
				    ptx1=ptx[0]-fabs(Gb*cos(angle*Pi));
				    ptx2=ptx[1]+fabs(Gb*cos(angle*Pi));
			      }
			    else
			      {
			        ptx1=ptx[0]+fabs(Gb*cos(angle*Pi));
				    ptx2=ptx[1]-fabs(Gb*cos(angle*Pi));
			      }
				if(((nong[i].l1.x-ptx1)*(nong[i].l1.x-ptx2)<0)&&((nong[i].l2.x-ptx1)*(nong[i].l2.x-ptx2)<0))
						{
						 cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco1++;
					}
				}
			}
			leng=GetProjectivePoint_len(pot(ptx[2],pty[2]),pot(ptx[3],pty[3]), nong[i].l1);
			if(leng<Ga)
			{
				if(abs(angle-90)<10)
				{
					if(pty[3]<pty[2])
			      {
				    ptx1=pty[3]-Gb;
				    ptx2=pty[2]+Gb;
			      }
			    else
			      {
			        ptx1=pty[3]+Gb;
				    ptx2=pty[2]-Gb;
			      }
					if(((nong[i].l1.y-ptx1)*(nong[i].l1.y-ptx2)<0)&&((nong[i].l2.y-ptx1)*(nong[i].l2.y-ptx2)<0))
						{
						 cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y), CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco2++;
					}
				}
				else
				{
				if(ptx[3]<ptx[2])
			      {
				    ptx1=ptx[3]-fabs(Gb*cos(angle*Pi));
				    ptx2=ptx[2]+fabs(Gb*cos(angle*Pi));
			      }
			    else
			      {
			        ptx1=ptx[3]+fabs(Gb*cos(angle*Pi));
				    ptx2=ptx[2]-fabs(Gb*cos(angle*Pi));
			      }
				if(((nong[i].l1.x-ptx1)*(nong[i].l1.x-ptx2)<0)&&((nong[i].l2.x-ptx1)*(nong[i].l2.x-ptx2)<0))
						{
						 cvLine(s2,cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco2++;
					}
				}
			}


		}
		if((fabs(nong[i].alpha-angle)>85)&&(fabs(nong[i].alpha-angle)<95))
		{
			 cvLine(s1, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(0, 0, 255), 0.5, CV_AA, 0);  
			leng=GetProjectivePoint_len(pot(ptx[0],pty[0]),pot(ptx[2],pty[2]), nong[i].l1);
			if(leng<Ga)
			{
				if(abs(nong[i].alpha-90)<10)
				{
					if(pty[0]<pty[2])
			      {
				    ptx1=pty[0]-Gb;
				    ptx2=pty[2]+Gb;
			      }
			    else
			      {
			        ptx1=pty[0]+Gb;
				    ptx2=pty[2]-Gb;
			      }
					if(((nong[i].l1.y-ptx1)*(nong[i].l1.y-ptx2)<0)&&((nong[i].l2.y-ptx1)*(nong[i].l2.y-ptx2)<0))
					{
						cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco3++;
					}
				}
				else
				{
				if(ptx[0]<ptx[2])
			      {
				    ptx1=ptx[0]-fabs(Gb*cos(nong[i].alpha*Pi));
				    ptx2=ptx[2]+fabs(Gb*cos(nong[i].alpha*Pi));
			      }
			    else
			      {
			        ptx1=ptx[0]+fabs(Gb*cos(nong[i].alpha*Pi));
				    ptx2=ptx[2]-fabs(Gb*cos(nong[i].alpha*Pi));
			      }
				if(((nong[i].l1.x-ptx1)*(nong[i].l1.x-ptx2)<0)&&((nong[i].l2.x-ptx1)*(nong[i].l2.x-ptx2)<0))
						{
						cvLine(s2,cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y), CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco3++;
					}
				}
			}
			leng=GetProjectivePoint_len(pot(ptx[1],pty[1]), pot(ptx[3],pty[3]), nong[i].l1);
			if(leng<Ga)
			{
				if(abs(nong[i].alpha-90)<10)
				{
					if(pty[1]<pty[3])
			      {
				    ptx1=pty[1]-Gb;
				    ptx2=pty[3]+Gb;
			      }
			    else
			      {
			        ptx1=pty[1]+Gb;
				    ptx2=pty[3]-Gb;
			      }
					if(((nong[i].l1.y-ptx1)*(nong[i].l1.y-ptx2)<0)&&((nong[i].l2.y-ptx1)*(nong[i].l2.y-ptx2)<0))
						{
						 cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco4++;
					}
				}
				else
				{
				if(ptx[1]<ptx[3])
			      {
				    ptx1=ptx[1]-fabs(Gb*cos(nong[i].alpha*Pi));
				    ptx2=ptx[3]+fabs(Gb*cos(nong[i].alpha*Pi));
			      }
			    else
			      {
			        ptx1=ptx[1]+fabs(Gb*cos(nong[i].alpha*Pi));
				    ptx2=ptx[3]-fabs(Gb*cos(nong[i].alpha*Pi));
			      }
				if(((nong[i].l1.x-ptx1)*(nong[i].l1.x-ptx2)<0)&&((nong[i].l2.x-ptx1)*(nong[i].l2.x-ptx2)<0))
						{
						 cvLine(s2, cvPoint(nong[i].l1.x,nong[i].l1.y),cvPoint(nong[i].l2.x,nong[i].l2.y),  CV_RGB(255, 0, 0), 0.5, CV_AA, 0);  
						sco4++;
					}
				}
			}
		}
	}
	
	cvNamedWindow("s1",1);
	cvShowImage("s1",s1);
	cvWaitKey();

	cvNamedWindow("s2",1);
	cvShowImage("s2",s2);
	cvWaitKey();

	cvDestroyWindow("s1");
	cvDestroyWindow("s2");

	
	cvReleaseImage(&s1);
	cvReleaseImage(&s2);
    /**/
	if(sco1>2)
		score+=2;
	else
		score+=sco1;
	if(sco2>2)
		score+=2;
	else
		score+=sco2;
	if(sco3>2)
		score+=2;
	else
		score+=sco3;
	if(sco4>2)
		score+=2;
	else
		score+=sco4;

	
		
		
		for(int i=0;i<4;i++)
	{
		ptx[i]=ptx[i]/MUL;
		pty[i]=pty[i]/MUL;
	}
	
  return score;
    
} 

