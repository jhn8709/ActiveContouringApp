
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPTSTR lpCmdLine, int nCmdShow)

{
MSG			msg;
HWND		hWnd;
WNDCLASS	wc;

wc.style=CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc=(WNDPROC)WndProc;
wc.cbClsExtra=0;
wc.cbWndExtra=0;
wc.hInstance=hInstance;
wc.hIcon=LoadIcon(hInstance,"ID_PLUS_ICON");
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
wc.lpszMenuName="ID_MAIN_MENU";
wc.lpszClassName="PLUS";

if (!RegisterClass(&wc))
  return(FALSE);

hWnd=CreateWindow("PLUS","plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT,0,400,400,NULL,NULL,hInstance,NULL);
if (!hWnd)
  return(FALSE);

ShowScrollBar(hWnd,SB_BOTH,FALSE);
ShowWindow(hWnd,nCmdShow);
UpdateWindow(hWnd);
MainWnd=hWnd;

ShowPixelCoords=0;

strcpy(filename,"");
OriginalImage=NULL;
ROWS=COLS=0;

InvalidateRect(hWnd,NULL,TRUE);
UpdateWindow(hWnd);

while (GetMessage(&msg,NULL,0,0))
  {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  }
return(msg.wParam);
}

LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char		t[15], rad[15];
//	int					TotalRegions, r, c, i, RegionSize, r2, c2;
//	double				avg, var;

	switch (Message)
	{
	case WM_INITDIALOG:

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
//			GetDlgItemText(hwnd, IDC_EDIT1, t, 256);
//			GetDlgItemText(hwnd, IDC_EDIT2, rad, 256);


			

			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}



LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam)

{
HMENU				hMenu;
OPENFILENAME		ofn;
FILE				*fpt;
HDC					hDC;
char				header[320],text[320];
int					BYTES,xPos,yPos,x,y,count = 0,i,j, newRows, newCols;
double				dist, local_min = 10000, local_max = -10000, avg;
unsigned char       *resized_image;




switch (uMsg)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
      {
	  case ID_SHOWPIXELCOORDS:
		  ShowPixelCoords=(ShowPixelCoords+1)%2;
		  PaintImage();
		  break;
	  case ID_DISPLAY_BIGDOTS:
		  bigDots = (bigDots + 1) % 2;
		  PaintImage();
		  break;
	  case ID_ACTIVECONTOUR_DRAW:
		  ActiveContourDraw = (ActiveContourDraw + 1) % 2;
		  SharpenImage();
		  BlurImage();
		  total_points = 0;
		  //PaintImage();
		  break;
	  case ID_ACTIVECONTOUR_EXPANSION:
		  PaintImage();
		  ActiveContourPoint = (ActiveContourPoint + 1) % 2;
		  total_points_exp = 0;
		  BlurImage();
	
		  break;
	  case ID_ACTIVECONTOUR_STARTCONTEXP:
		  _beginthread(BalloonContour, 0, MainWnd);
		  break;
	  case ID_ACTIVECONTOUR_STARTCONTOUR:
		  _beginthread(ActiveContour, 0, MainWnd);
		  break;
	  case ID_ACTIVECONTOUR_SELECTPOINT:
		  point_select = (point_select + 1) % 2;
		  break;
	  case ID_FILE_LOAD:
		if (OriginalImage != NULL)
		  {
		  free(OriginalImage);
		  OriginalImage=NULL;
		  }
		memset(&(ofn),0,sizeof(ofn));
		ofn.lStructSize=sizeof(ofn);
		ofn.lpstrFile=filename;
		filename[0]=0;
		ofn.nMaxFile=MAX_FILENAME_CHARS;
		ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
		ofn.lpstrFilter = "PNM files\0*.pnm\0All files\0*.*\0\0";
		if (!( GetOpenFileName(&ofn))  ||  filename[0] == '\0')
		  break;		/* user cancelled load */
		if ((fpt=fopen(filename,"rb")) == NULL)
		 {
			MessageBox(NULL,"Unable to open file",filename,MB_OK | MB_APPLMODAL);
			break;
		 }
		fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
		if (strcmp(header,"P6") != 0  ||  BYTES != 255)
		 {
			MessageBox(NULL,"Not a PNM (P6 greyscale) image",filename,MB_OK | MB_APPLMODAL);
			fclose(fpt);
			break;
		 }
		RGB_image = (unsigned char*)calloc(ROWS * COLS * 3, sizeof(unsigned char));
		OriginalImage=(unsigned char *)calloc(ROWS*COLS,1);

		header[0]=fgetc(fpt);	/* whitespace character after header */
		fread(RGB_image,1,ROWS*COLS*3,fpt);
		fclose(fpt);
		
		// convert from rgb to greyscale
		for (i = 0; i < ROWS * COLS; i++)
		{
			avg = (double)(((int)RGB_image[i * 3 + 0] + (int)RGB_image[i * 3 + 1] + (int)RGB_image[i * 3 + 2]) / 3.0);
			OriginalImage[i] = (int)avg;
		}

		// resize image if too large
		if ((ROWS > 1200) || (COLS > 1200))
		{
			resized_image = (unsigned char*)calloc(ROWS * COLS/4, 1);
			newRows = (int)(ROWS / 2);
			newCols = (int)(COLS / 2);
			for (y = 0; y < newRows; y++)
			{
				for (x = 0; x < newCols; x++)
				{
					resized_image[y * (newCols)+(x)] = OriginalImage[(y * 2)*COLS+(x*2)];
				}
			}
			ROWS = newRows;
			COLS = newCols;
			OriginalImage = (unsigned char*)calloc(ROWS * COLS, 1);
			memcpy(OriginalImage, resized_image, ROWS * COLS * 1);
		}
		if ((fpt = fopen("gray.ppm", "wb")) == NULL)
		{
			printf("Unable to open file for writing\n");
			exit(0);
		}
		fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
		fwrite(OriginalImage, 1, ROWS * COLS, fpt);
		fclose(fpt);

		SetWindowPos(hWnd, HWND_TOP, 300, 150, COLS+15, ROWS+58,SWP_SHOWWINDOW);

		// generate sobel image from greyscale image
		normSobel = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
		GenerateSobel(OriginalImage,normSobel);
		fpt = fopen("sobel-filtered-VS.ppm", "wb");
		fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
		fwrite(normSobel, COLS * ROWS, 1, fpt);
		fclose(fpt);
		
		// allocate memory to blurred and sharp version of image
		SharpImage = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
		memcpy(SharpImage, OriginalImage, ROWS * COLS * 1);
		BlurredImage = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
		memcpy(BlurredImage, OriginalImage, ROWS * COLS * 1);
		
		SetWindowText(hWnd,filename);
		PaintImage();
		break;

      case ID_FILE_QUIT:
        DestroyWindow(hWnd);
        break;
      }
    break;
  case WM_SIZE:		  /* could be used to detect when window size changes */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_PAINT:
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_LBUTTONDOWN:

	  xmouse = LOWORD(lParam);
	  ymouse = HIWORD(lParam);
	  // initiate drawing 
	  if (ActiveContourDraw == 1)
	  {
		  click = 1;
	  }
	  // initiate point fixing
	  if (point_select == 1)
	  {
		  point_move = 1;
		  // code to estimate which point was selected
		  if (shrink_cont == 1)
		  {
			  for (i = 0; i < total_points; i++)
			  {
				  dist = sqrt((double)SQR(px[i] - xmouse)
					  + (double)SQR(py[i] - ymouse));
				  if (dist < local_min)
				  {
					  local_min = dist;
					  x_change = px[i];
					  y_change = py[i];
					  selected_point = i;
				  }

			  }
		  }
		  else if (exp_cont == 1)
		  {
			  for (i = 0; i < total_points_exp; i++)
			  {
				  dist = sqrt((double)SQR(px2[i] - xmouse)
					  + (double)SQR(py2[i] - ymouse));
				  if (dist < local_min)
				  {
					  local_min = dist;
					  x_change = px2[i];
					  y_change = py2[i];
					  selected_point = i;
				  }

			  }
		  }
		  // removing cross shape without getting rid of all changes to the image
		  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
		  {
			  hDC = GetDC(MainWnd);
			  SetPixel(hDC, x_change, y_change + j,
				  RGB((int)OriginalImage[(y_change+j) * COLS + x_change],
					  (int)OriginalImage[(y_change+j) * COLS + x_change],
					  (int)OriginalImage[(y_change+j) * COLS + x_change]));
			  SetPixel(hDC, x_change + j, y_change,
				  RGB((int)OriginalImage[y_change * COLS + (x_change+j)],
					  (int)OriginalImage[y_change * COLS + (x_change+j)],
					  (int)OriginalImage[y_change * COLS + (x_change+j)]));
			  ReleaseDC(MainWnd, hDC);
		  }
	  }
	  // select location where the fixed point is placed
	  if ((point_select == 0) && (point_move == 1))
	  {
		  if (shrink_cont == 1)
		  {
			  px[selected_point] = xmouse;
			  py[selected_point] = ymouse;
			  hDC = GetDC(MainWnd);
			  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
			  {
				  SetPixel(hDC, px[selected_point], py[selected_point] + j, RGB(0, 0, 0));
				  SetPixel(hDC, px[selected_point] + j, py[selected_point], RGB(0, 0, 0));

			  }
			  ReleaseDC(MainWnd, hDC);
		  }
		  else if (exp_cont == 1)
		  {
			  px2[selected_point] = xmouse;
			  py2[selected_point] = ymouse;
			  hDC = GetDC(MainWnd);
			  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
			  {
				  SetPixel(hDC, px2[selected_point], py2[selected_point] + j, RGB(0, 0, 0));
				  SetPixel(hDC, px2[selected_point] + j, py2[selected_point], RGB(0, 0, 0));

			  }
			  ReleaseDC(MainWnd, hDC);
		  }
		  point_move = 0;
		  // call thread to contour the points again
		  _beginthread(ManualContour, 0, MainWnd);
	  }
	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;

  case WM_LBUTTONUP:
	  
	  // stop drawing
	  if (ActiveContourDraw == 1)
	  {
		  PaintImage();
		  click = 0;
		  ActiveContourDraw = 0;
		  // takes every 15th point in the drawing
		  while (count < cont_points)
		  {
			  if ( ((count % 15) == 0) || (count == 1) )
			  {
				  px[total_points] = draw_x[count];
				  py[total_points] = draw_y[count];
				  total_points++;
			  }
			  count++;
		  }
		  cont_points = 0;
		  hDC = GetDC(MainWnd);
		  for (i = 0; i < total_points - 1; i++)
		  {
			  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
			  {
				  SetPixel(hDC, px[i] + j, py[i], RGB(255, 0, 0));
				  SetPixel(hDC, px[i], py[i] + j, RGB(255, 0, 0));
			  }
		  }
		  ReleaseDC(MainWnd, hDC);
	  }
	  if (point_select == 1)
	  {
		  point_select = 0;

	  }
	  
	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;
	
  case WM_RBUTTONDOWN:
	  xmouse = LOWORD(lParam);
	  ymouse = HIWORD(lParam);
	  // initiate balloon contour points
	  if (ActiveContourPoint == 1)
	  {
		  clickr = 1;
	  }

	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	break;
  case WM_RBUTTONUP:

	  // create balloon contour points
	  if (clickr == 1)
	  {
		  _beginthread(InitExpandContour, 0, MainWnd);
		  clickr = 0;
		  ActiveContourPoint = 0;
	  }

	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;

  case WM_MOUSEMOVE:
	  // gather points and paint the image while moving
	  if (click == 1)
	  {
			xPos=LOWORD(lParam);
			yPos=HIWORD(lParam);
			if (xPos >= 0  &&  xPos < COLS  &&  yPos >= 0  &&  yPos < ROWS)
			{
				//sprintf(text,"%d,%d=>%d     ",xPos,yPos,OriginalImage[yPos*COLS+xPos]);
				sprintf(text, "%d points recorded  ", cont_points + 1);
				hDC=GetDC(MainWnd);
				TextOut(hDC,0,0,text,strlen(text));		/* draw text on the window */
				draw_x[cont_points] = xPos;
				draw_y[cont_points] = yPos;

				for (x = -2; x <= 2; x++)
				{
					for (y = -2; y <= 2; y++)
					{
						SetPixel(hDC, xPos + x, yPos + y, RGB(255, 0, 0));
					}
				}
				cont_points++;
				ReleaseDC(MainWnd,hDC);
			}
	  }
	  // have the contour be visible while moving it
	  if (point_move == 1)
	  {
		  xPos = LOWORD(lParam);
		  yPos = HIWORD(lParam);
		  hDC = GetDC(MainWnd);
		  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
		  {
			  //hDC = GetDC(MainWnd);
			  SetPixel(hDC, x_change, y_change + j,
				  RGB((int)OriginalImage[(y_change+j) * COLS + x_change],
					  (int)OriginalImage[(y_change+j) * COLS + x_change],
					  (int)OriginalImage[(y_change+j) * COLS + x_change]));
			  SetPixel(hDC, x_change + j, y_change,
				  RGB((int)OriginalImage[y_change * COLS + (x_change+j)],
					  (int)OriginalImage[y_change * COLS + (x_change+j)],
					  (int)OriginalImage[y_change * COLS + (x_change+j)]));
			  //ReleaseDC(MainWnd, hDC);
		  }
		  if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
		  {
			  //hDC = GetDC(MainWnd);
			  for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
			  {
				  SetPixel(hDC, xPos, yPos + j, RGB(0, 0, 0));
				  SetPixel(hDC, xPos + j, yPos, RGB(0, 0, 0));	  
				  
			  }
			  //ReleaseDC(MainWnd, hDC);
		  }
		  ReleaseDC(MainWnd, hDC);
		  x_change = xPos;
		  y_change = yPos;

	  }
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_KEYDOWN:
	if (wParam == 's'  ||  wParam == 'S')
	  PostMessage(MainWnd,WM_COMMAND,ID_SHOWPIXELCOORDS,0);	  /* send message to self */
	if ((TCHAR)wParam == '1')
	  {
	  TimerRow=TimerCol=0;
	  SetTimer(MainWnd,TIMER_SECOND,10,NULL);	/* start up 10 ms timer */
	  }
	if ((TCHAR)wParam == '2')
	  {
	  KillTimer(MainWnd,TIMER_SECOND);			/* halt timer, stopping generation of WM_TIME events */
	  PaintImage();								/* redraw original image, erasing animation */
	  }
	if ((TCHAR)wParam == '3')
	  {
	  ThreadRunning=1;
	  _beginthread(AnimationThread,0,MainWnd);	/* start up a child thread to do other work while this thread continues GUI */
	  }
 	if ((TCHAR)wParam == '4')
	  {
	  ThreadRunning=0;							/* this is used to stop the child thread (see its code below) */
	  }
	return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
	hDC=GetDC(MainWnd);
	SetPixel(hDC,TimerCol,TimerRow,RGB(0,0,255));	/* color the animation pixel blue */
	ReleaseDC(MainWnd,hDC);
	TimerRow++;
	TimerCol+=2;
	break;
  case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
    break;
  }

hMenu=GetMenu(MainWnd);
if (ShowPixelCoords == 1)
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_UNCHECKED);
if (bigDots == 1)
CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
CheckMenuItem(hMenu, ID_DISPLAY_BIGDOTS, MF_UNCHECKED);
DrawMenuBar(hWnd);

DrawMenuBar(hWnd);

return(0L);
}




void PaintImage()

{
PAINTSTRUCT			Painter;
HDC					hDC;
BITMAPINFOHEADER	bm_info_header;
BITMAPINFO			*bm_info;
int					i,r,c,DISPLAY_ROWS,DISPLAY_COLS;
unsigned char		*DisplayImage;

if (OriginalImage == NULL)
  return;		/* no image to draw */

		/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
DISPLAY_ROWS=ROWS;
DISPLAY_COLS=COLS;
if (DISPLAY_ROWS % 4 != 0)
  DISPLAY_ROWS=(DISPLAY_ROWS/4+1)*4;
if (DISPLAY_COLS % 4 != 0)
  DISPLAY_COLS=(DISPLAY_COLS/4+1)*4;
DisplayImage=(unsigned char *)calloc(DISPLAY_ROWS*DISPLAY_COLS,1);
for (r=0; r<ROWS; r++)
  for (c=0; c<COLS; c++)
	DisplayImage[r*DISPLAY_COLS+c]=OriginalImage[r*COLS+c];

BeginPaint(MainWnd,&Painter);
hDC=GetDC(MainWnd);
bm_info_header.biSize=sizeof(BITMAPINFOHEADER); 
bm_info_header.biWidth=DISPLAY_COLS;
bm_info_header.biHeight=-DISPLAY_ROWS; 
bm_info_header.biPlanes=1;
bm_info_header.biBitCount=8; 
bm_info_header.biCompression=BI_RGB; 
bm_info_header.biSizeImage=0; 
bm_info_header.biXPelsPerMeter=0; 
bm_info_header.biYPelsPerMeter=0;
bm_info_header.biClrUsed=256;
bm_info_header.biClrImportant=256;
bm_info=(BITMAPINFO *)calloc(1,sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD));
bm_info->bmiHeader=bm_info_header;
for (i=0; i<256; i++)
  {
  bm_info->bmiColors[i].rgbBlue=bm_info->bmiColors[i].rgbGreen=bm_info->bmiColors[i].rgbRed=i;
  bm_info->bmiColors[i].rgbReserved=0;
  } 

SetDIBitsToDevice(hDC,0,0,DISPLAY_COLS,DISPLAY_ROWS,0,0,
			  0, /* first scan line */
			  DISPLAY_ROWS, /* number of scan lines */
			  DisplayImage,bm_info,DIB_RGB_COLORS);
ReleaseDC(MainWnd,hDC);
EndPaint(MainWnd,&Painter);

free(DisplayImage);
free(bm_info);
}

void AnimationThread(HWND AnimationWindowHandle)

{
HDC		hDC;
char	text[300];

ThreadRow=ThreadCol=0;
while (ThreadRunning == 1)
  {
  hDC=GetDC(MainWnd);
  SetPixel(hDC,ThreadCol,ThreadRow,RGB(0,255,0));	/* color the animation pixel green */
  sprintf(text,"%d,%d     ",ThreadRow,ThreadCol);
  TextOut(hDC,300,0,text,strlen(text));		/* draw text on the window */
  ReleaseDC(MainWnd,hDC);
  ThreadRow+=3;
  ThreadCol++;
  Sleep(100);		/* pause 100 ms */
  }
}

void ExplosionThread(HWND ExplosionWindowHandle)
{
	HDC		hDC;
	//	char	text[300];
	int width = 3, i;
	int cx = xmouse, cy = ymouse;

	while (width < 31)
	{
		hDC = GetDC(MainWnd);
		for (i = -width; i <= width; i++)
		{
			SetPixel(hDC, cx + i, cy + width, RGB(255, 0, 0));
			SetPixel(hDC, cx + i, cy - width, RGB(255, 0, 0));
			SetPixel(hDC, cx + width, cy + i, RGB(255, 0, 0));
			SetPixel(hDC, cx - width, cy + i, RGB(255, 0, 0));
		}
		width += 2;
		ReleaseDC(MainWnd, hDC);
		Sleep(10);
	}

}

void GenerateSobel(unsigned char* InputImage, unsigned char* OutputSobel)
{
	FILE* fpt;
	unsigned char * normX, * normY;
	signed int*smoothedX, *smoothedY, *Sobel;
	int r, c, r2, c2, sum[3], totalSum, max = -10000,min = 10000;
	double f;


	// allocate memory to generate Sobel filtered image
	smoothedX = (signed int*)calloc(ROWS * COLS, sizeof(signed int));
	smoothedY = (signed int*)calloc(ROWS * COLS, sizeof(signed int));
	Sobel = (signed int*)calloc(ROWS * COLS, sizeof(signed int));
	normX = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	normY = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	//OutputSobel = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));

	// Compute X-gradient
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			if (c == 1)
			{
				sum[0] = 0;
				sum[1] = 0;
				sum[2] = 0;
			}
			for (c2 = -1; c2 <= 1; c2++)
			{
				// Calculate full separable filter at first column
				if (c == 1)
				{
					sum[c2 + 1] += (int)(InputImage[(r - 1) * COLS + c + c2]) * (-1)
						+ (int)(InputImage[(r)*COLS + c + c2]) * (0)
						+ (int)(InputImage[(r + 1) * COLS + c + c2]) * (1);
				}
				// After first column sliding window is used
				else
				{
					if (c2 == -1)
					{
						sum[0] = sum[1];
						sum[1] = sum[2];
					}
					if (c2 == 1)
					{
						sum[2] = (int)(InputImage[(r - 1) * COLS + c + c2]) * (-1)
							+ (int)(InputImage[(r)*COLS + c + c2]) * (0)
							+ (int)(InputImage[(r + 1) * COLS + c + c2]) * (1);
					}
				}
			}
			// Second part of separable filter calculation and record min/max
			totalSum = sum[0] * 1 + sum[1] * 2 + sum[2] * 1;
			smoothedX[r * COLS + c] = totalSum;
			if (totalSum > max)
			{
				max = totalSum;
			}
			if (totalSum < min)
			{
				min = totalSum;
			}

		}
	}
	// normalize X-gradient
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			f = (smoothedX[(r * COLS) + c] - min) * ((255.0 - 0.0) / ((double)(max - min)));
			normX[r * COLS + c] = (int)f;
		}
	}
	// write out horizontal gradient image to see result
	fpt = fopen("horizontal-gradient-VS.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(normX, COLS * ROWS, 1, fpt);
	fclose(fpt);
	printf("Horizontal gradient image done!\n");
	max = -10000;
	min = 10000;
	// Compute Y-gradient
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			if (c == 1)
			{
				sum[0] = 0;
				sum[1] = 0;
				sum[2] = 0;
			}
			for (c2 = -1; c2 <= 1; c2++)
			{
				if (c == 1)
				{
					sum[c2 + 1] += (int)(InputImage[(r - 1) * COLS + c + c2]) * (1)
						+ (int)(InputImage[(r)*COLS + c + c2]) * (2)
						+ (int)(InputImage[(r + 1) * COLS + c + c2]) * (1);
				}
				else
				{
					if (c2 == -1)
					{
						sum[0] = sum[1];
						sum[1] = sum[2];
					}
					if (c2 == 1)
					{
						sum[2] = (int)(InputImage[(r - 1) * COLS + c + c2]) * (1)
							+ (int)(InputImage[(r)*COLS + c + c2]) * (2)
							+ (int)(InputImage[(r + 1) * COLS + c + c2]) * (1);
					}
				}
			}
			totalSum = sum[0] * (-1) + sum[1] * 0 + sum[2] * (1);
			smoothedY[r * COLS + c] = totalSum;
			if (totalSum > max)
			{
				max = totalSum;
			}
			if (totalSum < min)
			{
				min = totalSum;
			}
		}
	}
	// normalize image
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			f = (smoothedY[(r * COLS) + c] - min) * ((255.0 - 0.0) / ((double)(max - min)));
			normY[r * COLS + c] = (int)f;
		}
	}
	// write out vertical gradient image to see result
	fpt = fopen("vertical-gradient-VS.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(normY, COLS * ROWS, 1, fpt);
	fclose(fpt);
	printf("Vertical gradient image done!\n");
	max = -10000;
	min = 10000;
	// Compute sobel operated image with X and Y gradient
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			totalSum = sqrt(SQR((int)smoothedX[r * COLS + c]) + SQR((int)smoothedY[r * COLS + c]));
			Sobel[r * COLS + c] = totalSum;
			if (totalSum > max)
			{
				max = totalSum;
			}
			if (totalSum < min)
			{
				min = totalSum;
			}
		}
	}
	// printf("The min is %d and the max is %d\n", min, max);
	// normalize image
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			f = (Sobel[(r * COLS) + c] - min) * ((255.0 - 0.0) / ((double)(max - min)));
			OutputSobel[r * COLS + c] = (int)f;
		}
	}
	// write out sobel-filtered image to see result
	fpt = fopen("sobel-filtered-VS.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(OutputSobel, COLS * ROWS, 1, fpt);
	fclose(fpt);

}

void ActiveContour(HWND ActiveContourHandle)
{
	HDC hDC;
	int x, y, i,repeat=0,j,k;
	int moveto_x, moveto_y;
	double totalDist=0,nextDist, TotalMin = 10000;
	double colorAvg = 0;
	double IntEnergyMin1=10000, IntEnergyMin2=10000, ExtEnergyMin1=10000, ExtEnergyMin2=10000;
	double IntEnergyMax1=-10000, IntEnergyMax2=-10000, ExtEnergyMax1=-10000, ExtEnergyMax2=-10000;
	double	*NormInternalEnergy1, *NormInternalEnergy2, *NormExternalEnergy1, *NormExternalEnergy2;
	double *internalEnergy1, *internalEnergy2, *externalEnergy1, *externalEnergy2;
	double	*totalEnergy;
	int dx, dy, target;

	internalEnergy1 = (double*)calloc(window * window, sizeof(double));
	internalEnergy2 = (double*)calloc(window * window, sizeof(double));
	externalEnergy1 = (double*)calloc(window * window, sizeof(double));
	externalEnergy2 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy1 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy2 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy1 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy2 = (double*)calloc(window * window, sizeof(double));
	totalEnergy = (double*)calloc(window * window, sizeof(double));

	shrink_cont = 1;
	exp_cont = 0;

	while (repeat < iterations)
	{
		// calculation for average distance between all contour points
		totalDist = 0;
		colorAvg = 0;
		for (i = 0; i < total_points; i++)
		{
			colorAvg += (double)OriginalImage[py[i] * COLS + px[i]];
			if (i == (total_points - 1))
			{
				totalDist += sqrt((double)SQR(px[i] - px[0])
					+ (double)SQR(py[i] - py[0]));
			}
			else
			{
				totalDist += sqrt((double)SQR(px[i] - px[i + 1])
					+ (double)SQR(py[i] - py[i + 1]));
			}
		}
		totalDist /= (double)total_points;
		colorAvg /= (double)total_points;
		// iterates for a window around each contour point
		for (i = 0; i < total_points; i++)
		{
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{

					// location (in image coords) for energy calculations
					moveto_x = px[i] - window / 2 + x;
					moveto_y = py[i] - window / 2 + y;

					// distance from location to next control point
					if (i == (total_points - 1))
					{
						nextDist = sqrt((double)SQR(moveto_x - px[0])
							+ (double)SQR(moveto_y - py[0]));
					}
					else
					{

						nextDist = sqrt((double)SQR(moveto_x - px[i + 1])
							+ (double)SQR(moveto_y - py[i + 1]));
					}
					// energy calculations

					internalEnergy1[y * window + x] = SQR(nextDist); // based on distance between points
					internalEnergy2[y * window + x] = SQR(nextDist - totalDist); // based on distance between points and overall average distance
					//externalEnergy1[y * window + x] = (int)normSobel[(moveto_y)*COLS + moveto_x];
					externalEnergy1[y * window + x] = 0-(double)normSobel_sharp[(moveto_y)*COLS + moveto_x]; // based on edge strength
					externalEnergy2[y * window + x] = SQR((double)OriginalImage[(moveto_y)*COLS + moveto_x] - colorAvg); // based on average edge strength

					// stores min/max value of each energy for normalization
					if (internalEnergy1[y * window + x] > IntEnergyMax1)
					{
						IntEnergyMax1 = internalEnergy1[y * window + x];
					}
					if (internalEnergy1[y * window + x] < IntEnergyMin1)
					{
						IntEnergyMin1 = internalEnergy1[y * window + x];
					}
					if (internalEnergy2[y * window + x] > IntEnergyMax2)
					{
						IntEnergyMax2 = internalEnergy2[y * window + x];
					}
					if (internalEnergy2[y * window + x] < IntEnergyMin2)
					{
						IntEnergyMin2 = internalEnergy2[y * window + x];
					}
					if (externalEnergy1[y * window + x] > ExtEnergyMax1)
					{
						ExtEnergyMax1 = externalEnergy1[y * window + x];
					}
					if (externalEnergy1[y * window + x] < ExtEnergyMin1)
					{
						ExtEnergyMin1 = externalEnergy1[y * window + x];
					}
					if (externalEnergy2[y * window + x] > ExtEnergyMax2)
					{
						ExtEnergyMax2 = externalEnergy2[y * window + x];
					}
					if (externalEnergy2[y * window + x] < ExtEnergyMin2)
					{
						ExtEnergyMin2 = externalEnergy2[y * window + x];
					}
				}
			}
			newX[i] = px[i]; // resets the location to move to
			newY[i] = py[i];
			TotalMin = 100000;
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{
					// normalization for each energy
					NormInternalEnergy1[y * window + x] =
						(internalEnergy1[y * window + x] - (double)IntEnergyMin1) * ((1.0 - 0.0) / ((double)(IntEnergyMax1 - IntEnergyMin1)));
					NormInternalEnergy2[y * window + x] =
						(internalEnergy2[y * window + x] - (double)IntEnergyMin2) * ((1.0 - 0.0) / ((double)(IntEnergyMax2 - IntEnergyMin2)));
					NormExternalEnergy1[y * window + x] =
						(externalEnergy1[y * window + x] - (double)ExtEnergyMin1) * ((1.0 - 0.0) / ((double)(ExtEnergyMax1 - ExtEnergyMin1)));
					NormExternalEnergy2[y * window + x] =
						(externalEnergy2[y * window + x] - (double)ExtEnergyMin2) * ((1.0 - 0.0) / ((double)(ExtEnergyMax2 - ExtEnergyMin2)));

					// total energy calculation where scalars are the weights of the energies
					if (repeat < 10)
					{
						totalEnergy[y * window + x] = 50.0 * NormInternalEnergy1[y * window + x] + 35.0 * NormInternalEnergy2[y * window + x]
							+ 3.5 * NormExternalEnergy1[y * window + x] + 0.0 * NormExternalEnergy2[y * window + x];
					}
					else
					{
						totalEnergy[y * window + x] = 30.0 * NormInternalEnergy1[y * window + x] + 20.0 * NormInternalEnergy2[y * window + x]
							+ 1.5 * NormExternalEnergy1[y * window + x] + 50.0 * NormExternalEnergy2[y * window + x];
					}
					
					// next movement for contour point located at pixel with lowest energy
					if (totalEnergy[y * window + x] < TotalMin)
					{
						TotalMin = totalEnergy[y * window + x];
						
						if ( (COLS-(px[i] - window / 2 + x) < 5) || (ROWS-(py[i] - window / 2 + y) < 5) )
						{
							continue;
						}
						
						if ((COLS - (px[i] - window / 2 + x) > (COLS-5)) || (ROWS - (py[i] - window / 2 + y) > (ROWS-5)) )
						{
							continue;
						}
						
						if (OriginalImage[(py[i] - window / 2 + y) * COLS + (px[i] - window / 2 + x)] == 0)
						{
							continue;
						}
						
						newX[i] = px[i] - window / 2 + x;
						newY[i] = py[i] - window / 2 + y;
					}
				}
			}

		}
		// move the contour point
		for (i = 0; i < total_points; i++)
		{
			px[i] = newX[i];
			py[i] = newY[i];
		}
		PaintImage();
		hDC = GetDC(MainWnd);
		for (i = 0; i < total_points; i++)
		{
			for (j = -(POINT_SIZE); j <= POINT_SIZE; j++)
			{
				SetPixel(hDC, px[i], py[i] + j, RGB(0, 0, 0));
				SetPixel(hDC, px[i] + j, py[i], RGB(0, 0, 0));
			}

		}
		ReleaseDC(MainWnd, hDC);

		repeat++;
	}
	// draw line code here
	hDC = GetDC(MainWnd);
	for (i = 0; i < total_points; i++)
	{
		if (i == (total_points - 1))
		{
			dx = px[0] - px[i];
			dy = py[0] - py[i];
			target = px[0];
		}
		else
		{
			dx = px[i + 1] - px[i];
			dy = py[i + 1] - py[i];
			target = px[i + 1];
		}
		if (dx > 0)
		{
			for (x = px[i]; x < target; x++)
			{
				y = py[i] + dy * (x - px[i]) / dx;
				for (j = -1; j <= 1; j++)
				{
					for (k = -1; k <= 1; k++)
					{
						SetPixel(hDC, x+j, y+k, RGB(255, 0, 0));
					}
				}
		
			}
		}
		else if (dx < 0)
		{
			for (x = px[i]; x > target; x+=(-1))
			{
				y = py[i] + dy * (x - px[i]) / dx;
				for (j = -1; j <= 1; j++)
				{
					for (k = -1; k <= 1; k++)
					{
						SetPixel(hDC, x + j, y + k, RGB(255, 0, 0));
					}
				}
			}
		}
		
		

	}
	ReleaseDC(MainWnd, hDC);
}

void InitExpandContour(HWND ExpansionContourHandle)
{
	HDC		hDC;
	int cx = xmouse, cy = ymouse;
	int radius = 10,i,j;
	double angle, x1, y1;

	// fill data array with balloon contour points and display
	hDC = GetDC(MainWnd);
	for (angle = 0; angle < 360; angle += 3)
	{
		x1 = radius * cos(angle * M_PI / 180);
		y1 = radius * sin(angle * M_PI / 180);
		SetPixel(hDC, cx + (int)x1, cy + (int)y1, RGB(255, 0, 0));
		px2[total_points_exp] = cx + (int)x1;
		py2[total_points_exp] = cy + (int)y1;
		total_points_exp++;
	}
	/*
	for (i = 0; i < total_points_exp - 1; i++)
	{
		for (j = -POINT_SIZE; j <= POINT_SIZE; j++)
		{
			SetPixel(hDC, px2[i] + j, py2[i], RGB(255, 255, 255));
			SetPixel(hDC, px2[i], py2[i] + j, RGB(255, 255, 255));
		}
	}
	*/

}

void BalloonContour(HWND BalloonContourHandle)
{
	HDC		hDC;
	int cx, cy, i, j, y, x, repeat=0,k;
	int move_x, move_y;
	double distance, TotalMin, threshold = 80;
	double x1, y1, x2, y2, dotp, mag1, mag2, min_a, min_b;
	double nextDist, prevDist, edgeAvg, colorAvg = 0; 
	double colorR = 0, colorG = 0, colorB = 0;
	double *internalEnergy1, * externalEnergy1, * internalEnergy2, *internalEnergy3, *externalEnergy2;
	double *NormInternalEnergy1, * NormInternalEnergy2, * NormExternalEnergy1, *NormExternalEnergy2;
	double *NormInternalEnergy3, *externalEnergy3, *NormExternalEnergy3;
	double* totalEnergy;
	double IntEnergyMin1 = 10000, IntEnergyMin2 = 10000, IntEnergyMin3 = 10000, ExtEnergyMin2 = 10000, ExtEnergyMin1 = 10000;
	double	IntEnergyMax1 = -10000, IntEnergyMax2 = -10000, IntEnergyMax3 = -10000,ExtEnergyMax2 = -10000, ExtEnergyMax1 = -10000;
	double ExtEnergyMin3 = 10000, ExtEnergyMax3 = -10000;

	internalEnergy1 = (double*)calloc(window*window, sizeof(double));
	internalEnergy2 = (double*)calloc(window * window, sizeof(double));
	internalEnergy3 = (double*)calloc(window * window, sizeof(double));
	externalEnergy1 = (double*)calloc(window * window, sizeof(double));
	externalEnergy2 = (double*)calloc(window * window, sizeof(double));
	externalEnergy3 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy1 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy2 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy3 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy1 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy2 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy3 = (double*)calloc(window * window, sizeof(double));
	totalEnergy = (double*)calloc(window * window, sizeof(double));

	exp_cont = 1;
	shrink_cont = 0;

	cx = cy = 0;
	ExtEnergyMax1 = 0;
	ExtEnergyMin1 = -255;
	while (repeat < iterations)
	{
		//calculation for average edge strength and color
		colorAvg = 0;
		edgeAvg = 0;
		for (i = 0; i < total_points_exp; i++)
		{
			colorAvg += (double)BlurredImage[py[i] * COLS + px[i]];
			edgeAvg += (double)normSobel_blur[py[i] * COLS + px[i]];
		}
		colorAvg /= (double)total_points_exp;
		edgeAvg /= (double)total_points_exp;

		// find the centroid of the contour points
		for (i = 0; i < total_points_exp; i++)
		{
			cx += px2[i];
			cy += py2[i];
		}
		cx /= total_points_exp;
		cy /= total_points_exp;
		
		for (i = 0; i < total_points_exp; i++)
		{
			x1 = px2[i] - cx;
			y1 = py2[i] - cy;
			mag1 = sqrt(SQR(x1) + SQR(y1));
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{
					move_x = px2[i] - window / 2 + x;
					move_y = py2[i] - window / 2 + y;

					distance = sqrt((double)SQR(cx - move_x) + (double)SQR(cy - move_y)); // distance from centroid 

					x2 = move_x - cx;
					y2 = move_y - cy;
					mag2 = sqrt(SQR(x2) + SQR(y2));
					dotp = x1 * x2 + y1 * y2;
				
					
					// distance from location to next and previous control point
					if (i == (total_points_exp - 1))
					{
						nextDist = sqrt((double)SQR(move_x - px2[0])
							+ (double)SQR(move_y - py2[0]));
						prevDist = (int)sqrt((double)SQR(move_x - px2[i-1])
							+ (double)SQR(move_y - py2[i-1]));
					}
					else
					{
						prevDist = (int)sqrt((double)SQR(move_x - px2[i-1])
							+ (double)SQR(move_y - py2[i-1]));
						nextDist = sqrt((double)SQR(move_x - px2[i + 1])
							+ (double)SQR(move_y - py2[i + 1]));
					}
					if (i == 0)
					{
						prevDist = (int)sqrt((double)SQR(move_x - px2[total_points_exp-1])
							+ (double)SQR(move_y - py2[total_points_exp - 1]));
					}
					
					internalEnergy1[y * window + x] = 0.0 - (double)distance; // distance from centroid
					internalEnergy2[y * window + x] = acos(dotp / (mag1 * mag2)) * 180 / M_PI; // angle between vectors (point-centroid) and (window-centroid)
					internalEnergy3[y * window + x] = SQR(nextDist-prevDist); // distance from location to next and previous control point
					externalEnergy1[y * window + x] = 0.0-(double)normSobel_blur[(move_y)*COLS + move_x]; // edge strength
					externalEnergy2[y * window + x] = SQR((double)BlurredImage[(move_y)*COLS + move_x] - colorAvg); // color
					externalEnergy3[y * window + x] = SQR(edgeAvg - (double)normSobel_blur[(move_y)*COLS + move_x]); // average edge strength


					// stores min/max value of each energy for normalization
					if (internalEnergy1[y * window + x] > IntEnergyMax1)
					{
						IntEnergyMax1 = internalEnergy1[y * window + x];
					}
					if (internalEnergy1[y * window + x] < IntEnergyMin1)
					{
						IntEnergyMin1 = internalEnergy1[y * window + x];
					}
					if (internalEnergy2[y * window + x] > IntEnergyMax2)
					{
						IntEnergyMax2 = internalEnergy2[y * window + x];
					}
					if (internalEnergy2[y * window + x] < IntEnergyMin2)
					{
						IntEnergyMin2 = internalEnergy2[y * window + x];
					}
					if (internalEnergy3[y * window + x] > IntEnergyMax3)
					{
						IntEnergyMax3 = internalEnergy3[y * window + x];
					}
					if (internalEnergy3[y * window + x] < IntEnergyMin3)
					{
						IntEnergyMin3 = internalEnergy3[y * window + x];
					}
					/*
					if (externalEnergy1[y * window + x] > ExtEnergyMax1)
					{
						ExtEnergyMax1 = externalEnergy1[y * window + x];
					}
					if (externalEnergy1[y * window + x] < ExtEnergyMin1)
					{
						ExtEnergyMin1 = externalEnergy1[y * window + x];
					}
					*/
					if (externalEnergy2[y * window + x] > ExtEnergyMax2)
					{
						ExtEnergyMax2 = externalEnergy2[y * window + x];
					}
					if (externalEnergy2[y * window + x] < ExtEnergyMin2)
					{
						ExtEnergyMin2 = externalEnergy2[y * window + x];
					}
					if (externalEnergy3[y * window + x] > ExtEnergyMax3)
					{
						ExtEnergyMax3 = externalEnergy3[y * window + x];
					}
					if (externalEnergy3[y * window + x] < ExtEnergyMin3)
					{
						ExtEnergyMin3 = externalEnergy3[y * window + x];
					}
					
				}
			}
			newX2[i] = px2[i]; // resets the location to move to
			newY2[i] = py2[i];
			TotalMin = 1000000;
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{
					// normalization for each energy
					NormInternalEnergy1[y * window + x] =
						(internalEnergy1[y * window + x] - (double)IntEnergyMin1) * ((1.0 - 0.0) / ((double)(IntEnergyMax1 - IntEnergyMin1)));
					NormInternalEnergy2[y * window + x] =
						(internalEnergy2[y * window + x] - (double)IntEnergyMin2) * ((1.0 - 0.0) / ((double)(IntEnergyMax2 - IntEnergyMin2)));
					NormInternalEnergy3[y * window + x] =
						(internalEnergy3[y * window + x] - (double)IntEnergyMin3) * ((1.0 - 0.0) / ((double)(IntEnergyMax3 - IntEnergyMin3)));
					NormExternalEnergy1[y * window + x] =
						(externalEnergy1[y * window + x] - (double)ExtEnergyMin1) * ((1.0 - 0.0) / ((double)(ExtEnergyMax1 - ExtEnergyMin1)));
					NormExternalEnergy2[y * window + x] =
						(externalEnergy2[y * window + x] - (double)ExtEnergyMin2) * ((1.0 - 0.0) / ((double)(ExtEnergyMax2 - ExtEnergyMin2)));
					NormExternalEnergy3[y * window + x] =
						(externalEnergy3[y * window + x] - (double)ExtEnergyMin3) * ((1.0 - 0.0) / ((double)(ExtEnergyMax3 - ExtEnergyMin3)));


					// total energy calculation where scalars are the weights of the energies
					if (repeat < 20)
					{
						totalEnergy[y * window + x] = 50.0 * NormInternalEnergy1[y * window + x] + 140.0 * NormInternalEnergy2[y * window + x]
							+ 25.0 * NormInternalEnergy3[y * window + x] + 11.0 * NormExternalEnergy1[y * window + x] + 39.0 * NormExternalEnergy2[y * window + x]
							- 40.0 * NormExternalEnergy3[y * window + x];
					}
					else
					{
						totalEnergy[y * window + x] = 15.0 * NormInternalEnergy1[y * window + x] + 80.0 * NormInternalEnergy2[y * window + x]
							+ 35.0 * NormInternalEnergy3[y * window + x] + 8.5 * NormExternalEnergy1[y * window + x] + 80.0 * NormExternalEnergy2[y * window + x]
							- 100.0 * NormExternalEnergy3[y * window + x];
					}
					//20 100
					// next movement for contour point located at pixel with lowest energy
					if (totalEnergy[y * window + x] < TotalMin)
					{
						if ((COLS - (px2[i] - window / 2 + x) < 10) || (ROWS - (py2[i] - window / 2 + y) < 10))
						{
							continue;
						}
						if ((COLS - (px2[i] - window / 2 + x) > (COLS - 10)) || (ROWS - (py2[i] - window / 2 + y) > (ROWS - 10)))
						{
							continue;
						}
						if (OriginalImage[(py2[i] - window / 2 + y) * COLS + (px2[i] - window / 2 + x)] == 0)
						{
							continue;
						}
						
						TotalMin = totalEnergy[y * window + x];
						newX2[i] = px2[i] - window / 2 + x;
						newY2[i] = py2[i] - window / 2 + y;
						
					}

				}
			}
		}
		IntEnergyMax1 = IntEnergyMax2 = IntEnergyMax3 = -10000;
		IntEnergyMin1 = IntEnergyMin2 = IntEnergyMin3 = 10000;
		//ExtEnergyMax1 = ExtEnergyMax2 = ExtEnergyMax3 =  -10000;
		//ExtEnergyMin1 = ExtEnergyMin2 = ExtEnergyMin3 = 10000;
		ExtEnergyMax2 = ExtEnergyMax3 = -10000;
		ExtEnergyMin2 = ExtEnergyMin3 = 10000;
		// move the contour point
		for (i = 0; i < total_points_exp; i++)
		{
			px2[i] = newX2[i];
			py2[i] = newY2[i];
		}
		PaintImage();
		hDC = GetDC(MainWnd);
		for (i = 0; i < total_points_exp; i++)
		{
			for (j = -(POINT_SIZE); j <= POINT_SIZE; j++)
			{
				SetPixel(hDC, px2[i], py2[i] + j, RGB(255, 0, 0));
				SetPixel(hDC, px2[i] + j, py2[i], RGB(255, 0, 0));
			}

		}
		ReleaseDC(MainWnd, hDC);
		Sleep(100);
		repeat++;
	}
}

void ManualContour(HWND ManualContour)
{
	HDC hDC;
	int repeat = 0, i, j, x, y, move_x, move_y, temp_total;
	int dx, dy, k, target, next = 0;
	//int *temp_px, *temp_py, * newpx, * newpy;
	double* internalEnergy1, * externalEnergy1, * internalEnergy2, * internalEnergy3, * externalEnergy2;
	double* NormInternalEnergy1, * NormInternalEnergy2, * NormExternalEnergy1, * NormExternalEnergy2;
	double* NormInternalEnergy3;
	double* externalEnergy3, * NormExternalEnergy3;
	double* totalEnergy;
	double IntEnergyMin1 = 10000, IntEnergyMin2 = 10000, IntEnergyMin3 = 10000, ExtEnergyMin2 = 10000, ExtEnergyMin1 = 10000;
	double IntEnergyMax1 = -10000, IntEnergyMax2 = -10000, IntEnergyMax3 = -10000, ExtEnergyMax2 = -10000, ExtEnergyMax1 = -10000;
	double ExtEnergyMin3 = 10000, ExtEnergyMax3 = -10000;
	double colorAvg,colorR, colorG, colorB, nextDist,prevDist, TotalMin, threshold = 125.0, edgeAvg;

	internalEnergy1 = internalEnergy2 = internalEnergy3 = (double*)calloc(window * window, sizeof(double));
	externalEnergy1 = externalEnergy2 = externalEnergy3 = (double*)calloc(window * window, sizeof(double));
	NormInternalEnergy1 = NormInternalEnergy2 = NormInternalEnergy3 = (double*)calloc(window * window, sizeof(double));
	NormExternalEnergy1 = NormExternalEnergy2 = NormExternalEnergy3 = (double*)calloc(window * window, sizeof(double));
	totalEnergy = (double*)calloc(window * window, sizeof(double));
	
	if (shrink_cont == 1)
	{	
		temp_total = total_points;
		for (i = 0; i < temp_total; i++)
		{
			temp_px[i] = px[i];
			temp_py[i] = py[i];
		}
	}
	else if (exp_cont == 1)
	{
		temp_total = total_points_exp;
		for (i = 0; i < temp_total; i++)
		{
			temp_px[i] = px2[i];
			temp_py[i] = py2[i];
		}
	}

	for (repeat = 0; repeat < iterations; repeat++)
	{
		colorAvg = 0;
		edgeAvg = 0;
		for (i = 0; i < temp_total; i++)
		{
			colorAvg += (double)BlurredImage[temp_py[i] * COLS + temp_px[i]];
			edgeAvg += (double)normSobel_blur[temp_py[i] * COLS + temp_px[i]];
		}
		colorAvg /= (double)temp_total;
		edgeAvg /= (double)temp_total;
		for (i = 0; i < temp_total; i++)
		{
			if (i == selected_point)
			{
				//next = 1;
				continue;		
			}
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{
					move_x = temp_px[i] - window / 2 + x;
					move_y = temp_py[i] - window / 2 + y;

					// distance from location to next and previous control point 
					if (i == (temp_total - 1))
					{
						nextDist = sqrt((double)SQR(move_x - temp_px[0])
							+ (double)SQR(move_y - temp_py[0]));
						prevDist = (int)sqrt((double)SQR(move_x - temp_px[i - 1])
							+ (double)SQR(move_y - temp_py[i - 1]));
					}
					else
					{
						prevDist = (int)sqrt((double)SQR(move_x - temp_px[i - 1])
							+ (double)SQR(move_y - temp_py[i - 1]));
						nextDist = sqrt((double)SQR(move_x - temp_px[i + 1])
							+ (double)SQR(move_y - temp_py[i + 1]));
					}
					if (i == 0)
					{
						prevDist = (int)sqrt((double)SQR(move_x - temp_px[temp_total - 1])
							+ (double)SQR(move_y - temp_py[temp_total - 1]));
					}
					internalEnergy1[y * window + x] = SQR(nextDist - prevDist);
					externalEnergy1[y * window + x] = 0.0 - (double)normSobel_blur[(move_y)*COLS + move_x];
					externalEnergy2[y * window + x] = SQR((double)BlurredImage[(move_y)*COLS + move_x] - colorAvg);
					externalEnergy3[y * window + x] = SQR(edgeAvg - (double)normSobel_blur[(move_y)*COLS + move_x]);


					// stores min/max value of each energy for normalization
					if (internalEnergy1[y * window + x] > IntEnergyMax1)
					{
						IntEnergyMax1 = internalEnergy1[y * window + x];
					}
					if (internalEnergy1[y * window + x] < IntEnergyMin1)
					{
						IntEnergyMin1 = internalEnergy1[y * window + x];
					}
					if (externalEnergy1[y * window + x] > ExtEnergyMax1)
					{
						ExtEnergyMax1 = externalEnergy1[y * window + x];
					}
					if (externalEnergy1[y * window + x] < ExtEnergyMin1)
					{
						ExtEnergyMin1 = externalEnergy1[y * window + x];
					}
					if (externalEnergy2[y * window + x] > ExtEnergyMax2)
					{
						ExtEnergyMax2 = externalEnergy2[y * window + x];
					}
					if (externalEnergy2[y * window + x] < ExtEnergyMin2)
					{
						ExtEnergyMin2 = externalEnergy2[y * window + x];
					}
					if (externalEnergy3[y * window + x] > ExtEnergyMax3)
					{
						ExtEnergyMax3 = externalEnergy3[y * window + x];
					}
					if (externalEnergy3[y * window + x] < ExtEnergyMin3)
					{
						ExtEnergyMin3 = externalEnergy3[y * window + x];
					}

				}
			}
			newpx[i] = temp_px[i]; // resets the location to move to
			newpy[i] = temp_py[i];
			TotalMin = 1000000;
			for (y = 0; y < window; y++)
			{
				for (x = 0; x < window; x++)
				{
					// normalization for each energy
					NormInternalEnergy1[y * window + x] =
						(internalEnergy1[y * window + x] - (double)IntEnergyMin1) * ((1.0 - 0.0) / ((double)(IntEnergyMax1 - IntEnergyMin1)));
					NormExternalEnergy1[y * window + x] =
						(externalEnergy1[y * window + x] - (double)ExtEnergyMin1) * ((1.0 - 0.0) / ((double)(ExtEnergyMax1 - ExtEnergyMin1)));
					NormExternalEnergy2[y * window + x] =
						(externalEnergy2[y * window + x] - (double)ExtEnergyMin2) * ((1.0 - 0.0) / ((double)(ExtEnergyMax2 - ExtEnergyMin2)));

					if (repeat < 20)
					{
						totalEnergy[y * window + x] = 2.0 * NormInternalEnergy1[y * window + x] + 10.0 * NormExternalEnergy1[y * window + x]
							+ 1.0 * NormExternalEnergy2[y * window + x] + 25.0 * NormExternalEnergy3[y * window + x];
					}
					else 
					{
						totalEnergy[y * window + x] = 10.0 * NormInternalEnergy1[y * window + x] + 10.0 * NormExternalEnergy1[y * window + x]
							+ 2.0 * NormExternalEnergy2[y * window + x] + 20.0 * NormExternalEnergy3[y * window + x];
					}
				
					// next movement for contour point located at pixel with lowest energy
					if (totalEnergy[y * window + x] < TotalMin)
					{
						if ((COLS - (temp_px[i] - window / 2 + x) < 5) || (ROWS - (temp_py[i] - window / 2 + y) < 5))
						{
							continue;
						}
						if ((COLS - (temp_px[i] - window / 2 + x) > (COLS - 5)) || (ROWS - (temp_py[i] - window / 2 + y) > (ROWS - 5)))
						{
							continue;
						}
						if (OriginalImage[(temp_py[i] - window / 2 + y) * COLS + (temp_px[i] - window / 2 + x)] == 0)
						{
							continue;
						}
						TotalMin = totalEnergy[y * window + x];
						newpx[i] = temp_px[i] - window / 2 + x;
						newpy[i] = temp_py[i] - window / 2 + y;
						
					}

				}
			}
		}
		IntEnergyMax1 = IntEnergyMax2 = IntEnergyMax3 = -10000;
		IntEnergyMin1 = IntEnergyMin2 = IntEnergyMin3 = 10000;
		ExtEnergyMax1 = ExtEnergyMax2 = ExtEnergyMax3 = -10000;
		ExtEnergyMin1 = ExtEnergyMin2 = ExtEnergyMin3 = 10000;
		// move the contour point
		for (i = 0; i < temp_total; i++)
		{
			if (i == selected_point)
			{
				continue;
			}
			temp_px[i] = newpx[i];
			temp_py[i] = newpy[i];
		}
		PaintImage();
		hDC = GetDC(MainWnd);
		for (i = 0; i < temp_total; i++)
		{
			for (j = -(POINT_SIZE); j <= POINT_SIZE; j++)
			{
				SetPixel(hDC, temp_px[i], temp_py[i] + j, RGB(255, 0, 0));
				SetPixel(hDC, temp_px[i] + j, temp_py[i], RGB(255, 0, 0));
			}

		}
		ReleaseDC(MainWnd, hDC);
		Sleep(100);
		repeat++;
	}

	if (shrink_cont == 1)
	{
		for (i = 0; i < temp_total; i++)
		{
			px[i] = temp_px[i];
			py[i] = temp_py[i];
		}
		
		// draw line code here
		hDC = GetDC(MainWnd);
		for (i = 0; i < total_points; i++)
		{
			if (i == (total_points - 1))
			{
				dx = px[0] - px[i];
				dy = py[0] - py[i];
				target = px[0];
			}
			else
			{
				dx = px[i + 1] - px[i];
				dy = py[i + 1] - py[i];
				target = px[i + 1];
			}
			if (dx > 0)
			{
				for (x = px[i]; x < target; x++)
				{
					y = py[i] + dy * (x - px[i]) / dx;
					for (j = -1; j <= 1; j++)
					{
						for (k = -1; k <= 1; k++)
						{
							SetPixel(hDC, x + j, y + k, RGB(255, 0, 0));
						}
					}

				}
			}
			else if (dx < 0)
			{
				for (x = px[i]; x > target; x += (-1))
				{
					y = py[i] + dy * (x - px[i]) / dx;
					for (j = -1; j <= 1; j++)
					{
						for (k = -1; k <= 1; k++)
						{
							SetPixel(hDC, x + j, y + k, RGB(255, 0, 0));
						}
					}
				}
			}
		}
		ReleaseDC(MainWnd, hDC);
	}
	else if (exp_cont == 1)
	{
		for (i = 0; i < temp_total; i++)
		{
			px2[i] = temp_px[i];
			py2[i] = temp_py[i];
		}
	}
}



void BlurImage(void)
{
	int r, c, r2, c2, sum = 0;
	FILE* fpt;

	normSobel_blur = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));

	/* smooth image, skipping the border points */
	for (r = 3; r < ROWS - 3; r++)
	{
		for (c = 3; c < COLS - 3; c++)
		{

			if (c == 3)
			{
				sum = 0;
			}
			for (r2 = -3; r2 <= 3; r2++)
			{
				if (c == 3)
				{
					sum += (int)(OriginalImage[(r + r2) * COLS + c - 3])
						+ (int)(OriginalImage[(r + r2) * COLS + c - 2])
						+ (int)(OriginalImage[(r + r2) * COLS + c - 1])
						+ (int)(OriginalImage[(r + r2) * COLS + c])
						+ (int)(OriginalImage[(r + r2) * COLS + c + 1])
						+ (int)(OriginalImage[(r + r2) * COLS + c + 2])
						+ (int)(OriginalImage[(r + r2) * COLS + c + 3]);
				}
				else
				{
					sum = sum + (int)(OriginalImage[(r + r2) * COLS + c + 3])
						- (int)(OriginalImage[(r + r2) * COLS + c - 4]);
				}
			}
			BlurredImage[r * COLS + c] = sum / 49;

		}

	}
	/* write out smoothed image to see result */
	fpt = fopen("blurred-image.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(BlurredImage, COLS * ROWS, 1, fpt);
	fclose(fpt);

	GenerateSobel(BlurredImage, normSobel_blur);
	fpt = fopen("sobel-blurred-image.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(normSobel_blur, COLS * ROWS, 1, fpt);
	fclose(fpt);

}

void SharpenImage(void)
{
	int r, c, r2, c2, sum = 0, max = -10000, min = 10000;
	FILE *fpt, *fpt2;
	signed int* FilteredImage;
	double f;

	FilteredImage = (signed int*)calloc(ROWS * COLS, sizeof(signed int));
	normSobel_sharp = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));

	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			sum = 0;
			for (r2 = -1; r2 <= 1; r2++)
			{
				for (c2 = -1; c2 <= 1; c2++)
				{
					sum += SharpImage[(r + r2) * COLS + c + c2] * sharp_kernel[r2 + 1][c2 + 1];
				}
			}
			FilteredImage[r * COLS + c] = sum;
			//SharpImage[r * COLS + c] = sum;
			if (sum > max)
			{
				max = sum;
			}
			else if (sum < min)
			{
				min = sum;
			}

		}
	}
	/* normalize image */
	for (r = 1; r < ROWS - 1; r++)
	{
		for (c = 1; c < COLS - 1; c++)
		{
			f = (FilteredImage[(r * COLS) + c] - min) * ((255.0 - 0.0) / ((double)(max - min)));
			SharpImage[r * COLS + c] = (int)f;
		}
	}
	
	// write out sharp image to see result
	fpt = fopen("sharp-image.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(SharpImage, COLS * ROWS, 1, fpt);
	fclose(fpt);

	GenerateSobel(SharpImage, normSobel_sharp);
	// write out sobel-filtered image to see result
	fpt = fopen("sharp-sobel-image.ppm", "wb");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(normSobel_sharp, COLS * ROWS, 1, fpt);
	fclose(fpt);
}