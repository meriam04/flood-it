#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 2
#define NUM_BOXES 8

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Begin part3.c code for Lab 7

//void	draw_line(int x0, int y0, int x1, int y1, short int line_color);
void	plot_pixel(int x, int y, short int line_color);
void	clear_screen();
void	swap(int* x, int* y);
void	draw_box(int x, int y, short int box_color);
void	wait_for_vsync();

	
volatile int pixel_buffer_start; // global variable
int N=14;         //14x14 board, later can be selected 

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables
    int count=25;     //turn downcounter, later can be loaded from SW
    int xbox [N][N];  
    int ybox [N][N];
    int color[]={0xFFFF,0xFFE0,0xF800,0x07E0,0x001F,0x07FF,0xF81F,GREY,PINK,ORANGE};
    int color_box[N][N];
    int flooded [N][N];
	short int flood_color=0xFFFF;

      // initialize location and direction of rectangles
    for (int i=0;i<N;i++){
      for (int j=0;j<N;j++){
        xbox[i][j]=319/N +i;      //rounding err?
        ybox[i][j]=239/N +j;	//240-10+1
        flooded[i][j]=0;//unless i=[0,0]
        color_box[i][j]=color[rand()%10];
      }
    }
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();

    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the front pixel buffer for initial screen clear
    
/* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;

    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw to the back buffer

    while (N!=0) //downcounter hits 0, game ends (escape)
    {
        //if flood, change color values of corresponding boxes
      
        

        // code for drawing the boxes and lines (not shown)
        for (int i=0;i<N;i++){
		for (int j=0;j<N;j++)
          		draw_box(xbox[i][j],ybox[i][j],color_box[i][j]);            
        }
              
	    //seperate interrupt for mouse click value here tells us what color is flooding (load into flood_color)
      
        // code for updating flood value of boxes based on their color AND proximity to a flooded box, include recursion
	      //for (int i=0;i<N;i++)//but 2d
		
		
        N--;  //is this legal?
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
    return 0;
}


// code for subroutines
void	wait_for_vsync(){
	volatile int* pixel_ctrl_ptr = (int*)0xFF203020;
	int status;
	//write 1 into front buffer register
	*pixel_ctrl_ptr = 1;
	status = *(pixel_ctrl_ptr + 3);
	//delay until s=0, while loop
	while((status&0x1)!= 0)
		status = *(pixel_ctrl_ptr + 3);
}

void	draw_box(int x, int y, short int box_color){		////////////CHECK DIM (width of each box)
	for(int i=x;i<=x+(319/N);i++){
		for(int j=y;j<=y+(239/N);j++)
			plot_pixel(i,j,box_color);
	}
}

void	draw_line(int x0, int y0, int x1, int y1, short int line_color){		//if we want to draw a border later (maybe swap with meriam's? mine is unreliable asf)
	int is_steep= ABS(x1-x0) < ABS(y1-y0);
	
	if (x0>x1){
		swap(&x0,&x1);
		swap(&y0,&y1);
	}
	if (is_steep){
		swap(&x0,&y0);
		swap(&x1,&y1);
	}
	int delta_x= x1-x0;
	int delta_y= ABS(y1-y0);
	int error= -(delta_x/2);
	
	int	y_step;
	int y=y0;
	
	if	(y0 < y1)
		y_step=1;
	else
		y_step=-1;
	
	for(int x=x0; x<x1; x++){   ////////////only<not<=
		if(is_steep)
			plot_pixel(y,x,line_color);
		else
			plot_pixel(x,y,line_color);
		
		error+=delta_y;
		if (error>=0){
			y+=y_step;
			error-=delta_x;
		}
	}
}

void	swap(int *x, int *y){
	int temp=*x;
	*x=*y;
	*y=temp;
}

void	clear_screen(){
	for(int y=0;y<=239;y++){
		for (int x=0;x<=319;x++)
			plot_pixel(x,y,0x0000);
	}
}			
		

void plot_pixel(int x, int y, short int line_color)///lakakakaka
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}
