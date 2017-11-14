#ifndef _OCR_H_
#define _OCR_H_

#ifdef DLL_API
#else
#define DLL_API /*extern "C"*/ _declspec( dllimport )
#endif

#include <vector>
#include <string>

using namespace std;

class CTextRecPal;

class DLL_API OCR
{
public:
	OCR(void);
	~OCR(void);

	int loadModels( const char* dictName_csp, const char* dictName_nn );
	int textRecognition( const unsigned char* imgGray, int hei, int wid, vector<string> &vStrResult, vector<string> &vCharCnd  );

private:
	CTextRecPal* pTextRecPal;
};

#endif

