
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320
#define MAX_QUEUE			10000
#define	POINT_SIZE			7


char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;
int		bigDots = 0;
int		ActiveContourDraw = 0;
int		ActiveContourPoint = 0;
int		point_select = 0, point_move = 0, x_change, y_change;
int		click = 0;
int		clickr = 0;
int		xmouse;
int		ymouse;

		// Color data
const COLORREF rgbRed = 0x000000FF;
const COLORREF rgbGreen = 0x0000FF00;
const COLORREF rgbBlue = 0x00FF0000;
const COLORREF rgbOrange = 0x000051FF;
const COLORREF rgbYellow = 0x0000FFFF;
const COLORREF rgbPurple = 0x00FF00FF;
COLORREF ChosenColor = 0x000000FF;


		// Image data
unsigned char*  RGB_image;
unsigned char*	OriginalImage;
unsigned char*  BlurredImage;
unsigned char*  SharpImage;
unsigned char*  normSobel;
unsigned char*	normSobel_sharp; // Normalized Sobel filtered 
unsigned char*  normSobel_blur; // Normalized Sobel filtered image

int				ROWS,COLS;

	// Active Contour Data Shrink
int	draw_x[1200], px[200], newX[200];
int	draw_y[1200], py[200], newY[200];
int	total_points=0;
int	cont_points = 0;
int	window=17;
int	iterations=30;
int	weights[3];

	// Active Contour Expand
int	circle_x[600], px2[120], newX2[120];
int	circle_y[600], py2[120], newY2[120];
int	total_points_exp = 0;
int	cont_points_exp = 0;

	// Active Contour Point Select
int	selected_point;
int	shrink_cont = 0, exp_cont = 0;
int temp_px[200], newpx[200];
int temp_py[200], newpy[200];

	// Image Processing Kernels
int	sharp_kernel[3][3] = {
	{0,-1,0},
	{-1,10,-1},
	{0,-1,0}
};
double gaussian_blue[5][5] = {
	{1/256.0, 4/256.0, 6/256.0, 4/256.0 ,1/256.0},
	{4/256.0, 16/256.0, 24/256.0, 16/256.0, 4/256.0},
	{6/256.0, 24/256.0, 36/256.0, 24/256.0, 6/256.0},
	{4/256.0, 16/256.0, 24/256.0, 16/256.0, 4/256.0},
	{1/256.0, 4/256.0, 6/256.0, 4/256.0, 1/256.0}
};


#define TIMER_SECOND	1			/* ID of timer used for animation */

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;

		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
void PaintImage();
void AnimationThread(void *);		/* passes address of window */
void ExplosionThread(void*);
void ActiveContour(void*);
void GenerateSobel(unsigned char* InputImage, unsigned char* OutputSobel);
void InitExpandContour(void*);
void BalloonContour(void*);
void ManualContour(void*);
void PNM2PPM(void);
void BlurImage(void);
void SharpenImage(void);
