//////////////////////////////////////
// COLOUR
//////////////////////////////////////

//RGB is 0-b 1-g 2-r
//HSV is 0-h 1-s 2-v

void rgb2hsv1(unsigned char* v)
{
	unsigned char min, max, delta;

	min = v[2] < v[1] ? v[2] : v[1];
	min = min  < v[0] ? min : v[0];

	max = v[2] > v[1] ? v[2] : v[1];
	max = max  > v[0] ? max : v[0];

	delta = max - min;
	if (delta == 0)
	{
		v[1] = 0;
		v[0] = 0; // undefined
	}
	else
	{
		if (v[2] == max)
			v[0] = (((int)v[1] - v[0]) * 43 / delta) % 256;        // h between yellow & magenta
		else
			if (v[1] == max)
				v[0] = (85 + ((int)v[0] - v[2]) * 43 / delta) % 256;  // h between cyan & yellow
			else
				v[0] = (171 + ((int)v[2] - v[1]) * 43 / delta) % 256;  // h between magenta & cyan

		v[1] = ((int)delta*255 / max);                  // s
	}
	v[2] = max;                                // v
}


void hsv2rgb1(unsigned char* v)
{
	unsigned char        p, q, ff;
	unsigned char        i;

	if (v[1] == 0) {
		v[1] = v[2];
		v[0] = v[2];
		v[2] = v[2];
		return;
	}
	i = v[0] / 43;
	ff = v[0] % 43;
	p = (v[2] * ((int)255 - v[1])) / 255;
	q = (v[2] * ((int)10965 - (v[1] * (i % 2 ? ff : (43 - ff))))) / 10965;

	switch (i) {
	case 0:
		v[1] = q;
		v[0] = p;
		//v[2] = v[2];
		break;
	case 1:
		v[1] = v[2];
		v[0] = p;
		v[2] = q;
		break;
	case 2:
		v[1] = v[2];
		v[0] = q;
		v[2] = p;
		break;

	case 3:
		v[1] = q;
		v[0] = v[2];
		v[2] = p;
		break;
	case 4:
		v[1] = p;
		v[0] = v[2];
		v[2] = q;
		break;
	case 5:
	default:
		v[1] = p;
		v[0] = q;
		//v[2] = v[2];
		break;
	}
}

void rgb2hsv(MAT_RGB & buf, int width, int height)
{
	int ix, iy;
	#pragma omp parallel for private(ix,iy)
	for (iy = 0;iy < height;iy++)
		for (ix = 0;ix < width;ix++)
			rgb2hsv1(buf(ix, iy));
}
void hsv2rgb(MAT_RGB & buf, int width, int height)
{
	int ix, iy;
#pragma omp parallel for private(ix,iy)
	for (iy = 0;iy < height;iy++)
		for (ix = 0;ix < width;ix++)
			hsv2rgb1(buf(ix, iy));
}
void hsv2huergb(MAT_RGB & buf, int width, int height,unsigned char black=32,unsigned char white=96,unsigned char steps=12)
{
	int ix, iy;
	int slice=256/steps;
#pragma omp parallel for private(ix,iy)
	for (iy = 0;iy < height;iy++)
		for (ix = 0;ix < width;ix++)
		{
			buf(ix,iy)[0]=((((int)buf(ix,iy)[0]+slice/2)/slice)%steps)*slice;
			buf(ix,iy)[1]=buf(ix,iy)[1]<white?0:255;
			buf(ix,iy)[2]=buf(ix,iy)[2]<black?0:255;
			hsv2rgb1(buf(ix, iy));
		}
}