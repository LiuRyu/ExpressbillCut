#include "stdafx.h"
#include <math.h>
#include <time.h>

#include "RyuCore.h"
#ifdef _DEBUG
#include "OpenCv_debugTools.h"
#endif
//
#include "LocateCodeNumber.h"
#include "PerspectiveTrans.h"
#include "OCR.h"
#include <windows.h>
//#include "MyMaths.h"
//#include "ClassicCCA_utils.h"
//#include "Gradiant_utils.h"
//#include "BarcodeImprovement.h"
//#include "Rotate.h"
//#include "USMSharpening.h"
//#include "RecgDecode.h"
//#include "Algorithm_locate.h"
//#include "BarCodeRecovery.h"

int nImproveMaxWidth = 0, nImproveMaxHeight = 0;

unsigned char * dstBottomImage;//�����°벿ͼ��
int BottomImageWidth =0;
int BottomImageHeight =0;
unsigned char * dstTopImage;//�����ϰ벿ͼ��
int TopImageWidth = 0;
int TopImageHeight =0;
unsigned char * dstBottomHorizontalImage;//�����°벿ˮƽͶӰ
unsigned char * dstTopHorizontalImage; //�����ϰ벿ˮƽͶӰ
unsigned char * NumberImage; //ȷ�����½�����������ͼ��
unsigned char * VerTicalNumberImage;//���봹ֱͶӰ
int * nPVX_Flag = 0;//��ֵ���x������
int *PeakStep = 0;//��ֵx����
unsigned char * NumberImageEx;//ȷ�����ҽ�����������ͼ��(������������)
OCR* pOCR;
vector< string > vStrResult; // Ϊÿ��ͼ�����5����ѡ���
vector< string > vCharCnd;   // Ϊÿ���ַ��ṩ3����ѡ���
int LocateCodeNumber_init(int max_wid, int max_hei)
{
	nImproveMaxWidth = max_wid;
	nImproveMaxHeight = max_hei;

	dstBottomImage = ( unsigned char *)malloc(max_wid*max_hei*sizeof(unsigned char));
	if(!dstBottomImage){
		return -1;
	}

	dstTopImage = ( unsigned char *)malloc(max_wid*max_hei*sizeof(unsigned char));
	if(!dstTopImage){
		return -1;
	}

	dstBottomHorizontalImage =( unsigned char *)malloc(max_hei*sizeof(unsigned char));
	if(!dstBottomHorizontalImage){
		return -1;
	}

	dstTopHorizontalImage =( unsigned char *)malloc(max_hei*sizeof(unsigned char));
	if(!dstTopHorizontalImage){
		return -1;
	}
	
	
	NumberImage = ( unsigned char *)malloc(max_wid*max_hei*sizeof(unsigned char));
	if(!NumberImage) {
		return -1;
	}

	VerTicalNumberImage = ( unsigned char *)malloc(max_wid*sizeof(unsigned char));
	if(!VerTicalNumberImage) {
		return -1;
	}
	nPVX_Flag =(int *)malloc(max_wid*sizeof(int));
	if(!nPVX_Flag) {
		return -1;
	}

	PeakStep =(int *)malloc(max_wid * sizeof(int));
	if(!PeakStep) {
		return -1;
	}

	NumberImageEx = ( unsigned char *)malloc(max_wid*max_hei*sizeof(unsigned char));
	if(!NumberImageEx) {
		return -1;
	}
	pOCR = new OCR;
	if(!pOCR) {
		return -1;
	}

	char Path[MAX_PATH] = {0};
	char Path1[MAX_PATH] = {0};
	GetModuleFileNameA(NULL,Path,MAX_PATH);
	(strrchr(Path,'\\'))[0]=0;
	strcpy(Path1,Path); 
	strcat(Path,"\\Dict\\dict_1.csp");
	strcat(Path1,"\\Dict\\dict_2.nn");
	const char* dictName_csp = Path;
	const char* dictName_nn  = Path1;

	int err = pOCR->loadModels( dictName_csp, dictName_nn ); // step1:ϵͳ����֮ǰ�ȼ���ģ�ͣ�ֻ�ü���һ�μ���
	if( 1 != err )
	{
		/*cout<< "load model failure"<< endl;*/
		#ifdef	_PRINT_PROMPT
			printf( "Warning! Unexpected return of OCR load model failure, ret_val=%d\n", err );
#endif
		return -1;
	}

	return 1;
}

void LocateCodeNumber_release()
{
	if(dstBottomImage)
		free(dstBottomImage);

	if(dstTopImage)
		free(dstTopImage);

	if(dstBottomHorizontalImage)
		free(dstBottomHorizontalImage);

	if(dstTopHorizontalImage)
		free(dstTopHorizontalImage);

	if(NumberImage)
		free(NumberImage);

	if(VerTicalNumberImage)
		free(VerTicalNumberImage);

	if(nPVX_Flag)
		free(nPVX_Flag);

	if(PeakStep)
		free(PeakStep);

	if(NumberImageEx)
		free(NumberImageEx);

}

/***��ȡ�������²���ͼ��***/
int SetCodeNumberImage(int BottomorTop,unsigned char * srcImage,const int sSrcImgWidth,const int sSrcImgHeight)
{
	int Return = 0;

	if(sSrcImgWidth>nImproveMaxWidth||sSrcImgHeight>nImproveMaxHeight||BottomorTop<1||BottomorTop>2)
	{
		Return = -1;
		return Return;
	}
	if(BottomorTop==1)
	{
		BottomImageWidth = sSrcImgWidth;
		BottomImageHeight= sSrcImgHeight;
		memcpy(dstBottomImage,srcImage,BottomImageWidth*BottomImageHeight*sizeof(unsigned char));
		/*#ifdef _DEBUG
			IplImage * iplnlBottom = cvCreateImage(cvSize(BottomImageWidth, BottomImageHeight), 8, 1);
			uc2IplImageGray(dstBottomImage,iplnlBottom);
			cvNamedWindow("����ײ���ͼ");
			cvShowImage("����ײ���ͼ", iplnlBottom);
			cvReleaseImage(&iplnlBottom);
			cvWaitKey(1000);
		#endif*/
	}
	else
	{
		TopImageWidth = sSrcImgWidth;
		TopImageHeight= sSrcImgHeight;
		memcpy(dstTopImage,srcImage,TopImageWidth*TopImageHeight*sizeof(unsigned char));
		/*#ifdef _DEBUG
			IplImage * iplnlTop = cvCreateImage(cvSize(TopImageWidth, TopImageHeight), 8, 1);
			uc2IplImageGray(dstTopImage,iplnlTop);
			cvNamedWindow("���붥����ͼ");
			cvShowImage("���붥����ͼ", iplnlTop);
			cvReleaseImage(&iplnlTop);
			cvWaitKey(1000);
		#endif*/
	}
	Return = 1;
	return Return;
}

void VerTicalProjection(unsigned char * const srcImage,unsigned char *dstImage,const int sSrcImgWidth,const int sSrcImgHeight,int widthstep,unsigned char *average_grey)
{
	const unsigned char* p;
	unsigned char * p_dst;
	p = srcImage;
	p_dst = dstImage;
	long sum;
	long sum_2 = 0;

	for(int i=0;i<sSrcImgWidth;i++)
	{    
		sum =0;
		p=srcImage+i;
		for(int j=0;j<sSrcImgHeight;j++)
		{
			sum+=*p;
			p+= widthstep;
		}
		p_dst[i] = (unsigned char)(sum/sSrcImgHeight);
		sum_2 +=p_dst[i];
		
	}
	 *average_grey = sum_2/sSrcImgWidth;
}

int IPA_getWavePeakValley4(unsigned char *cWave,int nWaveLen,float fBideGapRatio,float fBideGapTh,float fMidGapRatioTh,float fMidGapTh,int *nPVX_Flag)
{
	int i,n,nFlagLast,nLastX,nXEnd,nFlagCur;
	//float fLMax,fRMax;
	int nGapL,nGapR,nLMax,nRMax;
	

	//1������ʼ��,���������Ƶ���
	nLastX=0;
	nFlagLast=0;
	for (i=1;i<nWaveLen;i++)
	{
		nFlagCur=cWave[i]>cWave[i-1]?1:(cWave[i]<cWave[i-1]?-1:0);
		//fGapR=fabs(fWave[i+1]-fWave[i]);
		//fRMax=(fWave[i]+fWave[i+1])/2.0;//�þ�ֵӦ�ø�׼ȷЩ
		nGapR=abs(cWave[i]-cWave[nLastX]);
		nRMax=(cWave[i]+cWave[nLastX])/2.0;//�þ�ֵӦ�ø�׼ȷЩ

		if(nFlagCur==-1&&nGapR>fBideGapRatio*nRMax&&nGapR>fBideGapTh)//��С���ұ仯������Ϊ�ҵ���ʼ�հ���
		{
			nLastX=i;
			break;
		}
		if (nFlagCur!=0&&nFlagCur!=nFlagLast)
		{
			nLastX=i;
			nFlagLast=nFlagCur;
		}
	}
	nPVX_Flag[0]=nLastX;
	for(i=nLastX;i>0;i--)
	{
		if(cWave[i]<=cWave[i-1])
		{
			nPVX_Flag[0]--;
		}
		else
		{
			break;
		}
	}

	//2������ֹ��,���������Ƶ���
	nLastX=nWaveLen-1;
	nFlagLast=0;
	for (i=nWaveLen-2;i>nPVX_Flag[0];i--)
	{
		nFlagCur=cWave[i-1]>cWave[i]?1:(cWave[i-1]<cWave[i]?-1:0);
		/*fGapL=fabs(fWave[i-1]-fWave[i]);
		fLMax=(fWave[i]+fWave[i-1])/2.0;*/
		nGapL=abs(cWave[i]-cWave[nLastX]);
		nLMax=(cWave[i]+cWave[nLastX])/2.0;
		if(nFlagCur==-1&&nGapL>fBideGapRatio*nLMax&&nGapL>fBideGapTh)//��С���ұ仯������Ϊ�ҵ���ʼ�հ���
		{
			nLastX=i;
			break;
		}
		if (nFlagCur!=0&&nFlagCur!=nFlagLast)
		{
			nLastX=i;
			nFlagLast=nFlagCur;
		}
	}
	nXEnd=nLastX;
	for(i=nLastX;i<nWaveLen;i++)
	{
		if(cWave[i]<=cWave[i+1])
		{
			nXEnd++;
		}
		else
		{
			break;
		}
	}


	//3����ʼ����ʼ����ֹ������з��
	nFlagLast=-1;
	nLastX=nPVX_Flag[0];
	n=1;
	for (i=nPVX_Flag[0]+1;i<nXEnd;i++)
	{
		nFlagCur=cWave[i+1]>cWave[i]?1:(cWave[i+1]<cWave[i]?-1:0);
		nGapL=abs(cWave[i]-cWave[nLastX]);
		nGapR=abs(cWave[i+1]-cWave[i]);
		nLMax=(cWave[i]+cWave[nLastX])/2.0;
		nRMax=(cWave[i]+cWave[i+1])/2.0;	
		if(nFlagLast*nFlagCur<0&&((nGapL>fMidGapRatioTh*nLMax&&nGapL>fMidGapTh)||(nGapR>fMidGapRatioTh*nRMax&&nGapR>fMidGapTh)))//���߱仯���Ʋ�һ��	����ͬʱ��0	�仯�ܴ�
		{
			nPVX_Flag[n++]=nFlagLast<0?-i:i;
			nFlagLast=nFlagCur;
			nLastX=i;
		}
	}

	if (nPVX_Flag[n-1]<0)
	{
		nPVX_Flag[n++]=nXEnd;//����ֹ�����
	}
	return n;
}

int LocateCodeNumber(int codeHeight,unsigned char ** dstImage,int *width, int *height)
{
	

	//int codeWidth = coed_right - coed_left;
	//int codeHeight;
	//if(code_top>code_bottom)
	//	codeHeight = code_top-code_bottom;
	//else
	//	codeHeight = code_bottom -code_top;
	//unsigned char * pTopNumSrcImg = srcImage+ coed_left; //��������ͼ�񣨽Ƕ�У������תͼ��
	//unsigned char * pBottomNumSrcImg = srcImage + (code_bottom+1) * sSrcImgWidth + coed_left;//��������ͼ��
	//int Num_Bottom_Height = sSrcImgHeight - code_bottom-1;
	//int Num_Bottom_Width = coed_right - coed_left;
	//int Num_Top_Height = code_top;
	//int Num_Top_Width = coed_right - coed_left;
	
	int top=0,bottom=0,left = 0,right = 0;
	//����ײ�ˮƽͶӰ
	int temp_sum = 0; 
	for(int i = 0;i < BottomImageHeight;i++)
	{
		int horizontal_sum = 0;
		for(int j = 0;j<BottomImageWidth;j++)
		{
			horizontal_sum+=*(dstBottomImage +i*BottomImageWidth+j);
		}
		dstBottomHorizontalImage[i] = horizontal_sum/BottomImageWidth;
		temp_sum += dstBottomHorizontalImage[i];
	}
	int average_grey_Bottom = temp_sum/BottomImageHeight+2;
	int number_start_index = 0;
	int numberwidth = 0;
	float min_ration = 0.1;//����/������С����
	float max_ration = 0.5;//����/����������
	float max_number_ration = 0.3;//���־���������С����
	//���������½�
	for(int i = 0; i < BottomImageHeight; i++)
	{
		if((dstBottomHorizontalImage[i]>average_grey_Bottom)&&(dstBottomHorizontalImage[i+1]<=average_grey_Bottom))
		{
			number_start_index =i+1;
			if(number_start_index>max_number_ration*codeHeight)
			{
				number_start_index = 0;
				break;
			}
		}
		if((dstBottomHorizontalImage[i]<=average_grey_Bottom)&&(dstBottomHorizontalImage[i+1]>average_grey_Bottom)&&number_start_index>0)
		{
			numberwidth = i+1-number_start_index;
			if(numberwidth<min_ration*codeHeight||numberwidth>max_ration*codeHeight)
			{
				number_start_index = 0;
				numberwidth = 0;
			}
			else
			{
				break;
			}
		}
	}

	//#ifdef _DEBUG
	//	IplImage * debug_proj = cvCreateImage(cvSize(256, BottomImageHeight), 8, 3);
	//	cvZero(debug_proj);
	//	cvLine(debug_proj, cvPoint(average_grey_Bottom,0), cvPoint(average_grey_Bottom, BottomImageHeight), 
	//			CV_RGB(0,255,0));
	//	/*cvLine(debug_proj, cvPoint(average_grey_Top,0), cvPoint(average_grey_Top, BottomImageHeight), 
	//			CV_RGB(255,0,0));*/
	//	
	//	for(int i = 0; i < BottomImageHeight-1; i++) {
	//
	//		cvLine(debug_proj, cvPoint(dstBottomHorizontalImage[i],i), cvPoint(dstBottomHorizontalImage[i+1],i+1), 
	//			CV_RGB(255,255,255));
	//	}

	//	cvNamedWindow("����ײ�ˮƽͶӰ");
	//	cvShowImage("����ײ�ˮƽͶӰ", debug_proj);
	//	cvReleaseImage(&debug_proj);
	//#endif

	//���붥��ˮƽͶӰ
	temp_sum = 0;
	for(int i = 0;i<TopImageHeight;i++)
	{
		int horizontal_sum = 0;
		for(int j = 0;j<TopImageWidth;j++)
		{
			horizontal_sum+=*(dstTopImage+i*TopImageWidth+j);
		}
		dstTopHorizontalImage[i] = horizontal_sum/TopImageWidth;
		temp_sum += dstTopHorizontalImage[i];
	}
	int average_grey_Top = temp_sum/TopImageHeight+2;
 	int number_start_index_1 = 0;
	int numberwidth_1 = 0;
	//���������Ͻ��
	for(int i = TopImageHeight-1; i > 0; i--)
	{
 		if((dstTopHorizontalImage[i]>average_grey_Top)&&(dstTopHorizontalImage[i-1]<=average_grey_Top))
		{
			number_start_index_1 =i-1;
			if(number_start_index_1<TopImageHeight-max_number_ration*codeHeight)
			{
				number_start_index_1 = 0;
				break;
			}
		}
		if((dstTopHorizontalImage[i]<=average_grey_Top)&&(dstTopHorizontalImage[i-1]>average_grey_Top)&&number_start_index_1>0)
		{
 			numberwidth_1 = number_start_index_1-i+1;
			if(numberwidth_1<min_ration*codeHeight||numberwidth_1>max_ration*codeHeight)
			{
				number_start_index_1 = 0;
				numberwidth_1 = 0;
			}
			else
			{
				break;
			}
		}
	}
	/*#ifdef _DEBUG
		IplImage * debug_proj1 = cvCreateImage(cvSize(256, TopImageHeight), 8, 3);
		cvZero(debug_proj1);
		cvLine(debug_proj1, cvPoint(average_grey_Top,0), cvPoint(average_grey_Top, TopImageHeight), 
				CV_RGB(255,0,0));
		

		for(int i = 0; i < TopImageHeight-1; i++) {
	
			cvLine(debug_proj1, cvPoint(dstTopHorizontalImage[i],i), cvPoint(dstTopHorizontalImage[i+1],i+1), 
				CV_RGB(255,255,255));
		}

		cvNamedWindow("���붥��ˮƽͶӰ");
		cvShowImage("���붥��ˮƽͶӰ", debug_proj1);
		cvReleaseImage(&debug_proj1);
	#endif*/
		int whole_offset = 2;
		int BottomOrTop = 0;//�����������϶˻����¶� 1�¶ˣ�2 �϶�

		if(number_start_index>0&&numberwidth>0&&number_start_index_1>0&&numberwidth_1>0)//�������¾��ҵ���������
		{
 			int min_value1 = dstBottomHorizontalImage[number_start_index],min_value2 = dstTopHorizontalImage[number_start_index_1-numberwidth_1],max_value1 = dstBottomHorizontalImage[number_start_index],max_value2 = dstTopHorizontalImage[number_start_index_1-numberwidth_1];
			for(int i = number_start_index;i<number_start_index+numberwidth;i++)
			{
				if(dstBottomHorizontalImage[i]<min_value1)
					min_value1 = dstBottomHorizontalImage[i];
				if(dstBottomHorizontalImage[i]>max_value1)
					max_value1 = dstBottomHorizontalImage[i];
			}
			for(int i = number_start_index_1-numberwidth_1;i<number_start_index_1;i++)
			{
				if(dstTopHorizontalImage[i]<min_value2)
					min_value2 = dstTopHorizontalImage[i];
				if(dstTopHorizontalImage[i]>max_value2)
					max_value2 = dstTopHorizontalImage[i];
			}
			if(max_value2 - min_value2>max_value1-min_value1)//�����Сֵ֮����С��Ϊ���������ų����ߣ�
			{
				/*cvRectangle(iplrtt2, cvPoint(coed_left,number_start_index+code_bottom), cvPoint(coed_right, number_start_index+numberwidth+code_bottom), 
						CV_RGB(0,255,255));*/
				if(2*number_start_index+numberwidth-2+whole_offset>=BottomImageHeight)//�����½�
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,code_bottom+2+2), cvPoint(coed_right, sSrcImgHeight-1), 
						CV_RGB(0,255,255));*/
					top = 2+whole_offset;
					bottom = BottomImageHeight-1;
					
				}
				else
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,code_bottom+2+2), cvPoint(coed_right, 2*number_start_index+numberwidth+code_bottom-2+2), 
						CV_RGB(0,255,255));*/
					
					top = 2+whole_offset;
					bottom = 2*number_start_index+numberwidth-2+whole_offset;
				}
				BottomOrTop = 1;
				*height = bottom - top;
			}
			else
			{
				/*cvRectangle(iplrtt2, cvPoint(coed_left,number_start_index_1-numberwidth_1), cvPoint(coed_right, number_start_index_1), 
						CV_RGB(0,255,255));*/
				if(2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset<=0)//�����Ͻ�
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,1), cvPoint(coed_right, code_top-2-2), 
													CV_RGB(0,255,255));*/
					top = 1;
					bottom = TopImageHeight-2-whole_offset;
				}
				else
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,2*number_start_index_1-numberwidth_1-code_top+2-2), cvPoint(coed_right, code_top-2-2), 
						CV_RGB(0,255,255));*/
					top = 2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset;
					bottom = TopImageHeight-2-whole_offset;
				}
				BottomOrTop = 2;
				*height = bottom - top;
			}
		}
		else
		{
 			if(number_start_index>0&&numberwidth>0)
			{
				/*cvRectangle(iplrtt2, cvPoint(coed_left,number_start_index+code_bottom), cvPoint(coed_right, number_start_index+numberwidth+code_bottom), 
						CV_RGB(0,255,255));*/
				if(2*number_start_index+numberwidth-2+whole_offset>=BottomImageHeight)//�����½�
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,code_bottom+2+2), cvPoint(coed_right, sSrcImgHeight-1), 
						CV_RGB(0,255,255));*/
					top = 2+whole_offset;
					bottom = BottomImageHeight-1;
				}

				else
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,code_bottom+2+2), cvPoint(coed_right, 2*number_start_index+numberwidth+code_bottom-2+2), 
						CV_RGB(0,255,255));*/
					top = 2+whole_offset;
					bottom = 2*number_start_index+numberwidth-2+whole_offset;
				}
				BottomOrTop = 1;
				*height = bottom - top;
			}
			if(number_start_index_1>0&&numberwidth_1>0)
			{
				/*cvRectangle(iplrtt2, cvPoint(coed_left,number_start_index_1-numberwidth_1), cvPoint(coed_right, number_start_index_1), 
						CV_RGB(0,255,255));*/
				if(2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset<=0)//�����Ͻ�
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,1), cvPoint(coed_right, code_top-2-2), 
													CV_RGB(0,255,255));*/
					top = 1;
					bottom = TopImageHeight-2-whole_offset;
				}
				else
				{
					/*cvRectangle(iplrtt2, cvPoint(coed_left,2*number_start_index_1-numberwidth_1-code_top+2-2), cvPoint(coed_right, code_top-2-2), 
						CV_RGB(0,255,255));*/
					top = 2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset;
					bottom = TopImageHeight-2-whole_offset;
				}
				BottomOrTop = 2;
				*height = bottom - top;
			}
			
		}
		

		//��������ͼ��
		int codeWidth = 0;
		if(BottomOrTop!=0)
		{
			int NumHeight = *height;
			if(NumHeight == 0)
				return 0;
			if(BottomOrTop == 1)
			{
				for(int j=0;j<NumHeight;j++)
				{
					for(int i=0;i<BottomImageWidth;i++)
					{
						NumberImage[j*BottomImageWidth+i] = dstBottomImage[(top+j)*BottomImageWidth+i];
					}
				}
				codeWidth = BottomImageWidth;
			}
			else
			{
				for(int j=0;j<NumHeight;j++)
				{
					for(int i=0;i<TopImageWidth;i++)
					{
						NumberImage[j*TopImageWidth+i] = dstTopImage[(bottom-j)*TopImageWidth+TopImageWidth-i];
					}
				}
				codeWidth = TopImageWidth;
			}
			

		/*#ifdef _DEBUG
			IplImage * iplnl = cvCreateImage(cvSize(codeWidth, NumHeight), 8, 1);
			uc2IplImageGray(NumberImage,iplnl);
			cvNamedWindow("��������");
			cvShowImage("��������", iplnl);
		#endif*/


			//ȷ�����ұ߽�
			
			unsigned char  verTical_averagey = 0;
			memset(nPVX_Flag,0,codeWidth*sizeof(int));
			VerTicalProjection(NumberImage,VerTicalNumberImage,codeWidth,NumHeight,codeWidth,&verTical_averagey);
			int n=IPA_getWavePeakValley4(VerTicalNumberImage,codeWidth,0.05,8,0.05,10,nPVX_Flag);
			int bianjie = 0;
			if(n<15)//�����ֵ �ж�Ϊ������������
			{
				BottomOrTop = 0;
				*dstImage = NULL;
				/*free(NumberImage);
				free(dstBottomHorizontalImage);
				free(dstTopHorizontalImage);
				free(nPVX_Flag);*/
				return BottomOrTop;
			}
			else
			{
				if(abs(nPVX_Flag[0]-codeWidth+nPVX_Flag[n-1])>30)
				{
					float average_PeakStep = 0.0;
					int k=(n-1)/2;
					
					
					for(int i = 0;i<k;i++)
					{
						PeakStep[i] = nPVX_Flag[2*i+2]-nPVX_Flag[2*i];
						/*white[i] = abs(nPVX_Flag[2*i+3])-abs(nPVX_Flag[2*i+1]);*/
						average_PeakStep += PeakStep[i];
					}
					average_PeakStep = average_PeakStep/k;
					if(nPVX_Flag[0]<codeWidth-nPVX_Flag[n-1])//Ѱ�Ҳ���������
					{
						for(int i = 0;i<k;i++)
						{
							if(PeakStep[i]>average_PeakStep*3)
							{
								/*bianjie = (i+1)*2;*/
								bianjie = (i+1)*2;
								if(bianjie == n-1)
									bianjie = 0;
							}
						}
					}
					else
					{
						for(int i = k-1;i>0;i--)
						{
							if(PeakStep[i]>average_PeakStep*3)
							{
								bianjie = -i*2;
								/*bianjie = -(i+1)*2;*/
							}
						}

					}
					
				}
			}
			if(bianjie>0)
			{
				if(nPVX_Flag[bianjie]-20<0)
					left = 0;
				else
					left = nPVX_Flag[bianjie]-20;
				if(nPVX_Flag[n-1]+10>codeWidth-1)
					right = codeWidth-1;
				else
					right = nPVX_Flag[n-1]+10;
			}
			else if(bianjie<0)
			{
				if(nPVX_Flag[0]-10<0)
					left = 0;
				else
					left = nPVX_Flag[0]-10;
				if(nPVX_Flag[abs(bianjie)]+20>codeWidth-1)
					right = codeWidth-1;
				else
					right = nPVX_Flag[abs(bianjie)]+20;
			}
			else
			{
				if(nPVX_Flag[0]-10<0)
					left = 0;
				else
					left = nPVX_Flag[0]-10;
				if(nPVX_Flag[n-1]+10>codeWidth-1)
					right = codeWidth-1;
				else
					right = nPVX_Flag[n-1]+10;
			}
			*width = right - left;
			int NumWidth = *width;
			
			
			for(int j=0;j<NumHeight;j++)
			{
				for(int i=0;i<NumWidth;i++)
				{
					NumberImageEx[j*NumWidth+i] = NumberImage[j*codeWidth+left+i];
				}
			}
			*dstImage = NumberImageEx;
		/*#ifdef _DEBUG
			IplImage * iplnlEx = cvCreateImage(cvSize(NumWidth, NumHeight), 8, 1);
			uc2IplImageGray(NumberImageEx,iplnlEx);
			cvNamedWindow("������������и�");
			cvShowImage("������������и�", iplnlEx);
			cvReleaseImage(&iplnlEx);
		#endif*/


	/*#ifdef _DEBUG
			IplImage * debug_proj = cvCreateImage(cvSize(codeWidth, 512), 8, 3);
			cvZero(debug_proj);
			cvLine(debug_proj, cvPoint(left,0), cvPoint(left,512), CV_RGB(255,0,0));
			cvLine(debug_proj, cvPoint(right,0), cvPoint(right,512), CV_RGB(255,0,0));
			
			for(int i = 0; i < codeWidth-1; i++) {
	
				cvLine(debug_proj, cvPoint(i,VerTicalNumberImage[i]+100), cvPoint(i+1, VerTicalNumberImage[i+1]+100), 
					CV_RGB(255,255,255));
			
			}
			char txt[50];
			CvFont font;
			sprintf(txt, "l-%d r-%d c-%d ", nPVX_Flag[0],codeWidth-nPVX_Flag[n-1],abs(nPVX_Flag[0]-codeWidth+nPVX_Flag[n-1]));
			cvInitFont(&font, CV_FONT_HERSHEY_COMPLEX_SMALL, max(debug_proj->width/2048,0.8), max(debug_proj->width/2048,0.8), 0.0, max(debug_proj->width/2048,0.8), CV_AA);
			cvPutText(debug_proj, txt, cvPoint(5, debug_proj->width/10), &font, CV_RGB(255,255,0));
			cvNamedWindow("���ִ�ֱͶӰ");
			cvShowImage("���ִ�ֱͶӰ", debug_proj);

			cvLine(iplnl, cvPoint(left,0), cvPoint(left,512), CV_RGB(255,0,0));
			cvLine(iplnl, cvPoint(right,0), cvPoint(right,512), CV_RGB(255,0,0));
			cvNamedWindow("��������");
			cvShowImage("��������", iplnl);
			cvReleaseImage(&iplnl);
			cvWaitKey(2000);
			cvReleaseImage(&debug_proj);
			cvReleaseImage(&debug_proj1);
	#endif*/

			

		}
		

		return BottomOrTop;
}

int NumberRecoge(int codeHeight,char * code_result,int * code_char_num,int *code_direct)
{
	unsigned char * pNumImg = NULL;
	int NumImageW = 0,NumImageH = 0;
	int  nNumImageFixHeight = 50;//����ͼ�����Ź̶��߶�
	int codeDirect = 0;
	int status = 0;
	status = LocateCodeNumber(codeHeight,&pNumImg,&NumImageW, &NumImageH);
	if(status <= 0){
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of LocateCodeNumber, ret_val=%d\n", status );
#endif
		return -1;
	}
	if(NumImageW <= 0 || NumImageH <= 0) {
#ifdef	_PRINT_PROMPT
		printf( "Warning! Unexpected return of LocateCodeNumber, NumImageW=%d, NumImageH=%d\n", NumImageW, NumImageH );
#endif
		return -1;
	}
	codeDirect = status;
	//��ͼ���������̶�
	//ͼ������
	int dstHeight = nNumImageFixHeight;
	int dstWidth = nNumImageFixHeight * NumImageW /NumImageH;
	int srcX[8] = {0}, dstX[8] = {0};
	double matrix[9];
	srcX[0] = 0;			srcX[1] = 0; 
	srcX[2] = 0;			srcX[3] = dstHeight-1;
	srcX[4] = dstWidth-1;	srcX[5] = 0; 
	srcX[6] = dstWidth-1;	srcX[7] = dstHeight-1;

	dstX[0] = 0;			dstX[1] = 0; 
	dstX[2] = 0;			dstX[3] = NumImageH-1;
	dstX[4] = NumImageW-1;	dstX[5] = 0; 
	dstX[6] = NumImageW-1;	dstX[7] = NumImageH-1;

	IPA_getPerspectiveTransformMat( srcX, dstX, matrix );
	
	status = IPA_warpPerspectiveTransformFixed( pNumImg, NumImageW, NumImageH,NumberImage, dstWidth, dstHeight, matrix );
	if(status <=0){
#ifdef	_PRINT_PROMPT
			printf( "Warning! Unexpected return of IPA_warpPerspectiveTransformFixed, ret_val=%d\n", status );
#endif
	return -1;
	}

#ifdef _DEBUG
#ifdef _DEBUG_LCTCODENUM
	IplImage * iplRcgIm = cvCreateImage(cvSize(dstWidth, dstHeight), 8, 1);
	for (int yy=0;yy<iplRcgIm->height;yy++)
	{
		for (int xx=0;xx<iplRcgIm->width;xx++)
		{
			((unsigned char *)(iplRcgIm->imageData + iplRcgIm->widthStep*yy))[xx]=NumberImage[yy*iplRcgIm->width+xx];
		}
	}
	cvNamedWindow("����ʶ��ͼ��");
	cvShowImage("����ʶ��ͼ��", iplRcgIm);
	cvWaitKey();
	cvReleaseImage(&iplRcgIm);
#endif
#endif

	//����OCRģ���ȡ���
	/*vStrResult.clear();	
	vCharCnd.clear();*/
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- +++OCR proc start...");
#endif
#endif
	int err = pOCR->textRecognition( (const unsigned char* )NumberImage, dstHeight, dstWidth, vStrResult, vCharCnd ); // step2:���ýӿں���
 	if( 1 != err )
	{
		/*cout<< "error!"<< endl;*/
		#ifdef	_PRINT_PROMPT
			printf( "Warning! Unexpected return of OCR pOCR->textRecognition, ret_val=%d\n", err );
       #endif
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
			Write_Log(LOG_TYPE_INFO, "TRACE- +++OCR proc cannot find results, ret_val=%d\n", err);
#endif
#endif
		return -1;
	}
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- +++OCR proc find results successfully");
#endif
#endif
	int size = vStrResult[0].size();
	if(size <10||size>12)//λ������
		return 0;
	for( int n=1; n<vStrResult.size(); n++ )
	{
		if(vStrResult[n].size()!=size)//���λ����ͬ
			return 0;
	}
	for( int i=1; i<vStrResult.size(); i++ )
	{
		int dif = 0;
		for( int j=0; j<vStrResult[0].size(); j++ )
		{
			if(vStrResult[0][j] != vStrResult[i][j])
					dif++;
		}
		if(dif>2)
			return 0;
	}
	
	
	*code_char_num = vStrResult[0].size();
	if(codeDirect == 1)
		*code_direct = codeDirect;
	else
		*code_direct = codeDirect-3;
	strcpy(code_result,vStrResult[0].c_str());
	printf("code_result= %s\n",code_result);
	return 1;
}