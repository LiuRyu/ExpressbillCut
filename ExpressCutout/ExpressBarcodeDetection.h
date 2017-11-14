#ifndef _EXPRESS_BARCODE_DETECTION_H
#define _EXPRESS_BARCODE_DETECTION_H

#define EXPRESS_BARCODE_DETECT_LIBDLL
#ifdef  EXPRESS_BARCODE_DETECT_LIBDLL
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllexport) 
#else
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllimport) 
#endif

// typedef struct tagAlgorithmParamSet
// {
// 	int nFlag;
// 	int nCodeSymbology;	// ��������
// 	int nCodeWidth;		// ������(����)
// 	int nCodeHeight;	// ����߶�(����)
// 	int nCodeDgtNum;	// ����λ��
// 	int nCodeBlkNum;	// �����к�ɫ����
// 	int nGradThre;		// �ݶ���ֵ
// 	int nClusMinNum;	// ����ʱ��С����������ֵ
// 	//int reserve1;		// Ԥ��1
// 	int nCodeCount;  	// �滻Ԥ��1����ʶ����������0-�����ƣ�>0-�涨����
// 	//int reserve2;	 	// Ԥ��2
// 	int nMultiPkg;		// �滻Ԥ��2����������ģ�鿪��
// 	int reserve3;  		// Ԥ��3
// 	int reserve4;  		// Ԥ��4
// 	int reserve5;  		// Ԥ��5
// 	int reserve6;	 	// Ԥ��6
// 	int reserve7;  		// Ԥ��7
// 	int reserve8;  		// Ԥ��8
// } AlgorithmParamSet;

// �㷨�������ýṹ��
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// ��ʶ��������: 0-������, >0-�涨����(��С��MAX_BARCODE_COUNT����ֵ)

	int nCodeSymbology;		// ��������: 0-������, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-����25, (1<<4)-EAN13;
							//   ʹ��"��λ��"����϶�������, ��((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// �������ַ���λ��: 0-������,�㷨�Խ��λ�����޶�;
							//    >0,�㷨�Խ��λ�������޶������֧��4�ֲ�ͬ��λ��
							//    ÿ������λ����byte����ʽ�ֱ𱣴���nCodeDgtNum��31~24λ��23~16λ��15~8λ��7~0λ��
							//    ��������������޶�Ϊ10λ��12λ��13λ������nCodeDgtNum=(10<<16) | (12<<8) | 13

	int nCodeValidity;	 	// �ַ���Ч��: 0-������, (1<<0)-����(ASCII 48~57), (1<<1)-Сд��ĸ(ASCII 97~122), (1<<2)-��д��ĸ(ASCII 65~90)
							//   (1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
							//   (1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
							//   (1<<11)-"("��")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
							//   (1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
							//   (1<<19)-";"(ASCII 59), (1<<20)-"<"��">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
							//   (1<<23)-"@"(ASCII 64), (1<<24)-"["��"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
							//   (1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"��"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
							//   (1<<31)-"~"(ASCII 126)
							//   ʹ��"��λ��"����϶����ַ�����, ��((1<<0) | (1<<1) | (1<<2)) = ֧�ְ������֡�Сд��ĸ�Լ���д��ĸ����������������඼��Ϊ�Ƿ��ַ����й���

	int nCodeValidityExt;  	// �ַ���Ч�����䣬Ԥ�����ã�Ĭ��Ϊ0

	int nMultiPkgDetect;  	// �����Ԥ������: 0-�ر�, ��0-����

	int reserve4;  			// Ԥ��4
} AlgorithmParamSet;

typedef struct tagAlgorithmResult
{
	int  nFlag;				// ʶ���־
	int  nCodeSymbology;	// ��������
	int  nCodeCharNum;		// �ַ�λ��
	char strCodeData[128];	// ������
	int  ptCodeCenter;		// ������������(��16:X,��16:Y)
	int  ptCodeBound1;		// ���붥������1
	int  ptCodeBound2;		// ���붥������2
	int  ptCodeBound3;		// ���붥������3
	int  ptCodeBound4;		// ���붥������4
	int  nCodeOrient;		// ������ת�Ƕ�
	//int  reserve1;  		// Ԥ��1
	int  nCodeWidth;		// �滻Ԥ��1��������
	//int  reserve2;		// Ԥ��2
	int  nCodeHeight;		// �滻Ԥ��2������߶�
	//int  reserve3;  		// Ԥ��3
	int  nCodeModuleWid;	// �滻Ԥ��3�����뵥λ���*1024
	//int  reserve4;  		// Ԥ��4
	int  nCodeSeqNum;		// �滻Ԥ��4���������
	int  reserve5;  		// Ԥ��5
	int  reserve6;	 		// Ԥ��6
	int  reserve7;  		// Ԥ��7
	int  reserve8;  		// Ԥ��8
} AlgorithmResult;


EXPRESS_BARCODE_DETECT_LIBDLL int algorithm_init(int max_width, int max_height, 
										unsigned char ** results);

EXPRESS_BARCODE_DETECT_LIBDLL int algorithm_run(int lrning_flag, unsigned char * in_data, 
										int width, int height, unsigned char ** results);

EXPRESS_BARCODE_DETECT_LIBDLL void algorithm_release();

int algorithm_setparams(AlgorithmParamSet * paramset);

void algorithm_resetparams();

void CoorRelative2Absolute(int centAbs_x, int centAbs_y, int coorRltv_x, int coorRltv_y, 
	unsigned char rttAngle, int * coorAbs_x, int * coorAbs_y);

void CoorAbsolute2Relative(int centAbs_x, int centAbs_y, int coorAbs_x, int coorAbs_y, 
	unsigned char rttAngle, int * coorRltv_x, int * coorRltv_y);

#endif