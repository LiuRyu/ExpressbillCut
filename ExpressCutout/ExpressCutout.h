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

// 1--算法初始化
#ifndef INPUTTYPE_FILEPATH
EXPRESSCUTOUT_LIBDLL int ExpressCutout_init(int imageWidth, int imageHeight);
#else
EXPRESSCUTOUT_LIBDLL int ExpressCutout_init(int imageWidth, int imageHeight, char * LogPath);
#endif

/*********************************************************************
* 函数名称: ExpressCutout_set
* 说明: 条码参数设置函数
* 调用者：
* 输入参数:
* int code_count
*	--待识别条码数: [=0]-无限制; [>0]-为code_count所指定的个数(须小于128)
* int code_symbology
*	--待识别条码类型: [=0]-无限制; [=(1<<0)]-code128码; [=(1<<1)]-code39码; [=(1<<2)]-code93码; [=(1<<3)]-交插25码; [=(1<<4)]-EAN13码;
*		注: 可以使用"按位或"来组合多种类型, 如设置同时识别128, 39, EAN13三种类型条码, 可令code_symbology = ((1<<0) | (1<<1) | (1<<4)) = (code128 + code39 + EAN13)
* int code_digit		
*	--解码结果位数: [=0]-无限制,算法对结果位数无限定; [>0]-算法对结果位数进行限定, 最多支持4种不同的位数, 组合方式如下:
*		注: 每个设置位数以byte的形式分别保存在code_digit的31~24位, 23~16位, 15~8位, 7~0位中 (1 int = 4 bytes), 如下所示
*		|31--------24|23--------16|15--------8|7--------0|
*		    digit4       digit3      digit2     digit1
*		可以通过分别设置每个byte的值来组合多种位数, 如设置输出条码限定为10位, 12位及13位, 可令code_digit = (10<<16) | (12<<8) | 13
* int code_validity
*	--字符有效性: 0-无限制, (1<<0)-数字(ASCII 48~57), (1<<1)-小写字母(ASCII 97~122), (1<<2)-大写字母(ASCII 65~90)
*		(1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
*		(1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
*		(1<<11)-"("和")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
*		(1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
*		(1<<19)-";"(ASCII 59), (1<<20)-"<"和">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
*		(1<<23)-"@"(ASCII 64), (1<<24)-"["和"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
*		(1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"和"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
*		(1<<31)-"~"(ASCII 126)
*		注: 使用"按位或"来组合多种字符类型, 如((1<<0) | (1<<1) | (1<<2)) = 支持包含数字、小写字母以及大写字母的条码结果输出，其余都视为非法字符进行过滤
* int code_validityEx
*	--字符有效性扩充项，预留待用，默认为0
* 输出参数：
* 无
* 返回值:
* int  -- 1: 参数设置成功
*      -- 非1: 参数设置失败
* 作者: liuryu
* 时间: 2017-6-15
*********************************************************************/
EXPRESSCUTOUT_LIBDLL int ExpressCutout_set(int code_count, int code_symbology, int code_digit, int code_validity, int code_validityEx);


// 3--算法运行--条形码识别及快递单切割
#ifndef INPUTTYPE_FILEPATH
EXPRESSCUTOUT_LIBDLL int ExpressCutout_run(unsigned char * inputImage, int * imageWidth, int * imageHeight, char * code, int * codeLength);
#else
EXPRESSCUTOUT_LIBDLL int ExpressCutout_run(char * InputImagePath, char * CutOutPath, char * SuccessPath, 
							char * FailPath, char * ErrorPath, char * code, int * codeLength);
#endif

// 4--参数重置--使设置参数失效，算法参数回归默认值
EXPRESSCUTOUT_LIBDLL void ExpressCutout_reset();

// 5--算法释放
EXPRESSCUTOUT_LIBDLL void ExpressCutout_release();

EXPRESSCUTOUT_LIBDLL unsigned char * ExpressCutout_getFullResult();

EXPRESSCUTOUT_LIBDLL int ExpressCutout_getSuccessCount();



#endif


