#ifndef _LOCATE_CODE_NUMBER_H
#define _LOCATE_CODE_NUMBER_H

int LocateCodeNumber_init(int max_wid, int max_hei);
void LocateCodeNumber_release();
int SetCodeNumberImage(int BottomorTop,unsigned char * srcImage,const int sSrcImgWidth,const int sSrcImgHeight);
void VerTicalProjection(unsigned char * const srcImage,unsigned char *dstImage,const int sSrcImgWidth,const int sSrcImgHeight,int widthstep,unsigned char *average_grey);
int IPA_getWavePeakValley4(unsigned char *cWave,int nWaveLen,float fBideGapRatio,float fBideGapTh,float fMidGapRatioTh,float fMidGapTh,int *nPVX_Flag);
int LocateCodeNumber(int codeHeight,unsigned char ** dstImage,int *width, int *height);
int NumberRecoge(int codeHeight,char * code_result,int * code_char_num,int *code_direct);
#endif