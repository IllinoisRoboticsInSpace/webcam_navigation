// WebCamToWindow.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "WebCamToWindow.h"
#include "video_geometry.h"
#include "windows_specific.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

videoInput VI;
int device1= 1;
bool on_init=1;
int numDevices;
extern void printf(char*,...);

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
#ifndef Release_NONMP
	omp_set_num_threads(omp_get_num_procs()); 
#endif

	printf("");

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WEBCAMTOWINDOW, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WEBCAMTOWINDOW));
	int numDevices = VI.listDevices();

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    VI.stopDevice(device1);
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBCAMTOWINDOW));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WEBCAMTOWINDOW);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   global_display_hWnd = hWnd;

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	char output_text[1000];
	output_text[0]=0;

	switch (message)
	{
	case WM_CREATE:
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if(on_init)
		{
			char c[1000];
			if(numDevices)
				sprintf(c,"Se encontraron %d dispositivos, elija usando las teclas de número del teclado.",numDevices);
			else
				sprintf(c,"No se encontraron dispositivos, sin embargo puede probar el dispositivo 0 por si acaso.                 Elija usando las teclas de número del teclado.");
			TextOut(hdc,10,10,c,strlen(c));
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if(on_init)
		{
			on_init=0;
			device1=wParam-0x30;
			char c[1000];
			sprintf(c,"WebCamAnalysis ARR - device %d",device1);
			SetWindowText(hWnd,c);
			VI.setupDevice(device1);//,640,230);
			//VI.showSettingsWindow(device1);
			SetTimer(hWnd,10,100,0);
		}
		break;
	case WM_TIMER:
		{
			static MAT_GRAYSCALE bw,diff,xdiff,ydiff,bld;

			if(1)
			{
				static int millis=0;
				int millis2;
				static int lost_count=0;
				do{
					SYSTEMTIME systime;
					GetSystemTime(&systime);
					millis2=systime.wMilliseconds +1000*(int)systime.wSecond;
					//if(abs(millis2-millis)>300)
						break;
					Sleep(5);
				}
				while(1);
				if(abs(millis2-millis)>310)
					lost_count++;
				static int count =0;
				static double fps =1;
				fps=fps*.9+1000./(abs(millis2-millis))*.1;
				count++;
				char c[1000];
				sprintf(c,"WebCamAnalysis ARR - device %d - count %d lost %d fps %f",device1,count,lost_count,fps);
				SetWindowText(hWnd,c);
				millis = millis2;
			}

			int width;
			int height;
			static MAT_RGB buf0;
			static MAT_RGB buf;

			get_webcam_pixels(width,height,buf,device1,VI);

			//get_filter_gaussian(width,height,buf0,buf,3);

			buf0 = buf;

			rgb2hsv(buf0, width, height);
			rgb2hsv(buf, width, height);

			get_fn_bw(width,height,buf0,bw, fn_component_between3<250,15, 96, 255, 64, 255>);

			hsv2huergb(buf, width, height,0,0,24);
			//hsv2rgb(buf, width, height);

			//get_fn_bw(width,height,buf,bw,fn_get_component<1>);

			//get_grayscale_overlay_rgb(width,height,bw,buf);

			static std::list<pointxy_tag> corners;

			corners.clear();

			//get_grayscale_rgb(width,height,bw,buf);

			//seek_corners_fast(width, height, bw, corners,&buf);
		
			const int N_rel_c = 5;
			const int threshold=40;
			const int max_coord = 3 ;
			const int rel_c[N_rel_c]={0,1,2,3,max_coord};
			const int criteria_maxcount = 7 * (N_rel_c-1) / 5; //2*(N_rel_c-1)+1;
			const int criteria_maxncount = 8 * (N_rel_c-1) / 5; //N_rel_c/3;

			//const int N_rel_c = 5;
			//const int threshold=40;
			//const int max_coord = 3 * 2;
			//const int rel_c[N_rel_c]={0*2,1*2,2*2,3*2,max_coord};
			//const int criteria_maxcount = 7 * (N_rel_c-1) / 5; //2*(N_rel_c-1)+1;
			//const int criteria_maxncount = 8 * (N_rel_c-1) / 5; //N_rel_c/3;

			//const int N_rel_c = 9;
			//const int threshold=40;
			//const int max_coord = 12;
			//const int rel_c[N_rel_c]={0,2,4,6,8,10,12,12,max_coord};
			//const int criteria_maxcount = 8 * N_rel_c / 5; //2*(N_rel_c)+1;
			//const int criteria_maxncount = 8 * N_rel_c / 5; //N_rel_c/3;
	
			corner_tag<N_rel_c>::list_tag corner_list;

			seek_corners_fast3<
				N_rel_c,
				max_coord
			>(
				width,
				height,
				bw,
				corner_list,
				rel_c, //{0*2,1*2,2*2,3*2,max_coord}
				threshold,
				criteria_maxcount,
				criteria_maxncount
			);
			
			float m, b, r;
			geom_linear_least_squares(corner_list, &m, &b, &r);
			draw_line_rgb(width, height, buf, m, b);


			//draw_circle_markers_rgb<
			//	N_rel_c
			//>(
			//width,
			//height,
			//buf,
			//corner_list,
			//rel_c, //{0*2,1*2,2*2,3*2,max_coord}
			//threshold
			//);

			draw_point_corner_markers_rgb<
				N_rel_c
			>(
				width,
				height,
				buf,
				corner_list
			);

			//get_grayscale_rgb(width,height,bw,buf);
				
			sprintf(output_text,"%d corners",corner_list.size());


			draw_buffer(width,height,buf,output_text);


			//if(1)
			//{
			//	//paint buffer
			//	HDC hdc=GetDC(hWnd);
			//	BITMAPINFO bitmapInfo;
			//	bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
			//	bitmapInfo.bmiHeader.biWidth         = width; 
			//	bitmapInfo.bmiHeader.biHeight        = -(height);
			//	bitmapInfo.bmiHeader.biPlanes        = 1;
			//	bitmapInfo.bmiHeader.biBitCount      = 24;
			//	bitmapInfo.bmiHeader.biCompression   = BI_RGB;
			//	bitmapInfo.bmiHeader.biSizeImage     = VI.getSize(device1);
			//	bitmapInfo.bmiHeader.biXPelsPerMeter = 1;
			//	bitmapInfo.bmiHeader.biYPelsPerMeter = 1;
			//	bitmapInfo.bmiHeader.biClrUsed       = 0;
			//	bitmapInfo.bmiHeader.biClrImportant  = 0;
			//	int r = StretchDIBits(hdc,0,0,width*(width<100?5:1),height*(width<100?5:1),0,0,width,height,buf,&bitmapInfo,DIB_RGB_COLORS,SRCCOPY);
			//	
			//	if(0)
			//	{
			//		char c[1000];
			//		int px=10,py=0,dy=15;
			//		sprintf(c,"Camera settings of %s:",VI.getDeviceName(device1));
			//		TextOut2(hdc,px,py+=dy,c);
			//		for(int i =0;i<10;i++)
			//		{
			//			long imin,imax,idel,ival,ifla,idef;
			//			bool r=VI.getVideoSettingCamera(device1,i,imin,imax,idel,ival,ifla,idef);
			//			sprintf(c,"%d: min:%d max%d delta:%d value:%d flags:%d default:%d",r?i:-i,imin,imax,idel,ival,ifla,idef);
			//			TextOut2(hdc,px,py+=dy,c);
			//		}
			//		sprintf(c,"Filter settings of %s:",VI.getDeviceName(device1));
			//		TextOut2(hdc,px,py+=dy,c);
			//		for(int i =0;i<10;i++)
			//		{
			//			long imin,imax,idel,ival,ifla,idef;
			//			bool r=VI.getVideoSettingFilter(device1,i,imin,imax,idel,ival,ifla,idef);
			//			sprintf(c,"%d: min:%d max%d delta:%d value:%d flags:%d default:%d",r?i:-i,imin,imax,idel,ival,ifla,idef);
			//			TextOut2(hdc,px,py+=dy,c);
			//		}
			//	}
			//	TextOut2(hdc,1210,10,output_text);
			//	ReleaseDC(hWnd,hdc);
			//}
			SetTimer(hWnd,10,20,0);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
