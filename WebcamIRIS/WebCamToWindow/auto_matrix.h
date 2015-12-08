#pragma once
#include <memory.h>

#if _DEBUG
	#define ASSERT(x) if(!(x)) DebugBreak()
#else
	#define ASSERT(x) 
#endif

template<typename T> struct matrix_tag
{
public:
	matrix_tag(matrix_tag & m)
	{
		d=0;
		nx=0;ny=0;
		*this=m;
	}
	matrix_tag()
	{
		d=0;
		nx=0;ny=0;
	}
	int dx,dy;
	matrix_tag(int width, int height)
	{
		d=0;
		nx=0;ny=0;
		create(width,height);
	}
private:
	int nx,ny;
	T* d;
	T* g;
public:
	void create(int ax,int bx,int ay,int by)
	{
		int oldsize=nx*ny;
		dx=-ax;dy=-ay;
		nx=bx-ax+1;ny=by-ay+1;
		ASSERT(nx>0);
		ASSERT(ny>0);
		int newsize=nx*ny;
		if(d && oldsize!=newsize)
		{
			delete[] d;
			d=0;
		}
		if(!d)
			d=new T [newsize];
		g=d+dx+nx*dy;
	}
	void create(int width, int height)
	{
		create(0,width-1,0,height-1);
	}
	matrix_tag(int ax,int bx,int ay,int by)
	{
		d=0;
		nx=0;ny=0;
		create(ax,bx,ay,by);
	}
#if _DEBUG
	T & operator() (int ax,int ay)
	{
		int kx=ax+dx;int ky=ay+dy;
		ASSERT(kx<nx);
		ASSERT(ky<ny);
		ASSERT(kx>=0);
		ASSERT(ky>=0);
		return d[kx+nx*ky];
	}
#else
	T & operator() (int ax,int ay)
	{
		return g[ax+nx*ay];
	}
#endif
	matrix_tag & operator= (const matrix_tag & m)
	{
		if(d)
			delete[] d;
		memcpy(this,&m,sizeof(matrix_tag));
		d=new T [nx * ny];
		g=d+dx+nx*dy;
		memcpy(d,m.d,nx * ny * sizeof(T));
		return *this;
	}
	int xSize()
	{
		return nx;
	}
	int ySize()
	{
		return ny;
	}
	operator void*()
	{
		return (void*)d;
	}
	T* data()
	{
		return d;
	}
	void zeroMem()
	{
		memset(d,0,nx * ny * sizeof(T));
	}
	~matrix_tag()
	{
		if(d)
			delete[] d;
		d=0;
	}
} ;

typedef matrix_tag<double> MATRIX;
typedef matrix_tag<unsigned char> MAT_GRAYSCALE;
typedef matrix_tag<unsigned char[3]> MAT_RGB;

//**************************************************************
/*
   Return a RGB colour value given a scalar v in the range [vmin,vmax]
   In this case each colour component ranges from 0 (no contribution) to
   1 (fully saturated), modifications for other ranges is trivial.
   The colour is clipped at the end of the scales if v is outside
   the range [vmin,vmax]
*/

struct color_tag{
	union{
		struct{
			unsigned char r,g,b;
		};
		struct{
			unsigned char h,s,v;
		};
	};


};

template<typename T>color_tag BGRScale(T v,T vmin,T vmax) 
{
   color_tag c = {255,255,255}; // white
   double dv;

   if (v < vmin)
      v = vmin;
   if (v > vmax)
      v = vmax;
   dv = vmax - vmin;

   if (v < (vmin + dv/4)) {
      c.r = 0;
      c.g = 255 * 4 * (v - vmin) / dv;
   } else if (v < (vmin + dv/2)) {
      c.r = 0;
      c.b = 255 + 255 *(4 * (vmin  - v)+ dv) / dv;
   } else if (v < (vmin + 3 * dv/4)) {
      c.r = 255 * 4 * (v - vmin - dv/2) / dv;
      c.b = 0;
   } else {
      c.g =  255 + 255 *(4 * (vmin- v) + 3 * dv ) / dv;
      c.b = 0;
   }

   return(c);
}


/*
   Calculate RGB from HSV, reverse of RGB2HSV()
   Hue is in degrees
   Lightness is between 0 and 1
   Saturation is between 0 and 1
*/

color_tag HSV2RGB(color_tag c1)
{
   color_tag c2,sat;

   while (c1.h < 0)
      c1.h += 360;
   while (c1.h > 360)
      c1.h -= 360;

   if (c1.h < 120) {
      sat.r = (120 - c1.h) / 60.0;
      sat.g = c1.h / 60.0;
      sat.b = 0;
   } else if (c1.h < 240) {
      sat.r = 0;
      sat.g = (240 - c1.h) / 60.0;
      sat.b = (c1.h - 120) / 60.0;
   } else {
      sat.r = (c1.h - 240) / 60.0;
      sat.g = 0;
      sat.b = (360 - c1.h) / 60.0;
   }
   sat.r = min(sat.r,1);
   sat.g = min(sat.g,1);
   sat.b = min(sat.b,1);

   c2.r = (1 - c1.s + c1.s * sat.r) * c1.v;
   c2.g = (1 - c1.s + c1.s * sat.g) * c1.v;
   c2.b = (1 - c1.s + c1.s * sat.b) * c1.v;

   return(c2);
}

/*
   Calculate HSV from RGB
   Hue is in degrees
   Lightness is betweeen 0 and 1
   Saturation is between 0 and 1
*/

color_tag RGB2HSV(color_tag c1)
{
   double themin,themax,delta;
   color_tag c2;

   themin = min(c1.r,min(c1.g,c1.b));
   themax = max(c1.r,max(c1.g,c1.b));
   delta = themax - themin;
   c2.v = themax;
   c2.s = 0;
   if (themax > 0)
      c2.s = delta / themax;
   c2.h = 0;
   if (delta > 0) {
      if (themax == c1.r && themax != c1.g)
         c2.h += (c1.g - c1.b) / delta;
      if (themax == c1.g && themax != c1.b)
         c2.h += (2 + (c1.b - c1.r) / delta);
      if (themax == c1.b && themax != c1.r)
         c2.h += (4 + (c1.r - c1.g) / delta);
      c2.h *= 60;
   }
   return(c2);
}