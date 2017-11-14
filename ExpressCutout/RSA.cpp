#include "stdafx.h"
#include "RSA.h"
#include<stdio.h>   
#include<string.h>   
#include <stdlib.h>        
#include <math.h>   
#include <malloc.h> 
#include <time.h>

using namespace std;

#define LEN sizeof(struct slink) 

static int is_prime_san(int p[MAX_CODE_NUM]);   
static void primeRandom(int *p,int *q);   
static void erand(int e[MAX_CODE_NUM],int m[MAX_CODE_NUM]);   
static int  cmp(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM]);
static int coprime(int e[MAX_CODE_NUM],int s[MAX_CODE_NUM]);						/*求两个大数之间是否互质*/   
static void mov(int a[MAX_CODE_NUM],int *b);   
static void add(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c);  
static void sub(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c);
static void mul(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c);   
static void  divt(int t[MAX_CODE_NUM],int b[MAX_CODE_NUM],int *c ,int *w);		/*试商法//调用以后w为a mod b, C为a  div b;*/   
static void  mod(int a[MAX_CODE_NUM],int b[MAX_CODE_NUM],int *c);				/*c=a mod b//此处A和C的数组都改变了*/   
static void mulmod(int a[MAX_CODE_NUM] ,int b[MAX_CODE_NUM] ,int n[MAX_CODE_NUM],int *m);	/*解决 了 m=a*b mod n;*/   
static void expmod(int a[MAX_CODE_NUM] ,int p[MAX_CODE_NUM] ,int n[MAX_CODE_NUM],int *m);   
void rsad(int e[MAX_CODE_NUM],int g[MAX_CODE_NUM],int *d);  
static unsigned long  rsa(unsigned long p,unsigned long q,unsigned long e);/*求解密密钥d的函数(根据Euclid算法)*/   
std::wstring Format(const wchar_t *format, ...);




int is_prime_san(int p[MAX_CODE_NUM] )   
{   
	int i,a[MAX_CODE_NUM],t[MAX_CODE_NUM],s[MAX_CODE_NUM],o[MAX_CODE_NUM]; 

	memset(s,0,MAX_CODE_NUM*sizeof(int));
	memset(o,0,MAX_CODE_NUM*sizeof(int));
	memset(a,0,MAX_CODE_NUM*sizeof(int));
	memset(t,0,MAX_CODE_NUM*sizeof(int));

	t[0]=1;   
	t[MAX_CODE_NUM-1]=1;   
	a[0]=2;// { 2,3,5,7 }   
	a[MAX_CODE_NUM-1]=1;   
	sub(p,t,s);   
	expmod ( a, s, p ,o);   
	if ( cmp(o,t) != 0 )    
	{   
		return 0;   
	}   

	a[0]=3;   
	for(i=0;i<MAX_CODE_NUM;i++)  
		o[i]=0;   

	expmod ( a, s, p ,o);   
	if ( cmp(o,t) != 0 )           
	{   
		return 0;   
	}   

	a[0]=5;   
	for(i=0;i<MAX_CODE_NUM;i++)  
		o[i]=0;   

	expmod ( a, s, p ,o);   
	if ( cmp(o,t) != 0 )    
	{          
		return 0;   
	}   

	a[0]=7;   
	for(i=0;i<MAX_CODE_NUM;i++)  
		o[i]=0;   

	expmod ( a, s, p ,o);   

	if ( cmp(o,t) != 0 )    
	{   
		return 0;   
	}   

	return 1;   
}   

void primeRandom(int *p,int *q)   
{   
	int i,k;   
	time_t t;    
	p[0]=1;   
	q[0]=3;   
        
//  p[19]=1;   
//  q[18]=2;   
       
    p[MAX_CODE_NUM-1]=10;   
    q[MAX_CODE_NUM-1]=11;   
   
    do   
    {   
		t=time(NULL);   
		srand((unsigned long)t);   
		for(i=1;i<p[MAX_CODE_NUM-1]-1;i++)   
		{   
			k=rand()%10;   
			p[i]=k;   
		}   
		k=rand()%10;   
		while (k==0)   
		{   
			k=rand()%10;   
		}   
		p[p[MAX_CODE_NUM-1]-1]=k;   
    }while((is_prime_san(p))!=1); 

  /*  printf("素数 p 为  : ");   
    for(i=0;i<p[MAX_CODE_NUM-1];i++)   
    {   
		printf("%d",p[p[MAX_CODE_NUM-1]-i-1]);   
    }   
    printf("\n\n");*/

    do   
    {   
		t=time(NULL);   
		srand((unsigned long)t);   
		for(i=1;i<q[MAX_CODE_NUM-1];i++)   
		{   
			k=rand()%10;   
			q[i]=k;   
		}   
    }while((is_prime_san(q))!=1);

  /*  printf("素数 q 为 : ");   
    for(i=0;i<q[MAX_CODE_NUM-1];i++)   
    {   
		printf("%d",q[q[MAX_CODE_NUM-1]-i-1]);   
    }   
    printf("\n\n");   */
    return;   
}     


void erand(int e[MAX_CODE_NUM],int m[MAX_CODE_NUM])   
{   
    int i,k;   
    time_t t;   
    e[MAX_CODE_NUM-1]=5;   
    do   
    {   
        t=time(NULL);   
		srand((unsigned long)t);   
		for(i=0;i<e[MAX_CODE_NUM-1]-1;i++)   
		{   
			k=rand()%10;   
			e[i]=k;   
		}   
		while((k=rand()%10)==0)   
			k=rand()%10;   
		e[e[MAX_CODE_NUM-1]-1]=k;   
    }while(coprime( e, m)!=1);

	/*printf("随机产生一个与(p-1)*(q-1)互素的 e :");   
    for(i=0;i<e[MAX_CODE_NUM-1];i++)   
    {   
		printf("%d",e[e[MAX_CODE_NUM-1]-i-1]);   
    }   
    printf("\n\n");   */
    return ;   
}   

int  cmp(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM])   
{      
	int l1, l2,i;   

	l1=a1[99];   
	l2=a2[99];  

	if (l1>l2)   
		return 1;   
	else if (l1<l2)   
		return -1; 

	for(i=(l1-1);i>=0;i--)   
	{   
		if (a1[i]>a2[i])   
			return 1 ;   
		else if (a1[i]<a2[i])   
			return -1;   
	}   

	return 0;   
}   

int coprime(int e[MAX_CODE_NUM],int s[MAX_CODE_NUM]) /*//// 求两个大数之间是否互质////*/   
{   
    int i,a[MAX_CODE_NUM],b[MAX_CODE_NUM],c[MAX_CODE_NUM],d[MAX_CODE_NUM],o[MAX_CODE_NUM],l[MAX_CODE_NUM];   


	memset(l,0,MAX_CODE_NUM*sizeof(int));
	memset(o,0,MAX_CODE_NUM*sizeof(int));
	memset(c,0,MAX_CODE_NUM*sizeof(int));
	memset(d,0,MAX_CODE_NUM*sizeof(int));

    o[0]=0;o[MAX_CODE_NUM-1]=1;   
    l[0]=1;l[MAX_CODE_NUM-1]=1;   
    mov(e,b);   
    mov(s,a);   
    do   
    {   
        if(cmp(b,l)==0)   
        {   
            return 1;   
        }   
        for(i=0;i<MAX_CODE_NUM;i++)   
            c[i]=0;   
        divt(a,b,d,c);   
        mov(b,a);/*b--->a*/   
        mov(c,b);/*c--->b*/   
    }   
    while(cmp(c,o)!=0);/*   printf("Ihey are  not coprime!\n");*/   
    return 0;   
}   
//复制a->b
void mov(int a[MAX_CODE_NUM],int *b)   
{   
	memcpy(b,a,MAX_CODE_NUM*sizeof(int));
} 


void add(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c)   
{   
	int i,l1,l2,k,len,temp[MAX_CODE_NUM];   

	k=0;   
	l1=a1[MAX_CODE_NUM-1];   
	l2=a2[MAX_CODE_NUM-1];   
	if((a1[MAX_CODE_NUM-2]=='-')&&(a2[MAX_CODE_NUM-2]=='-'))   
	{   
		c[MAX_CODE_NUM-2]='-';   
	}   
	else if (a1[MAX_CODE_NUM-2]=='-')   
	{   
		mov(a1,temp);   
		temp[MAX_CODE_NUM-2]=0;   
		sub(a2,temp,c);   
		return;   
	}   
	else if (a2[MAX_CODE_NUM-2]=='-')   
	{   
		mov(a2,temp);   
		temp[98]=0;   
		sub(a1,temp,c);   
		return;   
	}  

	if(l1<l2)   
		len=l1;   
	else    
		len=l2;  

	for(i=0;i<len;i++)   
	{   
		c[i]=(a1[i]+a2[i]+k)%10;   
		k=(a1[i]+a2[i]+k)/10;   
	}  

	if(l1>len)   
	{   
		for(i=len;i<l1;i++)   
		{   
			c[i]=(a1[i]+k)%10;   
			k=(a1[i]+k)/10;      
		}  

		if(k!=0)   
		{   
			c[l1]=k;   
			len=l1+1;   
		}   
		else   
			len=l1;   
	}   
	else   
	{   
		for(i=len;i<l2;i++)   
		{   
			c[i]=(a2[i]+k)%10;   
			k=(a2[i]+k)/10;      
		}   

		if(k!=0)   
		{   
			c[l2]=k;   
			len=l2+1;   
		}   
		else   
			len=l2;   
	}   

	c[99]=len;     
}   


void sub(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c)   
{   
	int i,l1,l2,k,len,t1[MAX_CODE_NUM],t2[MAX_CODE_NUM];  

	k=0;   
	l1=a1[MAX_CODE_NUM-1];   
	l2=a2[MAX_CODE_NUM-1];   
	if ((a1[MAX_CODE_NUM-2]=='-') && (a2[MAX_CODE_NUM-2]=='-'))   
	{   
		mov(a1,t1);   
		mov(a2,t2);   
		t1[MAX_CODE_NUM-2]=0;   
		t2[MAX_CODE_NUM-2]=0;   
		sub(t2,t1,c);   
		return;   
	}   
	else if( a2[MAX_CODE_NUM-2]=='-')   
	{   
		mov(a2,t2);   
		t2[MAX_CODE_NUM-2]=0;   
		add(a1,t2,c);   
		return;   
	}   
	else if (a1[MAX_CODE_NUM-2]=='-')   
	{   
		mov(a2,t2);   
		t2[MAX_CODE_NUM-2]='-';   
		add(a1,t2,c);   
		return;   
	}   

	if(cmp(a1,a2)==1)   
	{   
		len=l2;   
		for(i=0;i<len;i++)   
		{   
			if ((a1[i]-k-a2[i])<0)   
			{   
				c[i]=(a1[i]-a2[i]-k+10)%10;   
				k=1;   
			}   
			else    
			{   
				c[i]=(a1[i]-a2[i]-k)%10;   
				k=0;   
			}   
		} 

		for(i=len;i<l1;i++)   
		{   
			if ((a1[i]-k)<0)   
			{   
				c[i]=(a1[i]-k+10)%10;   
				k=1;   
			}   
			else    
			{   
				c[i]=(a1[i]-k)%10;   
				k=0;   
			}    
		} 

		if(c[l1-1]==0)/*使得数组C中的前面所以0字符不显示了，如1000-20=0980--->显示为980了*/   
		{   
			len=l1-1;   
			i=2;   
			while (c[l1-i]==0)/*111456-111450=00006，消除0后变成了6；*/   
			{   
				len=l1-i;   
				i++;   
			}   
		}   
		else    
		{   
			len=l1;   
		}   

	}   
	else if(cmp(a1,a2)==(-1))   
	{   
		c[MAX_CODE_NUM-2]='-';   
		len=l1;   

		for(i=0;i<len;i++)   
		{   
			if ((a2[i]-k-a1[i])<0)   
			{   
				c[i]=(a2[i]-a1[i]-k+10)%10;   
				k=1;   
			}   
			else    
			{   
				c[i]=(a2[i]-a1[i]-k)%10;   
				k=0;   
			}   
		}   

		for(i=len;i<l2;i++)   
		{   
			if ((a2[i]-k)<0)   
			{   
				c[i]=(a2[i]-k+10)%10;   
				k=1;   
			}   
			else    
			{   
				c[i]=(a2[i]-k)%10;   
				k=0;   
			}      
		}  

		if(c[l2-1]==0)   
		{     
			len=l2-1;   
			i=2;   
			while (c[l1-i]==0)   
			{   
				len=l1-i;   
				i++;   
			}   

		}   
		else 
			len=l2;   
	}   
	else   
	{   
		len=1;   
		c[len-1]=0;   
	}   
	c[MAX_CODE_NUM-1]=len;     
}   

void mul(int a1[MAX_CODE_NUM],int a2[MAX_CODE_NUM],int *c)   
{   
	int i,j,x,y,z,w,l1,l2;   

	l1=a1[MAX_CODE_NUM-1];   
	l2=a2[MAX_CODE_NUM-1];   

	if (a1[MAX_CODE_NUM-2]=='-'&& a2[MAX_CODE_NUM-2]=='-')   
		c[MAX_CODE_NUM-2]=0;   
	else if (a1[MAX_CODE_NUM-2]=='-')   
		c[MAX_CODE_NUM-2]='-';   
	else if (a2[MAX_CODE_NUM-2]=='-')   
		c[MAX_CODE_NUM-2]='-';   

	for(i=0;i<l1;i++)   
	{   
		for(j=0;j<l2;j++)   
		{   
			x=a1[i]*a2[j];  //积 
			y=x/10;			//排除个位的数
			z=x%10;			//个位
			w=i+j;			//序号和 
			c[w]+=z;		//加上个位
			c[w+1]+=y+c[w]/10;   //排除个位的数 + 加上个位之后的（排除个位的数）
			c[w]=c[w]%10;   //加上个位之后的个位
		}   
	}  

	w=l1+l2;   
	if(c[w-1]==0)   
		w=w-1;   
	c[MAX_CODE_NUM-1]=w;     
}    


void divt(int t[MAX_CODE_NUM],int b[MAX_CODE_NUM],int  *c ,int *w)/*//试商法//调用以后w为a mod b, C为a  div b;*/   
{   
	int a1,b1,i,j,m;/*w用于暂时保存数据*/   
	int d[MAX_CODE_NUM],e[MAX_CODE_NUM],f[MAX_CODE_NUM],g[MAX_CODE_NUM],a[MAX_CODE_NUM];   

	mov(t,a); 

	memset(e,0,MAX_CODE_NUM*sizeof(int));
	memset(d,0,MAX_CODE_NUM*sizeof(int));
	memset(g,0,MAX_CODE_NUM*sizeof(int));

	a1=a[MAX_CODE_NUM-1];   
	b1=b[MAX_CODE_NUM-1];   
	if (cmp(a,b)==(-1))   
	{   
		c[0]=0;   
		c[MAX_CODE_NUM-1]=1;   
		mov(t,w);   
		return;   
	}   
	else if (cmp(a,b)==0)   
	{   
		c[0]=1;   
		c[MAX_CODE_NUM-1]=1;   
		w[0]=0;   
		w[MAX_CODE_NUM-1]=1;   
		return;   
	}   

	m=(a1-b1);   
	for(i=m;i>=0;i--)/*341245/3=341245-300000*1--->41245-30000*1--->11245-3000*3--->2245-300*7--->145-30*4=25--->25-3*8=1*/   
	{   
		memset(d,0,MAX_CODE_NUM*sizeof(int));
		d[i]=1;   
		d[MAX_CODE_NUM-1]=i+1;   
		mov(b,g);   
		mul(g,d,e);   
		while (cmp(a,e)!=(-1))   
		{   
			c[i]++;   
			sub(a,e,f);   
			mov(f,a);/*f复制给g*/   
		}   

		for(j=i;j<MAX_CODE_NUM;j++)/*高位清零*/   
			e[j]=0;   
	}   
	mov(a,w);   
	if (c[m]==0) 
		c[MAX_CODE_NUM-1]=m;   
	else 
		c[MAX_CODE_NUM-1]=m+1;   
}      

void mod(int a[MAX_CODE_NUM],int b[MAX_CODE_NUM],int  *c)/*/c=a mod b//此处A和C的数组都改变了*/   
{      
    int d[MAX_CODE_NUM];   
    mov(a,d);   
    while (cmp(d,b)!=(-1))/*/c=a-b-b-b-b-b.......until(c<b)*/   
    {   
        sub(d,b,c);   
        mov(c,d);/*/c复制给a*/   
    }      
}   

void mulmod(int a[MAX_CODE_NUM] ,int b[MAX_CODE_NUM] ,int n[MAX_CODE_NUM],int *m)/*解决 了 m=a*b mod n;*/   
{   
    int i,c[MAX_CODE_NUM],d[MAX_CODE_NUM];  

	memset(d,0,MAX_CODE_NUM*sizeof(int));
	memset(c,0,MAX_CODE_NUM*sizeof(int));
    mul(a,b,c);   
    divt(c,n, d,m);   
  /*  for(i=0;i<m[MAX_CODE_NUM-1];i++)   
        printf("%d",m[m[MAX_CODE_NUM-1]-i-1]);   
    printf("\nm  length is :  %d \n",m[MAX_CODE_NUM-1]);   */
}   

/*接下来的重点任务是要着手解决 m=a^p  mod n的函数问题。*/   
void expmod(int a[MAX_CODE_NUM] ,int p[MAX_CODE_NUM] ,int n[MAX_CODE_NUM],int *m)   
{   
    int t[MAX_CODE_NUM],l[MAX_CODE_NUM],temp[MAX_CODE_NUM]; /*/t放入2，l放入1；*/   
    int w[MAX_CODE_NUM],s[MAX_CODE_NUM],c[MAX_CODE_NUM],b[MAX_CODE_NUM],i; 

	memset(b,0,MAX_CODE_NUM*sizeof(int));
	memset(l,0,MAX_CODE_NUM*sizeof(int));
	memset(w,0,MAX_CODE_NUM*sizeof(int));
	memset(t,0,MAX_CODE_NUM*sizeof(int));

    t[0]=2;t[MAX_CODE_NUM-1]=1;   
    l[0]=1;l[MAX_CODE_NUM-1]=1;   
    mov(l,temp);   
    mov(a,m);   
    mov(p,b);   
 
    while(cmp(b,l)!=0)   
    {   
        for(i=0;i<MAX_CODE_NUM;i++)   
			w[i]=c[i]=0;   
        divt(b,t,w,c);/*// c=p mod 2  w= p /2*/    
        mov(w,b);/*//p=p/2*/   
        if(cmp(c,l)==0) /*/余数c==1*/   
        {   
            for(i=0;i<MAX_CODE_NUM;i++)   
                w[i]=0;   
            mul(temp,m,w);   
            mov(w,temp);       
            for(i=0;i<MAX_CODE_NUM;i++)   
                w[i]=c[i]=0;   
            divt(temp,n,w,c);/* /c为余c=temp % n，w为商w=temp/n */   
            mov(c,temp);   
        }   
        for(i=0;i<MAX_CODE_NUM;i++)   
            s[i]=0;   
        mul(m,m,s);//s=a*a   
        for(i=0;i<MAX_CODE_NUM;i++)   
            c[i]=0;   
        divt(s,n,w,c);/*/w=s/n;c=s mod n*/   
        mov (c,m);   
    }   
    for(i=0;i<MAX_CODE_NUM;i++)   
        s[i]=0;   
    mul(m,temp,s);   
    for(i=0;i<MAX_CODE_NUM;i++)   
        c[i]=0;   
    divt(s,n,w,c);   
    mov (c,m);/*余数s给m*/   
    m[MAX_CODE_NUM-2]=a[MAX_CODE_NUM-2];/*为后面的汉字显示需要，用第99位做为标记*/   
    return;/*/k=temp*k%n;*/   
}   

void rsad(int e[MAX_CODE_NUM],int g[MAX_CODE_NUM],int *d)   
{   
    int r[MAX_CODE_NUM],n1[MAX_CODE_NUM],n2[MAX_CODE_NUM],k[MAX_CODE_NUM],w[MAX_CODE_NUM];   
    int i,t[MAX_CODE_NUM],b1[MAX_CODE_NUM],b2[MAX_CODE_NUM],temp[MAX_CODE_NUM];   
    mov(g,n1);   
    mov(e,n2);   
    for(i=0;i<MAX_CODE_NUM;i++)   
        k[i]=w[i]=r[i]=temp[i]=b1[i]=b2[i]=t[i]=0;   
    b1[MAX_CODE_NUM-1]=0;b1[0]=0;/*/b1=0;*/   
    b2[MAX_CODE_NUM-1]=1;b2[0]=1;/*/b2=1;*/   
    while(1)   
    {   
   
		for(i=0;i<MAX_CODE_NUM;i++)   
			k[i]=w[i]=0;   
		divt(n1,n2,k,w);/*/k=n1/n2;*/   
		for(i=0;i<MAX_CODE_NUM;i++)   
			temp[i]=0;   
		mul(k,n2,temp);/*/temp=k*n2;*/   
		for(i=0;i<MAX_CODE_NUM;i++)   
			r[i]=0;   
        sub(n1,temp,r);   
   
        if((r[MAX_CODE_NUM-1]==1) && (r[0]==0))/*/r=0*/   
        {   
            break;   
        }   
        else   
        {   
            mov(n2,n1);/*/n1=n2;*/   
            mov( r,n2);/*/n2=r;*/   
            mov(b2, t);/*/t=b2;*/   
            for(i=0;i<MAX_CODE_NUM;i++)   
              temp[i]=0;   
            mul(k,b2,temp);/*/b2=b1-k*b2;*/   
            for(i=0;i<MAX_CODE_NUM;i++)   
              b2[i]=0;   
            sub(b1,temp,b2);   
            mov(t,b1);   
        }   
    }   
   
    for(i=0;i<MAX_CODE_NUM;i++)   
        t[i]=0;   
    add(b2,g,t);   
    for(i=0;i<MAX_CODE_NUM;i++)   
        temp[i]=d[i]=0;   
    divt(t,g,temp,d);   
   /* printf("由以上的(p-1)*(q-1)和 e 计算得出的 d : ");   
    for(i=0;i<d[MAX_CODE_NUM-1];i++)   
		printf("%d",d[d[MAX_CODE_NUM-1]-i-1]);   
    printf("\n\n");   */
}   
/*/求解密密钥d的函数(根据Euclid算法)96403770511368768000*/   
unsigned long rsa(unsigned long p,unsigned long q,unsigned long e)  /*/求解密密钥d的函数(根据Euclid算法)*/   
{   
    unsigned long g,k,r,n1,n2,t;   
    unsigned long b1=0,b2=1;   
   
    g=(p-1)*(q-1);   
    n1=g;   
    n2=e;   
       
    while(1)   
    {   
        k=n1/n2;   
        r=n1-k*n2;   
        if(r!=0)   
        {   
            n1=n2;   
            n2=r;   
            t=b2;   
            b2=b1-k*b2;   
            b1=t;   
        }   
        else   
        {   
            break;   
        }   
    }   
    return (g+b2)%g;   
}  

void printIntArray(int a[MAX_CODE_NUM])
{   
	int i;   
	for(i=0;i<a[MAX_CODE_NUM-1];i++)   
		printf("%d",a[a[99]-i-1]);   
	printf("\n\n");      
} 

void RSA_PrintStruct(struct slink *h)   
{   
	struct slink *p;   
	int i;   
	p=(struct slink * )malloc(LEN);   
	p=h;   
	if(h!=NULL)
	{
		do    
		{   
			for(i=0;i<p->nBigNums[MAX_CODE_NUM-1];i++)   
				printf("%d",p->nBigNums[p->nBigNums[MAX_CODE_NUM-1]-i-1]);   
			p=p->pNext;   
		}while(p!=NULL); 
	}
	printf("\n\n");   
}     

void RSA_Genarate(int e[MAX_CODE_NUM],int d[MAX_CODE_NUM],int n[MAX_CODE_NUM])
{
	int m[MAX_CODE_NUM],p[MAX_CODE_NUM],q[MAX_CODE_NUM],p1[MAX_CODE_NUM],q1[MAX_CODE_NUM];

	memset(m,0,MAX_CODE_NUM*sizeof(int));
	memset(p,0,MAX_CODE_NUM*sizeof(int));
	memset(q,0,MAX_CODE_NUM*sizeof(int));
	memset(p1,0,MAX_CODE_NUM*sizeof(int));
	memset(q1,0,MAX_CODE_NUM*sizeof(int));
	memset(e,0,MAX_CODE_NUM*sizeof(int));
	memset(d,0,MAX_CODE_NUM*sizeof(int));
	memset(n,0,MAX_CODE_NUM*sizeof(int));

	primeRandom(p,q);/*随机产生两个大素数*/   
	mul(p,q,n);  
	mov(p,p1);   
	p1[0]--;         
	mov(q,q1);   
	q1[0]--; 
	mul(p1,q1,m);
	erand(e,m); 
	rsad(e,m,d);   
}

//加载加密key
void RSA_LoadEnKey(char *loadFilename,int e[MAX_CODE_NUM],int n[MAX_CODE_NUM])
{   
    FILE *fp;   
    char str[MAX_CODE_NUM],ch;   
    int i,k;  

	memset(e,0,MAX_CODE_NUM*sizeof(int));
	memset(n,0,MAX_CODE_NUM*sizeof(int));
    fp=fopen(loadFilename,"r");
	if(fp!=NULL)
	{
		k=0;   
		while((ch=fgetc(fp))!=EOF)   
		{      
			if(ch!=' ')   
			{   
				str[k]=ch;   
				k++;   
			}   
			else   
			{   
				for(i=0;i<k;i++)   
				{   
					e[i]=str[k-i-1]-48;   
				}   
				e[MAX_CODE_NUM-1]=k;   
				k=0;   
			}   
		}   
		for(i=0;i<k;i++)   
			n[i]=str[k-i-1]-48; 

		n[MAX_CODE_NUM-1]=k;   
		fclose(fp);   
	}
	else
	{
		printf("Load EnKey File Error!");
	}
}   

//加载解密key
int RSA_LoadDeKey(char *loadFilename,int d[MAX_CODE_NUM],int n[MAX_CODE_NUM])
{   
    FILE *fp;   
    char str[MAX_CODE_NUM],ch;   
    int i,k,num;
	bool bOk;

	memset(d,0,MAX_CODE_NUM*sizeof(int));
	memset(n,0,MAX_CODE_NUM*sizeof(int));
 
    fp=fopen(loadFilename,"r");  
	if(fp!=NULL)
	{ 
		k=0;  
		num=0;
		bOk=false;
		while((ch=fgetc(fp))!=EOF)   
		{      
			if(ch!=' '&&ch!='\n')   
			{   
				str[k]=ch;   
				k++; 
				bOk=false;
			}   
			else  if(num==0&&!bOk)  
			{   
				for(i=0;i<k;i++)   
				{   
					d[i]=str[k-i-1]-48;   
				}   
				d[MAX_CODE_NUM-1]=k;   
				k=0; 
				num++;
				bOk=true;
			} 
			else if (num==1&&!bOk) 
			{
				for(i=0;i<k;i++)   
					n[i]=str[k-i-1]-48;  
				n[MAX_CODE_NUM-1]=k; 
				num++;
				break;
			}
		} 
		if (num==1)
		{
			for(i=0;i<k;i++)   
				n[i]=str[k-i-1]-48;  
			n[MAX_CODE_NUM-1]=k; 
		}
		fclose(fp);   
	}
	else
	{
		printf("Load DeKey File Error!");
		return -1;
	}
	return 0;
}  

//保存加密key
void RSA_SaveEnKey(int e[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename)
{   
    FILE *fp;   
    int  i;    
    char  ch;   
 
    fp=fopen(saveFilename,"w");   
	if (fp!=NULL)
	{
		for(i=0;i<e[MAX_CODE_NUM-1];i++)   
		{   
			ch=e[e[MAX_CODE_NUM-1]-i-1]+48;   
			fputc(ch,fp);   
		}   

		ch='\n';   
		fputc(ch,fp);   

		for(i=0;i<n[MAX_CODE_NUM-1];i++)   
		{   
			ch=n[n[MAX_CODE_NUM-1]-i-1]+48;   
			fputc(ch,fp);   
		}   
		fclose(fp);   
	}
	else
	{
		printf("Save EnKey File Error!");
	}
   
} 

//保存解密key
void RSA_SaveDekey(int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *saveFilename)  
{   
    FILE *fp;   
    int  i;    
    char ch;  

    fp=fopen(saveFilename,"w");  
	if (fp!=NULL)
	{
		for(i=0;i<d[MAX_CODE_NUM-1];i++)   
		{   
			ch=d[d[MAX_CODE_NUM-1]-i-1]+48;   
			fputc(ch,fp);   
		}   

		ch='\n';   
		fputc(ch,fp); 

		for(i=0;i<n[MAX_CODE_NUM-1];i++)   
		{   
			ch=n[n[MAX_CODE_NUM-1]-i-1]+48;   
			fputc(ch,fp);   
		}   
		fclose(fp);   
	}
	else
	{
		printf("Save DeKey File Error!");
	}   
}   






//void RSA_fcEncode(char *cPriFilename,int e[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cCode) 
//{   
//	FILE *fp;   
//	int i,k,count,temp,m;   
//	char ch;   
//	struct slink  *p,*p1,*p2,*h;   
//
//	h=p=p1=p2=(struct slink * )malloc(LEN);   	
//	fp=fopen(cPriFilename,"r");
//	if (fp!=NULL)
//	{
//		count=0;   
//		while((ch=fgetc(fp))!=EOF)   
//		{     
//			m=ch;   
//			k=0;   
//			if(m<0)   
//			{   
//				m=abs(m);/*/把负数取正并且做一个标记*/   
//				p1->nBigNums[MAX_CODE_NUM-2]='0';   
//			}   
//			else   
//			{   
//				p1->nBigNums[MAX_CODE_NUM-2]='1';   
//			}   
//
//			while(m/10!=0)   
//			{   
//				temp=m%10;   
//				m=m/10;   
//				p1->nBigNums[k]=temp;   
//				k++;   
//			}   
//			p1->nBigNums[k]=m;   
//			p1->nBigNums[MAX_CODE_NUM-1]=k+1;   
//			count++;   
//			if(count==1)   
//				h=p1;   
//			else 
//				p2->pNext=p1;   
//			p2=p1;   
//			p1=(struct slink * )malloc(LEN);   
//		}   
//		p2->pNext=NULL;    
//
//
//		k=0;
//		p=p1=(struct slink * )malloc(LEN);   
//		p=h;    
//		if(h!=NULL) 
//		{
//			do    
//			{    
//				expmod( p->nBigNums , e ,n ,p1->nBigNums);   
//				cCode[k++]=p1->nBigNums[MAX_CODE_NUM-2];  
//				if ((p1->nBigNums[MAX_CODE_NUM-1]/10) ==0)/*/判断p1->nBigNums[99]的是否大于十；*/   
//				{   
//					cCode[k++]=0+48;   
//					cCode[k++]=p1->nBigNums[MAX_CODE_NUM-1]+48;   
//				}   
//				else   
//				{   
//					cCode[k++]=p1->nBigNums[MAX_CODE_NUM-1]/10+48;   
//					cCode[k++]=p1->nBigNums[MAX_CODE_NUM-1]%10+48;   
//				}   
//
//				for(i=0;i<p1->nBigNums[MAX_CODE_NUM-1];i++)   
//				{   
//					cCode[k++]=p1->nBigNums[i]+48;   
//				}   
//				p=p->pNext;   
//				p1=(struct slink * )malloc(LEN);   
//			}while(p!=NULL);   
//		}
//		fclose(fp);   
//	}
//}   

//void RSA_fsEncode(char *cPriFilename, int e[MAX_CODE_NUM], int n[MAX_CODE_NUM], char *sCode)
//{   
//	FILE *fp;   
//	int i,k,count,temp,m;   
//	char ch;   
//	struct slink  *p,*p1,*p2,*h;   
//	string sTemp;
//
//	sCode="";
//
//
//	h=p=p1=p2=(struct slink * )malloc(LEN);   	
//	fp=fopen(cPriFilename,"r");
//	if (fp!=NULL)
//	{
//		count=0;   
//		while((ch=fgetc(fp))!=EOF)   
//		{     
//			m=ch;   
//			k=0;   
//			if(m<0)   
//			{   
//				m=abs(m);/*/把负数取正并且做一个标记*/   
//				p1->nBigNums[MAX_CODE_NUM-2]='0';   
//			}   
//			else   
//			{   
//				p1->nBigNums[MAX_CODE_NUM-2]='1';   
//			}   
//
//			while(m/10!=0)   
//			{   
//				temp=m%10;   
//				m=m/10;   
//				p1->nBigNums[k]=temp;   
//				k++;   
//			}   
//			p1->nBigNums[k]=m;   
//			p1->nBigNums[MAX_CODE_NUM-1]=k+1;   
//			count++;   
//			if(count==1)   
//				h=p1;   
//			else 
//				p2->pNext=p1;   
//			p2=p1;   
//			p1=(struct slink * )malloc(LEN);   
//		}   
//		p2->pNext=NULL;    
//
//
//		k=0;
//		p=p1=(struct slink * )malloc(LEN);   
//		p=h;    
//		if(h!=NULL) 
//		{
//			do    
//			{    
//				expmod( p->nBigNums , e ,n ,p1->nBigNums);   
//
//				sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-2]);
//				sCode+=sTemp;
//				if ((p1->nBigNums[MAX_CODE_NUM-1]/10) ==0)/*/判断p1->nBigNums[99]的是否大于十；*/   
//				{   
//					sTemp.Format(_T("%c"),48);
//					sCode+=sTemp;
//					sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]+48);
//					sCode+=sTemp;
//				}   
//				else   
//				{   
//					sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]/10+48);
//					sCode+=sTemp;
//					sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]%10+48);
//					sCode+=sTemp;  
//				}   
//
//				for(i=0;i<p1->nBigNums[MAX_CODE_NUM-1];i++)   
//				{   
//					sTemp.Format(_T("%c"),p1->nBigNums[i]+48);
//					sCode+=sTemp; 
//				}   
//				p=p->pNext;   
//				p1=(struct slink * )malloc(LEN);   
//			}while(p!=NULL);   
//		}
//		fclose(fp);   
//	}
//}   
//
//void RSA_ssEncode(CString sInput,int e[MAX_CODE_NUM], int n[MAX_CODE_NUM],CString &sCode) 
//{   
//	int i,j,k,count,temp,m;   
//	char ch;   
//	struct slink  *p,*p1,*p2,*h;  
//	CString sTemp;
//
//	h=p=p1=p2=(struct slink * )malloc(LEN);  
//
//	sCode="";
//	count=0;  
//	for (i=0;i<sInput.GetLength();i++)
//	{
//		ch=sInput.GetAt(i);
//
//		m=ch;   
//		k=0;   
//		if(m<0)   
//		{   
//			m=abs(m);/*/把负数取正并且做一个标记*/   
//			p1->nBigNums[MAX_CODE_NUM-2]='0';   
//		}   
//		else   
//		{   
//			p1->nBigNums[MAX_CODE_NUM-2]='1';   
//		}   
//
//		while(m/10!=0)   
//		{   
//			temp=m%10;   
//			m=m/10;   
//			p1->nBigNums[k]=temp;   
//			k++;   
//		}   
//		p1->nBigNums[k]=m;   
//		p1->nBigNums[MAX_CODE_NUM-1]=k+1;   
//		count++;   
//		if(count==1)   
//			h=p1;   
//		else 
//			p2->pNext=p1;   
//		p2=p1;   
//		p1=(struct slink * )malloc(LEN);  
//	}
//	p2->pNext=NULL;    
//
//	k=0;
//	p=p1=(struct slink * )malloc(LEN);   
//	p=h;    
//	if(h!=NULL) 
//	{
//		do    
//		{    
//			expmod( p->nBigNums , e ,n ,p1->nBigNums); 
//			sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-2]);
//			sCode+=sTemp;
//			if ((p1->nBigNums[MAX_CODE_NUM-1]/10) ==0)/*/判断p1->nBigNums[99]的是否大于十；*/   
//			{   
//				sTemp.Format(_T("%c"),48);
//				sCode+=sTemp;
//				sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]+48);
//				sCode+=sTemp;
//			}   
//			else   
//			{   
//				sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]/10+48);
//				sCode+=sTemp;
//				sTemp.Format(_T("%c"),p1->nBigNums[MAX_CODE_NUM-1]%10+48);
//				sCode+=sTemp;  
//			}   
//
//			for(j=0;j<p1->nBigNums[MAX_CODE_NUM-1];j++)   
//			{   
//				sTemp.Format(_T("%c"),p1->nBigNums[j]+48);
//				sCode+=sTemp; 
//			}   
//			p=p->pNext;   
//			p1=(struct slink * )malloc(LEN);   
//		}while(p!=NULL);   
//	}
//}   
//
//
//void RSA_fcDecode(char *cCodeFilename,int d[MAX_CODE_NUM], int n[MAX_CODE_NUM],char *cResult)   
//{      
//	FILE *fp;      
//	struct slink *h,*p1,*p2;   
//	char ch;   
//	int i,j,k,m,count,temp;   
//
//
//	fp=fopen(cCodeFilename,"r");
//	if (fp!=NULL)
//	{
//		i=0;   
//		j=3;   
//		count=0;   
//		h=p1=p2=(struct slink * )malloc(LEN);   
//		while((ch=fgetc(fp))!=EOF)   
//		{     
//			m=ch;         
//			if(j==3)   
//			{   
//				p1->nBigNums[MAX_CODE_NUM-2]=m;   
//				j--;   
//			}   
//			else if(j==2)   
//			{   
//				temp=m-48;   
//				j--;   
//			}   
//			else if(j==1)   
//			{   
//				p1->nBigNums[MAX_CODE_NUM-1]=temp*10+m-48;   
//				j--;   
//			}   
//			else if (j==0)   
//			{   
//				p1->nBigNums[i]=m-48;   
//				i++;   
//				if(i==p1->nBigNums[MAX_CODE_NUM-1])   
//				{    
//					i=0;   
//					j=3;   
//					count++;   
//					if (count==1)   
//						h=p1;   
//					else   
//						p2->pNext=p1;   
//					p2=p1;   
//					p1=(struct slink * )malloc(LEN);   
//				}   
//			}   
//		}   
//		p2->pNext=NULL;   
//
//		p2=(struct slink * )malloc(LEN);   
//		p1=h;   
//		k=0;   
//		if(h!=NULL)/*/temp为暂存ASIIC码的int值*/  
//		{
//			do   
//			{   
//				memset(p2->nBigNums,0,MAX_CODE_NUM*sizeof(int)) ;
//				expmod( p1->nBigNums , d ,n ,p2->nBigNums);          
//				temp=p2->nBigNums[0]+p2->nBigNums[1]*10+p2->nBigNums[2]*100;   
//				if (( p2->nBigNums[MAX_CODE_NUM-2])=='0')   
//				{   
//					temp=0-temp;   
//				}/*/转化为正确的ASIIC码，如-78-96形成汉字    */      
//				cResult[k++]=temp;   
//				p1=p1->pNext;   
//				p2=(struct slink * )malloc(LEN);   
//			}while (p1!=NULL);   
//		}
//		fclose(fp);   
//	}
//}   
//
//
//
int RSA_fsDecode(char *cCodeFilename,int d[MAX_CODE_NUM], int n[MAX_CODE_NUM],string &sResult)   
{      
	FILE *fp;      
	struct slink *h,*p1,*p2;   
	char ch;   
	int i,j,k,m,count,temp; 

	sResult="";

	fp=fopen(cCodeFilename,"r");
	if (fp != NULL)
	{
		i = 0;
		j = 3;
		count = 0;
		h = p1 = p2 = (struct slink *)malloc(LEN);
		while ((ch = fgetc(fp)) != EOF)
		{
			m = ch;
			if (j == 3)
			{
				p1->nBigNums[MAX_CODE_NUM - 2] = m;
				j--;
			}
			else if (j == 2)
			{
				temp = m - 48;
				j--;
			}
			else if (j == 1)
			{
				p1->nBigNums[MAX_CODE_NUM - 1] = temp * 10 + m - 48;
				j--;
			}
			else if (j == 0)
			{
				p1->nBigNums[i] = m - 48;
				i++;
				if (i == p1->nBigNums[MAX_CODE_NUM - 1])
				{
					i = 0;
					j = 3;
					count++;
					if (count == 1)
						h = p1;
					else
						p2->pNext = p1;
					p2 = p1;
					p1 = (struct slink *)malloc(LEN);
				}
			}
		}
		p2->pNext = NULL;

		p2 = (struct slink *)malloc(LEN);
		p1 = h;
		k = 0;
		if (h != NULL)/*/temp为暂存ASIIC码的int值*/
		{
			do
			{
				memset(p2->nBigNums, 0, MAX_CODE_NUM*sizeof(int));
				expmod(p1->nBigNums, d, n, p2->nBigNums);
				temp = p2->nBigNums[0] + p2->nBigNums[1] * 10 + p2->nBigNums[2] * 100;
				if ((p2->nBigNums[MAX_CODE_NUM - 2]) == '0')
				{
					temp = 0 - temp;
				}/*/转化为正确的ASIIC码，如-78-96形成汉字    */
				ch = temp;
				//sTemp << ch << endl;
				////sprintf(sTemp,_T("%c"), ch);
				//string rtemp = sTemp.str;
				sResult += ch;
				p1 = p1->pNext;
				p2 = (struct slink *)malloc(LEN);
			} while (p1 != NULL);
		}
		fclose(fp);
	}
	else
	{
		return -1;
	}
	return 0;
}   


//
//
//
//
//void RSA_ssDecode(CString sCode,int d[MAX_CODE_NUM], int n[MAX_CODE_NUM],CString &sResult)   
//{           
//	struct slink *h,*p1,*p2;   
//	char ch;   
//	int i,j,l,k,m,count,temp;  
//	CString sTemp;
//
//	sResult="";
//	l=0;   
//	j=3;   
//	count=0;   
//	h=p1=p2=(struct slink * )malloc(LEN); 
//
//	for (i=0;i<sCode.GetLength();i++)
//	{
//		ch=sCode.GetAt(i);    
//		m=ch;         
//		if(j==3)   
//		{   
//			p1->nBigNums[MAX_CODE_NUM-2]=m;   
//			j--;   
//		}   
//		else if(j==2)   
//		{   
//			temp=m-48;   
//			j--;   
//		}   
//		else if(j==1)   
//		{   
//			p1->nBigNums[MAX_CODE_NUM-1]=temp*10+m-48;   
//			j--;   
//		}   
//		else if (j==0)   
//		{   
//			p1->nBigNums[l]=m-48;   
//			l++;   
//			if(l==p1->nBigNums[MAX_CODE_NUM-1])   
//			{    
//				l=0;   
//				j=3;   
//				count++;   
//				if (count==1)   
//					h=p1;   
//				else   
//					p2->pNext=p1;   
//				p2=p1;   
//				p1=(struct slink * )malloc(LEN);   
//			}   
//		}   
//	}   
//	p2->pNext=NULL;   
//
//	p2=(struct slink * )malloc(LEN);   
//	p1=h;   
//	k=0;   
//	if(h!=NULL)/*/temp为暂存ASIIC码的int值*/  
//	{
//		do   
//		{   
//			memset(p2->nBigNums,0,MAX_CODE_NUM*sizeof(int)) ;
//			expmod( p1->nBigNums , d ,n ,p2->nBigNums);          
//			temp=p2->nBigNums[0]+p2->nBigNums[1]*10+p2->nBigNums[2]*100;   
//			if (( p2->nBigNums[MAX_CODE_NUM-2])=='0')   
//			{   
//				temp=0-temp;   
//			}/*/转化为正确的ASIIC码，如-78-96形成汉字    */   
//
//			ch=temp;
//			sTemp.Format(_T("%c"),ch);
//			sResult+=sTemp;   
//			p1=p1->pNext;   
//			p2=(struct slink * )malloc(LEN);   
//		}while (p1!=NULL);   
//	}
//}   




//对原文进行编码变换
struct slink *RSA_Input(char *cInputs,int nLen) 
{ 
	struct  slink *head,*pCur,*p2;   
	int  i,j,n,m,temp;     
  
	pCur=p2=(struct slink * )malloc(LEN);   
	head=NULL;   

	m=0; 
	for (i=0;i<nLen;i++)
	{
		n=cInputs[i];
		pCur->nBigNums[MAX_CODE_NUM-2]=n<0?'0':'1';   
		n=abs(n);						

		j=0;
		while(n/10!=0)   
		{   
			temp=n%10;   
			n/=10;   
			pCur->nBigNums[j]=temp;   
			j++;   
		}   
		pCur->nBigNums[j]=n;   
		pCur->nBigNums[MAX_CODE_NUM-1]=j+1;   
		m+=1;   
		if(m==1)   
			head=pCur;   
		else 
			p2->pNext=pCur;   
		p2=pCur;   
		pCur=(struct slink * )malloc(LEN);   
	}
	p2->pNext=NULL;    
	return(head);   
}   

struct slink *RSA_Encode(struct slink *pInput,int e[MAX_CODE_NUM],int  n[MAX_CODE_NUM],char *cCodes)
{   
	struct  slink *p,*pCode,*p1,*p2;   
	int m,i,k; 

	m=0;  
    p1=p2=(struct slink* )malloc(LEN);   
    pCode=NULL;   
    p=pInput;   
	k=0;
	if(pInput!=NULL) 
	{
		do    
		{   
			expmod( p->nBigNums , e ,n ,p1->nBigNums);   
			for(i=0;i<p1->nBigNums[MAX_CODE_NUM-1];i++)   
			{   
				cCodes[k++]=p1->nBigNums[p1->nBigNums[MAX_CODE_NUM-1]-1-i];
			}   

			m++;
			if(m==1)   
				pCode=p1;   
			else 
				p2->pNext=p1; 

			p2=p1;   
			p1=(struct slink * )malloc(LEN);   
			p=p->pNext;   
		}while(p!=NULL); 
	}

    p2->pNext=NULL;      
    p=pCode;   
    return(pCode);   
} 

void RSA_Decode(struct  slink *pCode,int d[MAX_CODE_NUM],int n[MAX_CODE_NUM],char *cOutputs)
{   
	int i,m,k;   
	struct slink *p,*p1;   
	
	p1=(struct slink* )malloc(LEN);   
	p=pCode;   
	k=0;
	if(pCode!=NULL)   
	{
        do    
		{   
			for(i=0;i<MAX_CODE_NUM;i++)   
				p1->nBigNums[i]=0;   
	   
			expmod( p->nBigNums , d ,n ,p1->nBigNums);   
	   
			m=p1->nBigNums[0]+p1->nBigNums[1]*10+p1->nBigNums[2]*100;   
			if (( p1->nBigNums[MAX_CODE_NUM-2])=='0')   
			{   
				m=0-m;   
			}   
	           
			cOutputs[k++]=m;
			p=p->pNext;   
		}while (p!=NULL);   
	} 

	if (p1!=NULL)
	{
		free(p1);
	}
}  