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

unsigned char * dstBottomImage;//条码下半部图像
int BottomImageWidth =0;
int BottomImageHeight =0;
unsigned char * dstTopImage;//条码上半部图像
int TopImageWidth = 0;
int TopImageHeight =0;
unsigned char * dstBottomHorizontalImage;//条码下半部水平投影
unsigned char * dstTopHorizontalImage; //条码上半部水平投影
unsigned char * NumberImage; //确定上下界后的数字区域图像
unsigned char * VerTicalNumberImage;//条码垂直投影
int * nPVX_Flag = 0;//峰值峰谷x轴坐标
int *PeakStep = 0;//峰值x轴间距
unsigned char * NumberImageEx;//确定左右界后的数字区域图像(最终数字区域)
OCR* pOCR;
vector< string > vStrResult; // 为每幅图像输出5个候选结果
vector< string > vCharCnd;   // 为每个字符提供3个候选结果
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

	int err = pOCR->loadModels( dictName_csp, dictName_nn ); // step1:系统启动之前先加载模型，只用加载一次即可
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

/***获取条码上下部分图像***/
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
			cvNamedWindow("条码底部截图");
			cvShowImage("条码底部截图", iplnlBottom);
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
			cvNamedWindow("条码顶部截图");
			cvShowImage("条码顶部截图", iplnlTop);
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
	

	//1、找起始峰,并进行左移调整
	nLastX=0;
	nFlagLast=0;
	for (i=1;i<nWaveLen;i++)
	{
		nFlagCur=cWave[i]>cWave[i-1]?1:(cWave[i]<cWave[i-1]?-1:0);
		//fGapR=fabs(fWave[i+1]-fWave[i]);
		//fRMax=(fWave[i]+fWave[i+1])/2.0;//用均值应该更准确些
		nGapR=abs(cWave[i]-cWave[nLastX]);
		nRMax=(cWave[i]+cWave[nLastX])/2.0;//用均值应该更准确些

		if(nFlagCur==-1&&nGapR>fBideGapRatio*nRMax&&nGapR>fBideGapTh)//变小，且变化极大即认为找到初始空白区
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

	//2、找终止峰,并进行右移调整
	nLastX=nWaveLen-1;
	nFlagLast=0;
	for (i=nWaveLen-2;i>nPVX_Flag[0];i--)
	{
		nFlagCur=cWave[i-1]>cWave[i]?1:(cWave[i-1]<cWave[i]?-1:0);
		/*fGapL=fabs(fWave[i-1]-fWave[i]);
		fLMax=(fWave[i]+fWave[i-1])/2.0;*/
		nGapL=abs(cWave[i]-cWave[nLastX]);
		nLMax=(cWave[i]+cWave[nLastX])/2.0;
		if(nFlagCur==-1&&nGapL>fBideGapRatio*nLMax&&nGapL>fBideGapTh)//变小，且变化极大即认为找到初始空白区
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


	//3、开始找起始和终止间的所有峰谷
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
		if(nFlagLast*nFlagCur<0&&((nGapL>fMidGapRatioTh*nLMax&&nGapL>fMidGapTh)||(nGapR>fMidGapRatioTh*nRMax&&nGapR>fMidGapTh)))//两边变化趋势不一样	不能同时是0	变化很大
		{
			nPVX_Flag[n++]=nFlagLast<0?-i:i;
			nFlagLast=nFlagCur;
			nLastX=i;
		}
	}

	if (nPVX_Flag[n-1]<0)
	{
		nPVX_Flag[n++]=nXEnd;//将终止峰加入
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
	//unsigned char * pTopNumSrcImg = srcImage+ coed_left; //条码上沿图像（角度校正后旋转图像）
	//unsigned char * pBottomNumSrcImg = srcImage + (code_bottom+1) * sSrcImgWidth + coed_left;//条码下沿图像
	//int Num_Bottom_Height = sSrcImgHeight - code_bottom-1;
	//int Num_Bottom_Width = coed_right - coed_left;
	//int Num_Top_Height = code_top;
	//int Num_Top_Width = coed_right - coed_left;
	
	int top=0,bottom=0,left = 0,right = 0;
	//条码底部水平投影
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
	float min_ration = 0.1;//数字/条码最小比例
	float max_ration = 0.5;//数字/条码最大比例
	float max_number_ration = 0.3;//数字距离条码最小比例
	//搜索数字下届
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

	//	cvNamedWindow("条码底部水平投影");
	//	cvShowImage("条码底部水平投影", debug_proj);
	//	cvReleaseImage(&debug_proj);
	//#endif

	//条码顶部水平投影
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
	//搜索数字上届届
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

		cvNamedWindow("条码顶部水平投影");
		cvShowImage("条码顶部水平投影", debug_proj1);
		cvReleaseImage(&debug_proj1);
	#endif*/
		int whole_offset = 2;
		int BottomOrTop = 0;//数字在条码上端还是下端 1下端；2 上端

		if(number_start_index>0&&numberwidth>0&&number_start_index_1>0&&numberwidth_1>0)//条码上下均找到数字区域
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
			if(max_value2 - min_value2>max_value1-min_value1)//最大最小值之差最小者为数字区域（排除横线）
			{
				/*cvRectangle(iplrtt2, cvPoint(coed_left,number_start_index+code_bottom), cvPoint(coed_right, number_start_index+numberwidth+code_bottom), 
						CV_RGB(0,255,255));*/
				if(2*number_start_index+numberwidth-2+whole_offset>=BottomImageHeight)//超出下届
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
				if(2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset<=0)//超出上届
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
				if(2*number_start_index+numberwidth-2+whole_offset>=BottomImageHeight)//超出下届
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
				if(2*number_start_index_1-numberwidth_1-TopImageHeight+2-whole_offset<=0)//超出上届
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
		

		//剪切数字图像
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
			cvNamedWindow("数字区域");
			cvShowImage("数字区域", iplnl);
		#endif*/


			//确定左右边界
			
			unsigned char  verTical_averagey = 0;
			memset(nPVX_Flag,0,codeWidth*sizeof(int));
			VerTicalProjection(NumberImage,VerTicalNumberImage,codeWidth,NumHeight,codeWidth,&verTical_averagey);
			int n=IPA_getWavePeakValley4(VerTicalNumberImage,codeWidth,0.05,8,0.05,10,nPVX_Flag);
			int bianjie = 0;
			if(n<15)//峰谷阈值 判断为不是数字区域
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
					if(nPVX_Flag[0]<codeWidth-nPVX_Flag[n-1])//寻找不连续区域
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
			cvNamedWindow("数字区域二次切割");
			cvShowImage("数字区域二次切割", iplnlEx);
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
			cvNamedWindow("数字垂直投影");
			cvShowImage("数字垂直投影", debug_proj);

			cvLine(iplnl, cvPoint(left,0), cvPoint(left,512), CV_RGB(255,0,0));
			cvLine(iplnl, cvPoint(right,0), cvPoint(right,512), CV_RGB(255,0,0));
			cvNamedWindow("数字区域");
			cvShowImage("数字区域", iplnl);
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
	int  nNumImageFixHeight = 50;//数字图像缩放固定高度
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
	//将图像缩放至固定
	//图像缩放
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
	cvNamedWindow("数字识别图像");
	cvShowImage("数字识别图像", iplRcgIm);
	cvWaitKey();
	cvReleaseImage(&iplRcgIm);
#endif
#endif

	//输入OCR模块获取结果
	/*vStrResult.clear();	
	vCharCnd.clear();*/
#ifdef _WRITE_LOG
#ifdef _LOG_TRACE
	Write_Log(LOG_TYPE_INFO, "TRACE- +++OCR proc start...");
#endif
#endif
	int err = pOCR->textRecognition( (const unsigned char* )NumberImage, dstHeight, dstWidth, vStrResult, vCharCnd ); // step2:调用接口函数
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
	if(size <10||size>12)//位数限制
		return 0;
	for( int n=1; n<vStrResult.size(); n++ )
	{
		if(vStrResult[n].size()!=size)//结果位数相同
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