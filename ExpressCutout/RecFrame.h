#ifndef _FRAME_CUTOUT
#define _FRAME_CUTOUT

int  RecFrame_init(int imageWidth, int imageHeight);
int  RecFrame_init0();
void RecFrame_release();

int	 RecFrame_InputImg( unsigned char * imageData,int width,int height);

int  RecFrame_Judge(int ptx[4],int pty[4],double angle);

#endif