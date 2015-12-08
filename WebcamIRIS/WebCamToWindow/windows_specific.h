#pragma once

#include "auto_matrix.h"
#include <windows.h>

HWND global_display_hWnd = 0;

BOOL __forceinline TextOut2(HDC dc, int X, int Y, char*c)
{
	return TextOut(dc, X, Y, c, strlen(c));
}

void draw_buffer(int width,
	int height,
	MAT_RGB & buf,
	char* output_text="")
{
	//paint buffer
	HDC hdc = GetDC(global_display_hWnd);
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -(height);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 24;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = width*height*3;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 1;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 1;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	int r = StretchDIBits(hdc, 0, 0, width*(width<100 ? 5 : 1), height*(width<100 ? 5 : 1), 0, 0, width, height, buf, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	TextOut2(hdc, 1210, 10, output_text);
	ReleaseDC(global_display_hWnd, hdc);
}