
#include "videoInput.h"
#include "auto_matrix.h"
#include <list>
#include <omp.h>
#include "colour_transform.h"
#include <stdlib.h>
#include <math.h>                           /* math functions                */

#define M_PI       3.14159265358979323846


//AUXILIAR INLINE TEMPLATE FUNCTIONS
template<typename number> __forceinline double gaussian(number x, number y, number radius)
{
	return exp( (-x*x-y*y)/(double)(2*radius*radius) )/(2*M_PI*radius*radius);
}

template<typename number> __forceinline number min3(number a, number b, number c)
{
	if(a<b)
	{
		if(a<c)
			return a;
		//if(c<a)
			return c;
	}else{
		if(b<c)
			return b;
		//if(c<b)
			return c;
	}
}

unsigned char __forceinline acot8(int d)
{
	if(d>0xff)
		return 0xff;
	if(d<0)
		return 0;
	return d;
}

template<typename number> __forceinline number pow2(number X){return X*X;}

//eigenvalues:
//
//    a  b
//    c  d
//

template<typename number> __forceinline void eigenvalues(double eigenval_out[2], number a, number b, number c, number d)
{
	double x1=(a+d)/2.0;
	double x2=sqrt((double)(4*b*c-pow2(a-d)))/2.0;
	eigenval_out[0] = x1+x2;
	eigenval_out[1] = x1-x2;
}

template<typename number> __forceinline void eigenvalues2_2(double eigenval_out[2], number a, number b, number c, number d)
{
	double x1=pow2(a+d);
	double x2=(double)(4*b*c-pow2(a-d));
	eigenval_out[0] = x1+x2;
	eigenval_out[1] = x1-x2;
}

void __forceinline Pset(MAT_RGB & m, int ix, int iy, int radius=1, int r=255, int g=0, int b=0)
{
	int height=m.ySize();
	int width=m.xSize();
	int jx,jy;
	for(jy=max(iy-radius,0);jy<min(iy+radius+1,height);jy++)
		for(jx=max(ix-radius,0);jx<min(ix+radius+1,width);jx++)
			m(jx,jy)[0]=b,m(jx,jy)[1]=g,m(jx,jy)[2]=r;
}

void __forceinline Cset(MAT_RGB & m, int ix, int iy, int N_rel_c, const int* rel_c, unsigned char center, unsigned char *circle, int r=255, int g=255, int b=255)
{
	int i,jx=ix,jy=iy;
	if(center>127){
		m(jx,jy)[0]=b,m(jx,jy)[1]=0,m(jx,jy)[2]=r;
	}else{
		m(jx,jy)[0]=0,m(jx,jy)[1]=g,m(jx,jy)[2]=r;
	}
	for (i=0;i<N_rel_c-1;i++)
	{
		for (int sx=1;sx>=-1;sx-=2)
			for (int sy=sx;sx*sy>=-1;sy-=2*sx)
		{
			int ni=N_rel_c-i-1;
			bool bi=sx==sy;
			jx=ix+rel_c[bi?i:ni]*sx;
			jy=iy+rel_c[bi?ni:i]*sy;
			if(circle[i+(N_rel_c-1)*((3-sx*sy)/2-sx)]>127){
				m(jx,jy)[0]=b,m(jx,jy)[1]=0,m(jx,jy)[2]=r;
			}else{
				m(jx,jy)[0]=0,m(jx,jy)[1]=g,m(jx,jy)[2]=r;
			}
		}
	}
}

void __forceinline Cset2(MAT_RGB & m, int ix, int iy, int N_rel_c, const int* rel_c, int thr, int *circle, int r=255, int g=255, int b=255)
{
	int i,jx=ix,jy=iy;
	m(jx,jy)[0]=0,m(jx,jy)[1]=g,m(jx,jy)[2]=r;

	for (i=0;i<N_rel_c-1;i++)
	{
		for (int sx=1;sx>=-1;sx-=2)
			for (int sy=sx;sx*sy>=-1;sy-=2*sx)
		{
			int ni=N_rel_c-i-1;
			bool bi=sx==sy;
			jx=ix+rel_c[bi?i:ni]*sx;
			jy=iy+rel_c[bi?ni:i]*sy;
			if(circle[i+(N_rel_c-1)*((3-sx*sy)/2-sx)]>thr){
				m(jx,jy)[0]=b,m(jx,jy)[1]=0,m(jx,jy)[2]=r;
			}else{
				m(jx,jy)[0]=0,m(jx,jy)[1]=g,m(jx,jy)[2]=r;
			}
		}
	}
}


//GET PIXELS (WINDOWS)
void get_webcam_pixels
(int & width,
int & height,
MAT_RGB & buf,
int device,
videoInput & VI)
{
	//get webcam pixels
	width = VI.getWidth(device);
	height = VI.getHeight(device);
	ASSERT(VI.getSize(device)/width/height==3);
	buf.create(width,height);
	VI.getPixels(device, (unsigned char*)((void*)buf), false, true);
}


//unused:::
void test_case_pixels
(int width,
int height,
MAT_RGB & buf)
{
	//test case pixels
	int ix,iy;
	width = 56;
	height = 58;
	buf.create(width,height);
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			if(ix>15 && ix<35 && iy>15 && iy<35)
				{buf(ix,iy)[0]=255;buf(ix,iy)[1]=255;buf(ix,iy)[2]=255;}
			else
				{buf(ix,iy)[0]=0;buf(ix,iy)[1]=0;buf(ix,iy)[2]=0;}
}

void get_resized_grayscale
(int & width,
int & height,
MAT_RGB & in,
MAT_GRAYSCALE & out,
int infactor=8)
{
	//process image: get a resized average-component image
	int ix,iy;
	width/=infactor;
	height/=infactor;
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=(in(ix*infactor,iy*infactor)[2]+in(ix*infactor,iy*infactor)[1]+in(ix*infactor,iy*infactor)[0])/3;
}

void get_resized_min
(int & width,
int & height,
MAT_RGB & in,
MAT_GRAYSCALE & out,
int infactor=8)
{
	//process image: get a resized min-component image
	int ix,iy;
	width/=infactor;
	height/=infactor;
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=(in(ix*infactor,iy*infactor)[2]+in(ix*infactor,iy*infactor)[1]+in(ix*infactor,iy*infactor)[0])/3;
}

void get_resized
(int & width,
int & height,
MAT_RGB & in,
MAT_RGB & out,
int infactor=8)
{
	//process image: get a resized image
	int ix,iy,i;
	width/=infactor;
	height/=infactor;
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			for(i=0;i<3;i++)
				out(ix,iy)[i]=in(ix*infactor,iy*infactor)[i];
}

void get_grayscale
(int width,
int height,
MAT_RGB & in,
MAT_GRAYSCALE & out)
{
	//process image: get a grayscale image (avg)
	int ix,iy;
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=(in(ix,iy)[2]+in(ix,iy)[1]+in(ix,iy)[0])/3;
}

void get_min_component
(int width,
int height,
MAT_RGB & in,
MAT_GRAYSCALE & out)
{
	//process image: get a min-component image
	int ix,iy;
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=min3(in(ix,iy)[2],in(ix,iy)[1],in(ix,iy)[0]);
}

void get_gradients
(int width,
int height,
MAT_GRAYSCALE & in,
MAT_GRAYSCALE & out,
MAT_GRAYSCALE & xdiff,
MAT_GRAYSCALE & ydiff)
{
	//compute vertical/horizontal differences
	int ix,iy;
	xdiff.create(width,height);
	ydiff.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
	{
		for(ix=0;ix<width-1;ix++)
			xdiff(ix,iy)=abs(in(ix,iy)-(int)in(ix+1,iy))/2;
		xdiff(width-1,iy)=0;
	}
	#pragma omp parallel for private(ix,iy)
	for(ix=0;ix<width;ix++)
	{
		for(iy=0;iy<height-1;iy++)
			ydiff(ix,iy)=abs(in(ix,iy)-(int)in(ix,iy+1))/2;
		ydiff(ix,height-1)=0;
	}

	//gradient (edges)
	out.create(width,height);
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=sqrt((float)(xdiff(ix,iy)*(int)xdiff(ix,iy)+ydiff(ix,iy)*(int)ydiff(ix,iy)));
}

//intensification to be used on gradient maps
void intensify_square
(int width,
int height,
MAT_GRAYSCALE & out)
{
	//intensify
	int ix,iy;
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=255-(unsigned char)((255-out(ix,iy))*(int)(255-out(ix,iy)));
}

void filter_threshold
(int width,
int height,
MAT_GRAYSCALE & out,
unsigned char threshold = 205)
{
	//threshold
	int ix,iy;
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=out(ix,iy)>threshold?255:0;	
}

void filter_unused00
(int width,
int height,
MAT_GRAYSCALE & out,
MAT_RGB & orig,
unsigned char threshold = 205)
{
	//intensify3
	int ix,iy;
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)=(out(ix,iy)>threshold&&orig(ix,iy)[0]<orig(ix,iy)[2]+int(20)&&orig(ix,iy)[1]<orig(ix,iy)[2]+(int)20)?255:0;	
}

void get_filter_gaussian
(int width,
int height,
MAT_GRAYSCALE & in,
MAT_GRAYSCALE & out,
int radius=1)
{
	////gaussian blurr
	int ix,iy;
	out.create(width,height);
	//build gaussian
	matrix_tag<unsigned int> gauss(-radius,radius,-radius,radius);
	#pragma omp parallel for private(ix,iy)
	for(iy=-radius;iy<1+radius;iy++)
		for(ix=-radius;ix<1+radius;ix++)
			gauss(ix,iy)=1/gaussian(ix,iy,radius);
	//apply it
	int jx,jy;
	out.zeroMem();
	#pragma omp parallel for private(ix,iy,jx,jy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			for(jy=max(iy-radius,0);jy<min(iy+radius+1,height);jy++)
				for(jx=max(ix-radius,0);jx<min(ix+radius+1,width);jx++)
					out(ix,iy)+=in(jx,jy)/gauss(jx-ix,jy-iy);
}

void get_filter_gaussian
(int width,
int height,
MAT_RGB & in,
MAT_RGB & out,
int radius=1)
{
	////gaussian blurr
	int ix,iy;
	out.create(width,height);
	//build gaussian
	matrix_tag<unsigned long long> gauss(-radius,radius,-radius,radius);
	#pragma omp parallel for private(ix,iy)
	for(iy=-radius;iy<1+radius;iy++)
		for(ix=-radius;ix<1+radius;ix++)
			gauss(ix,iy)=1/gaussian(ix,iy,radius);
	//apply it
	int jx,jy;
	out.zeroMem();
	#pragma omp parallel for private(ix,iy,jx,jy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			for(jy=max(iy-radius,0);jy<min(iy+radius+1,height);jy++)
				for(jx=max(ix-radius,0);jx<min(ix+radius+1,width);jx++)
				{
					out(ix,iy)[0]+=in(jx,jy)[0]/gauss(jx-ix,jy-iy);
					out(ix,iy)[1]+=in(jx,jy)[1]/gauss(jx-ix,jy-iy);
					out(ix,iy)[2]+=in(jx,jy)[2]/gauss(jx-ix,jy-iy);
				}
}

void get_grayscale_rgb
(int width,
int height,
MAT_GRAYSCALE & in,
MAT_RGB & out)
{
	//show grayscale
	int ix,iy;
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			out(ix,iy)[2]=out(ix,iy)[1]=out(ix,iy)[0]=in(ix,iy);
}

void get_grayscale_overlay_rgb
(int width,
	int height,
	MAT_GRAYSCALE & in,
	MAT_RGB & out)
{
	//show grayscale
	int ix, iy;
#pragma omp parallel for private(ix,iy)
	for (iy = 0;iy < height;iy++)
		for (ix = 0;ix < width;ix++)
		{
			out(ix, iy)[2] =(out(ix, iy)[2] + in(ix, iy))/2;
			out(ix, iy)[1] =(out(ix, iy)[1] + in(ix, iy))/2;
			out(ix, iy)[0] =(out(ix, iy)[0] + in(ix, iy))/2;
		}
}

void draw_avg_centroid_bw
(int width,
int height,
MAT_GRAYSCALE & in,
MAT_RGB * out=0,
int * x_out=0,
int * y_out=0,
bool b_set_cursor_pos=false)
{
	int ix,iy;
	__int64 x=0,y=0;
	int n=0;
	#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height;iy++)
		for(ix=0;ix<width;ix++)
			if(in(ix,iy))
				{n++;x+=ix;y+=iy;}
	if(n!=0)
	{
		static int cx=0,cy=0;
		x=x/n;y=y/n;
		cx+=(x-cx)*0.1;
		cy+=(y-cy)*0.1;
		if(out)
		{
			Pset(*out,x,y,5,0,255,0);
			Pset(*out,cx,cy,5,255,255,0);
		}
		if(b_set_cursor_pos)SetCursorPos(1900-cx*1900/width,cy*1200/height);
		if(x_out)*x_out=x;
		if(y_out)*y_out=y;
	}
}

struct pointxy_tag{int ix,iy;pointxy_tag(int _ix,int _iy):ix(_ix),iy(_iy){}};


void seek_corners_gradient_matrix
(int width,
int height,
MAT_GRAYSCALE & xdiff,
MAT_GRAYSCALE & ydiff,
std::list<pointxy_tag> & corner_list)
{
	//seek for corners: gradient matrix
	int ix,iy;

	int corner_radius = 3; //7
	double corner_threshold = 100;   //200 old->//200*200;
	#pragma omp parallel for private(ix,iy)
	for(iy=1;iy<height-1;iy++)
		for(ix=1;ix<width-1;ix++)
		{
			__int64 sx=0,sy=0,sxy=0;
			int jx,jy;
			for(jy=max(iy-corner_radius,0);jy<min(iy+corner_radius+1,height);jy++)
				for(jx=max(ix-corner_radius,0);jx<min(ix+corner_radius+1,width);jx++)
				{
					sx+=pow2((int)xdiff(jx,jy));
					sy+=pow2((int)ydiff(jx,jy));
					sxy+=xdiff(jx,jy)*(int)ydiff(jx,jy);
				}
			double ssx = sx/double(corner_radius*corner_radius);
			double ssy = sy/double(corner_radius*corner_radius);
			double ssxy = sxy/double(corner_radius*corner_radius);
			double lambda[2];
			eigenvalues(lambda,ssx,ssxy,ssxy,ssy);
			if((lambda[0]>corner_threshold)&&(lambda[1]>corner_threshold))
			{
				corner_list.push_back(pointxy_tag(ix,iy));
			}
		}
}

void seek_corners_fast
(int width,
int height,
MAT_GRAYSCALE & in,
std::list<pointxy_tag> & corner_list,
MAT_RGB * out=0)
{
	//seek for corners: FAST
	int ix,iy;
	const int N_rel_c = 7;
	const int max_coord = 4;
	const int rel_c[N_rel_c]={0,1,2,3,3,4,max_coord};
	#pragma omp parallel for private(ix,iy)
	for(iy=max_coord;iy<height-max_coord-1;iy++)
		for(ix=max_coord;ix<width-max_coord-1;ix++)
		{
			int i;
			unsigned char center = in(ix,iy), circle[4*(N_rel_c-1)];
			for (i=0;i<N_rel_c-1;i++)
			{
				for (int sx=1;sx>=-1;sx-=2)
					for (int sy=sx;sx*sy>=-1;sy-=2*sx)
				{
					int ni=N_rel_c-i-1;
					bool bi=sx==sy;
					circle[i+(N_rel_c-1)*((3-sx*sy)/2-sx)]=in(ix+rel_c[bi?i:ni]*sx,iy+rel_c[bi?ni:i]*sy);
				}
			}
			int cont_count=-999999,maxcount=0;
			int cont_ncount=-999999,maxncount=0;
			for (int ii=0;ii<8*(N_rel_c-1);ii++)
			{
				i=ii%(4*(N_rel_c-1));
				if(circle[i]!=center)
				{
					cont_count++;
					maxncount=max(maxncount,cont_ncount);
					cont_ncount=0;
				}
				else
				{
					cont_ncount++;
					maxcount=max(maxcount,cont_count);
					cont_count=0;
				}
			}
			maxcount=max(maxcount,cont_count);
			maxncount=max(maxncount,cont_ncount);
			if((maxcount>2*(N_rel_c-1)+2) && (maxncount>N_rel_c/3)) // && (maxcount+maxncount==4*(N_rel_c-1)))
			{
				corner_list.push_back(pointxy_tag(ix,iy));
				if(out)Cset(*out,ix,iy,N_rel_c,rel_c,center,circle);
			}
		}
}


//EXAMPLE CONFIGURATION
//PARAMS FOR seek for corners: FAST3 -2gap

//const int N_rel_c = 5;
//const int threshold=40;
//const int max_coord = 3*2;
//const int rel_c[N_rel_c]={0*2,1*2,2*2,3*2,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)-1;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 5;
//const int threshold=40;
//const int max_coord = 6;
//const int rel_c[N_rel_c]={0,2,4,6,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+1;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 5;
//const int threshold=40;
//const int max_coord = 12;
//const int rel_c[N_rel_c]={0,4,8,12,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+1;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 7;
//const int threshold=50;
//const int max_coord = 4;
//const int rel_c[N_rel_c]={0,1,2,3,3,4,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+1;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 8;
//const int threshold=50;
//const int max_coord = 5;
//const int rel_c[N_rel_c]={0,1,2,3,4,5,5,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+1;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 9;
//const int threshold=40;
//const int max_coord = 6;
//const int rel_c[N_rel_c]={0,1,2,3,4,5,6,6,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+2;
//const int criteria_maxncount = N_rel_c/3-1;

//const int N_rel_c = 6;
//const int threshold=40;
//const int max_coord = 7;
//const int rel_c[N_rel_c]={0,2,4,6,7,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+2;
//const int criteria_maxncount = N_rel_c/3;

//const int N_rel_c = 6;
//const int threshold=30;
//const int max_coord = 14;
//const int rel_c[N_rel_c]={0,4,8,12,14,max_coord};
//const int criteria_maxcount = 2*(N_rel_c-1)+2;
//const int criteria_maxncount = N_rel_c/3;

template<int N_rel_c = 5> struct corner_tag{int ix;int iy; int circle[4*(N_rel_c-1)];typedef std::list<corner_tag<N_rel_c>> list_tag;};

inline int internal_get_fast3_difference(unsigned char center[3] , unsigned char p[3])
{
	int difp = 0;
	for (int k = 0;k < 3;k++)
		difp = max(difp, +abs((int)(p[k] - center[k]))); //difp+=abs(((int)p[k]-center[k]));
	return difp;
}

inline int internal_get_fast3_difference(unsigned char center, unsigned char p)
{
	return abs((int)p - center);
}

template<
	int N_rel_c, //5
	int max_coord, //3*2
	typename T> // pixel type (either unsigned char or unsigned char [3])
void seek_corners_fast3
(
	int width,
	int height,
	matrix_tag<T> & buf,
	typename corner_tag<N_rel_c>::list_tag & corner_list,
	const int rel_c[N_rel_c],  //{0*2,1*2,2*2,3*2,max_coord},
	int threshold=40,
	int criteria_maxcount = 2*(N_rel_c-1)-1,
	int criteria_maxncount = N_rel_c/3
)
{
	//seek for corners: FAST3 -2gap
	int ix,iy;

	#pragma omp parallel for private(ix,iy)
	for(iy=max_coord;iy<height-max_coord-1;iy+=2)
		for(ix=max_coord;ix<width-max_coord-1;ix+=2)
		{
			int i;
			T & center = buf(ix, iy);
			int circle[4*(N_rel_c-1)];
			for (i=0;i<N_rel_c-1;i++)
			{
				for (int sx=1;sx>=-1;sx-=2)
					for (int sy=sx;sx*sy>=-1;sy-=2*sx)
				{
					int ni=N_rel_c-i-1;
					bool bi=sx==sy;
					T & p= buf(ix+rel_c[bi?i:ni]*sx,iy+rel_c[bi?ni:i]*sy);
					circle[i+(N_rel_c-1)*((3-sx*sy)/2-sx)]= internal_get_fast3_difference(center, p);
				}
			}
			int cont_count=-999999,maxcount=0;
			int cont_ncount=-999999,maxncount=0;
			for (int ii=0;ii<8*(N_rel_c-1);ii++)
			{
				i=ii%(4*(N_rel_c-1));
				if(circle[i]>(threshold))
				{
					cont_count++;
					maxncount=max(maxncount,cont_ncount);
					cont_ncount=0;
				}
				else
				{
					cont_ncount++;
					maxcount=max(maxcount,cont_count);
					cont_count=0;
				}
			}
			maxcount=max(maxcount,cont_count);
			maxncount=max(maxncount,cont_ncount);
			if((maxcount>criteria_maxcount) && (maxncount>criteria_maxncount)) // && (maxcount+maxncount==4*(N_rel_c-1)))
			{
				corner_tag<N_rel_c> c;
				c.ix=ix;
				c.iy=iy;
				memcpy(c.circle,circle,sizeof(circle));
				#pragma omp critical
				{
					corner_list.push_back(c);        
				}
			}
		}
}


template<
	int N_rel_c> //5
void draw_circle_markers_rgb
(
	int width,
	int height,
	MAT_RGB & out,
	typename corner_tag<N_rel_c>::list_tag & corner_list,
	const int rel_c[N_rel_c], //{0*2,1*2,2*2,3*2,max_coord}
	int threshold=40
)
{
	//Draw markers for features found
	for(std::list<corner_tag<N_rel_c>>::iterator it=corner_list.begin();it!=corner_list.end();it++)
		//Pset(buf,it->ix,it->iy,0); 
		Cset2(out,it->ix,it->iy,N_rel_c,rel_c,threshold,it->circle);
}

template<
	int N_rel_c> //5
void draw_point_corner_markers_rgb
(
	int width,
	int height,
	MAT_RGB & out,
	typename corner_tag<N_rel_c>::list_tag & corner_list
)
{
	//Draw markers for features found
	for (std::list<corner_tag<N_rel_c>>::iterator it = corner_list.begin();it != corner_list.end();it++)
		Pset(out,it->ix,it->iy,1);
}


void draw_line_rgb
(
	int width,	int height,
	MAT_RGB & out,
	float m, int b // y=mx+b
)
{
	//Draw markers for features found
	for (int i = 0;i < width;i++)
		Pset(out, i, m*i+b, 1, 0, 255, 0);
	for (int i = 0;i < height;i++)
		Pset(out, (i-b)/m, i, 1,0,255,0);
}

void get_root_square_differences_rgb
(int width,
int height,
MAT_RGB & buf,
int x,
int y)
{
	//draw root sqare differences
	const int threshold=3*10*30;
	int ix,iy;
	unsigned char center[3];
	memcpy(center, buf(x,y),3);
	//#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height-1;iy++)
		for(ix=0;ix<width-1;ix++)
		{
			unsigned char * p=buf(ix,iy);
			int difp=0;
			for (int k=0;k<3;k++)
				difp=max(difp,+abs((int)(p[k]-center[k])));
			color_tag c = BGRScale(difp,0,3*255) ;
			Pset(buf,ix,iy,0,c.r,c.g,c.b);
		}
	Pset(buf,x,y,1,255,0,255);
}

void get_fn_bw
(int width,
int height,
MAT_RGB & in,
MAT_GRAYSCALE & out,
unsigned char (*fn)(unsigned char *))
{
	//apply function to all pixels
	int ix,iy;
	out.create(width,height);
	//#pragma omp parallel for private(ix,iy)
	for(iy=0;iy<height-1;iy++)
		for(ix=0;ix<width-1;ix++)
			out(ix,iy)=fn(in(ix,iy));
}

template<int n>
inline unsigned char fn_get_component(unsigned char *c)
{ return c[n];}

template<unsigned char(*fn1)(unsigned char *), unsigned char(*fn2)(unsigned char *)>
inline unsigned char fn_and(unsigned char *c)
{
	return (fn1(c) && fn2(c)) ? 255 : 0;
}

template<int component, unsigned char min, unsigned char max>
inline unsigned char fn_component_between(unsigned char *c)
{
	if (min <= max)
		return ((c[component] <= max) && (c[component] >= min)) ? 255 : 0;
	else
		return ((c[component] <= max) || (c[component] >= min)) ? 255 : 0;
}

template<unsigned char min1, unsigned char max1,unsigned char min2, unsigned char max2,unsigned char min3, unsigned char max3>
inline unsigned char fn_component_between3(unsigned char *c)
{return fn_and<fn_and<fn_component_between<0,min1,max1>, fn_component_between<1, min2,max2>>, fn_component_between<2, min3,max3>>(c);}

////////////////////////////////////////
// OLD CODE
////////////////////////////////////////



			//if(0)
			//{
			//	//seek for corners: FAST3
			//	
			//	const int N_rel_c = 5;
			//	const int threshold=40;
			//	const int max_coord = 3;
			//	const int rel_c[N_rel_c]={0,1,2,3,max_coord};
			//	const int criteria_maxcount = 2*(N_rel_c-1)-1;
			//	const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 5;
			//	//const int threshold=40;
			//	//const int max_coord = 6;
			//	//const int rel_c[N_rel_c]={0,2,4,6,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+1;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 5;
			//	//const int threshold=40;
			//	//const int max_coord = 12;
			//	//const int rel_c[N_rel_c]={0,4,8,12,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+1;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 7;
			//	//const int threshold=50;
			//	//const int max_coord = 4;
			//	//const int rel_c[N_rel_c]={0,1,2,3,3,4,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+1;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 8;
			//	//const int threshold=50;
			//	//const int max_coord = 5;
			//	//const int rel_c[N_rel_c]={0,1,2,3,4,5,5,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+1;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 9;
			//	//const int threshold=40;
			//	//const int max_coord = 6;
			//	//const int rel_c[N_rel_c]={0,1,2,3,4,5,6,6,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+2;
			//	//const int criteria_maxncount = N_rel_c/3-1;

			//	//const int N_rel_c = 6;
			//	//const int threshold=40;
			//	//const int max_coord = 7;
			//	//const int rel_c[N_rel_c]={0,2,4,6,7,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+2;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	//const int N_rel_c = 6;
			//	//const int threshold=30;
			//	//const int max_coord = 14;
			//	//const int rel_c[N_rel_c]={0,4,8,12,14,max_coord};
			//	//const int criteria_maxcount = 2*(N_rel_c-1)+2;
			//	//const int criteria_maxncount = N_rel_c/3;

			//	struct corner_tag{int ix;int iy; int circle[4*(N_rel_c-1)];};
			//	std::list<corner_tag> corner_list;
			//	#pragma omp parallel for private(ix,iy)
			//	for(iy=max_coord;iy<height-max_coord-1;iy++)
			//		for(ix=max_coord;ix<width-max_coord-1;ix++)
			//		{
			//			int i;
			//			unsigned char *center = buf(ix,iy);
			//			int circle[4*(N_rel_c-1)];
			//			for (i=0;i<N_rel_c-1;i++)
			//			{
			//				for (int sx=1;sx>=-1;sx-=2)
			//					for (int sy=sx;sx*sy>=-1;sy-=2*sx)
			//				{
			//					int ni=N_rel_c-i-1;
			//					bool bi=sx==sy;
			//					unsigned char * p=buf(ix+rel_c[bi?i:ni]*sx,iy+rel_c[bi?ni:i]*sy);
			//					int difp=0;
			//					for (int k=0;k<3;k++)
			//						difp=max(difp,+abs((int)(p[k]-center[k])));//difp+=abs(((int)p[k]-center[k]));
			//					circle[i+(N_rel_c-1)*((3-sx*sy)/2-sx)]=difp;
			//				}
			//			}
			//			int cont_count=-999999,maxcount=0;
			//			int cont_ncount=-999999,maxncount=0;
			//			for (int ii=0;ii<8*(N_rel_c-1);ii++)
			//			{
			//				i=ii%(4*(N_rel_c-1));
			//				if(circle[i]>(threshold))
			//				{
			//					cont_count++;
			//					maxncount=max(maxncount,cont_ncount);
			//					cont_ncount=0;
			//				}
			//				else
			//				{
			//					cont_ncount++;
			//					maxcount=max(maxcount,cont_count);
			//					cont_count=0;
			//				}
			//			}
			//			maxcount=max(maxcount,cont_count);
			//			maxncount=max(maxncount,cont_ncount);
			//			if((maxcount>criteria_maxcount) && (maxncount>criteria_maxncount)) // && (maxcount+maxncount==4*(N_rel_c-1)))
			//			{
			//				corner_tag c;
			//				c.ix=ix;
			//				c.iy=iy;
			//				memcpy(c.circle,circle,sizeof(circle));
			//				#pragma omp critical
			//				{
			//					corner_list.push_back(c);        
			//				}
			//			}
			//		}
			//	for(std::list<corner_tag>::iterator it=corner_list.begin();it!=corner_list.end();it++)
			//		//Pset(buf,it->ix,it->iy,0); 
			//		Cset2(buf,it->ix,it->iy,N_rel_c,rel_c,threshold,it->circle);
			//	sprintf(output_text,"%d corners",corner_list.size());
			//}


			//seek_corners_fast3<
			//	5,  //int N_rel_c 
			//	3*2 //int max_coord
			//>(
			//	width,
			//	height,
			//	buf,
			//	corner_list,
			//	rel_c, //{0*2,1*2,2*2,3*2,max_coord}
			//	threshold,
			//	criteria_maxcount,
			//	criteria_maxncount
			//);

			//POINT pp;
			//if (GetCursorPos(&pp))
			//if (ScreenToClient(hWnd, &pp))
			//if (pp.x<width && pp.x>=0 && pp.y<height && pp.y>=0)
			//{
			//	get_root_square_differences_rgb
			//	(width,
			//	height,
			//	buf,
			//	pp.x,
			//	pp.y);
			//}