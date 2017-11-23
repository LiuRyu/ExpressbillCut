#ifndef _EXPRESS_BARCODE_DETECTION_H
#define _EXPRESS_BARCODE_DETECTION_H

//#define EXPRESS_BARCODE_DETECT_LIBDLL
#ifdef  EXPRESS_BARCODE_DETECT_LIBDLL
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllexport) 
#else
#define EXPRESS_BARCODE_DETECT_LIBDLL extern "C" _declspec(dllimport) 
#endif

// 算法参数设置结构体
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// 待识别条码数: 0-无限制, >0-规定个数(须小于MAX_BARCODE_COUNT给定值，当前为128)

	int nCodeSymbology;		// 条码类型: 0-无限制, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-交插25, (1<<4)-EAN13;
	//   使用"按位或"来组合多种类型, 如((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// 解码结果字符串位数: 0-无限制,算法对结果位数(<=32)无限定;
	//    >0-算法对结果位数(<=32)进行限定，最多支持32种不同的位数(1~32)
	//    (1<<(n-1)) = 支持结果字符串位数为n的条码输出
	//    使用"按位或"来组合多种类型
	//    如设置输出条码限定为1位、12位及32位三种，可令nCodeDgtNum=(1<<0) | (1<<11) | (1<<31)

	int nCodeDgtNumExt;		// 解码结果字符串位数扩充，是[nCodeDgtNum]在大于32位时的扩充，当前无功能，预留待用，默认为0

	int nCodeValidity;	 	// 字符有效性: 0-无限制, (1<<0)-数字(ASCII 48~57), (1<<1)-小写字母(ASCII 97~122), (1<<2)-大写字母(ASCII 65~90)
	//   (1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
	//   (1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
	//   (1<<11)-"("和")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
	//   (1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
	//   (1<<19)-";"(ASCII 59), (1<<20)-"<"和">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
	//   (1<<23)-"@"(ASCII 64), (1<<24)-"["和"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
	//   (1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"和"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
	//   (1<<31)-"~"(ASCII 126)
	//   使用"按位或"来组合多种字符类型, 如((1<<0) | (1<<1) | (1<<2)) = 支持包含数字、小写字母以及大写字母的条码结果输出，其余都视为非法字符进行过滤

	int nCodeValidityExt;  	// 字符有效性扩充，当前无功能，预留待用，默认为0

	int nMultiPkgDetect;  	// 多包裹预警开关: 0-关闭; 非0-开启

} AlgorithmParamSet;


typedef struct tagAlgorithmResult
{
	int  nFlag;				// 识别标志
	int  nCodeSymbology;	// 条码类型
	int  nCodeCharNum;		// 字符位数
	char strCodeData[128];	// 解码结果
	int  ptCodeCenter;		// 条码中心坐标(高16:X,低16:Y)
	int  ptCodeBound1;		// 条码顶点坐标1
	int  ptCodeBound2;		// 条码顶点坐标2
	int  ptCodeBound3;		// 条码顶点坐标3
	int  ptCodeBound4;		// 条码顶点坐标4
	int  nCodeOrient;		// 条码旋转角度
	//int  reserve1;  		// 预留1
	int  nCodeWidth;		// 替换预留1，条码宽度
	//int  reserve2;		// 预留2
	int  nCodeHeight;		// 替换预留2，条码高度
	//int  reserve3;  		// 预留3
	int  nCodeModuleWid;	// 替换预留3，条码单位宽度*1024
	//int  reserve4;  		// 预留4
	int  nCodeSeqNum;		// 替换预留4，条码序号
	int  reserve5;  		// 预留5
	int  reserve6;	 		// 预留6
	int  reserve7;  		// 预留7
	int  reserve8;  		// 预留8
} AlgorithmResult;


EXPRESS_BARCODE_DETECT_LIBDLL int algorithm_init(int max_width, int max_height, 
										unsigned char ** results);

EXPRESS_BARCODE_DETECT_LIBDLL int algorithm_run(int lrning_flag, unsigned char * in_data, 
										int width, int height, unsigned char ** results);

EXPRESS_BARCODE_DETECT_LIBDLL void algorithm_release();

EXPRESS_BARCODE_DETECT_LIBDLL int  algorithm_setparams(AlgorithmParamSet * paramset);

EXPRESS_BARCODE_DETECT_LIBDLL void algorithm_resetparams();

void CoorRelative2Absolute(int centAbs_x, int centAbs_y, int coorRltv_x, int coorRltv_y, 
	unsigned char rttAngle, int * coorAbs_x, int * coorAbs_y);

void CoorAbsolute2Relative(int centAbs_x, int centAbs_y, int coorAbs_x, int coorAbs_y, 
	unsigned char rttAngle, int * coorRltv_x, int * coorRltv_y);

#endif