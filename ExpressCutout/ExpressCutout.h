#ifndef _EXPRESS_CUTOUT
#define _EXPRESS_CUTOUT

#ifdef EXPRESSCUTOUT_LIBDLL
#define EXPRESSCUTOUT_LIBDLL extern "C" _declspec(dllexport) 
#else
#define EXPRESSCUTOUT_LIBDLL extern "C" _declspec(dllimport) 
#endif

#define INPUTTYPE_FILEPATH
#ifdef INPUTTYPE_FILEPATH
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#endif

EXPRESSCUTOUT_LIBDLL int Add(int a, int b);

EXPRESSCUTOUT_LIBDLL void PretreatmentImage(char imageData[], int *imageWidth, int *imageHeight, char* code, int codeLength);

// 1--�㷨��ʼ��
#ifndef INPUTTYPE_FILEPATH
EXPRESSCUTOUT_LIBDLL int ExpressCutout_init(int imageWidth, int imageHeight);
#else
EXPRESSCUTOUT_LIBDLL int ExpressCutout_init(int imageWidth, int imageHeight, char * LogPath);
#endif

/*********************************************************************
* ��������: ExpressCutout_set
* ˵��: ����������ú���
* �����ߣ�
* �������:
* int code_count
*	--��ʶ��������: [=0]-������; [>0]-Ϊcode_count��ָ���ĸ���(��С��128)
* int code_symbology
*	--��ʶ����������: [=0]-������; [=(1<<0)]-code128��; [=(1<<1)]-code39��; [=(1<<2)]-code93��; [=(1<<3)]-����25��; [=(1<<4)]-EAN13��;
*		ע: ����ʹ��"��λ��"����϶�������, ������ͬʱʶ��128, 39, EAN13������������, ����code_symbology = ((1<<0) | (1<<1) | (1<<4)) = (code128 + code39 + EAN13)
* int code_digit		
*	--������λ��: [=0]-������,�㷨�Խ��λ�����޶�; [>0]-�㷨�Խ��λ�������޶�, ���֧��4�ֲ�ͬ��λ��, ��Ϸ�ʽ����:
*		ע: ÿ������λ����byte����ʽ�ֱ𱣴���code_digit��31~24λ, 23~16λ, 15~8λ, 7~0λ�� (1 int = 4 bytes), ������ʾ
*		|31--------24|23--------16|15--------8|7--------0|
*		    digit4       digit3      digit2     digit1
*		����ͨ���ֱ�����ÿ��byte��ֵ����϶���λ��, ��������������޶�Ϊ10λ, 12λ��13λ, ����code_digit = (10<<16) | (12<<8) | 13
* int code_validity
*	--�ַ���Ч��: 0-������, (1<<0)-����(ASCII 48~57), (1<<1)-Сд��ĸ(ASCII 97~122), (1<<2)-��д��ĸ(ASCII 65~90)
*		(1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
*		(1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
*		(1<<11)-"("��")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
*		(1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
*		(1<<19)-";"(ASCII 59), (1<<20)-"<"��">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
*		(1<<23)-"@"(ASCII 64), (1<<24)-"["��"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
*		(1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"��"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
*		(1<<31)-"~"(ASCII 126)
*		ע: ʹ��"��λ��"����϶����ַ�����, ��((1<<0) | (1<<1) | (1<<2)) = ֧�ְ������֡�Сд��ĸ�Լ���д��ĸ����������������඼��Ϊ�Ƿ��ַ����й���
* int code_validityEx
*	--�ַ���Ч�������Ԥ�����ã�Ĭ��Ϊ0
* ���������
* ��
* ����ֵ:
* int  -- 1: �������óɹ�
*      -- ��1: ��������ʧ��
* ����: liuryu
* ʱ��: 2017-6-15
*********************************************************************/
EXPRESSCUTOUT_LIBDLL int ExpressCutout_set(int code_count, int code_symbology, int code_digit, int code_validity, int code_validityEx);


// 3--�㷨����--������ʶ�𼰿�ݵ��и�
#ifndef INPUTTYPE_FILEPATH
EXPRESSCUTOUT_LIBDLL int ExpressCutout_run(unsigned char * inputImage, int * imageWidth, int * imageHeight, char * code, int * codeLength);
#else
EXPRESSCUTOUT_LIBDLL int ExpressCutout_run(char * InputImagePath, char * CutOutPath, char * SuccessPath, 
							char * FailPath, char * ErrorPath, char * code, int * codeLength);
#endif

// 4--��������--ʹ���ò���ʧЧ���㷨�����ع�Ĭ��ֵ
EXPRESSCUTOUT_LIBDLL void ExpressCutout_reset();

// 5--�㷨�ͷ�
EXPRESSCUTOUT_LIBDLL void ExpressCutout_release();

EXPRESSCUTOUT_LIBDLL unsigned char * ExpressCutout_getFullResult();

EXPRESSCUTOUT_LIBDLL int ExpressCutout_getSuccessCount();



#endif


