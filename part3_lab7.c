/* This files provides address values that exist in the system */
// adding a funky little comment :D
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

// Begin part3.c code for Lab 7

void	draw_line(int x0, int y0, int x1, int y1, short int line_color);
void	plot_pixel(int x, int y, short int line_color);
void	clear_screen();
void	swap(int* x, int* y);
void	draw_box(int x, int y, short int box_color);
void	wait_for_vsync();

	
volatile int pixel_buffer_start; // global variable

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables
	volatile int N=8;
	volatile int xbox [N];
	volatile int ybox [N];
	volatile int color[]={0xFFFF,0xFFE0,0xF800,0x07E0,0x001F,0x07FF,0xF81F,GREY,PINK,ORANGE};
	volatile int color_box[N];
	volatile int dx_box[N];
	volatile int dy_box[N];

    // initialize location and direction of rectangles
	for (int i=0;i<N;i++){
		xbox[i]=rand()%310;	//319-width +1
		ybox[i]=rand()%230;	//240-10+1
		dx_box[i]=((rand()% 2)*2)-1; //gives -1 or 1
		dy_box[i]=((rand()% 2)*2)-1;
		color_box[i]=color[rand()%10];
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

    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        //either specifically or clear_screen()
	clear_screen();

        // code for drawing the boxes and lines (not shown)
	for (int i=0;i<N;i++){
		draw_box(xbox[i],ybox[i],color_box[i]);
		//if(i>0)
			draw_line(xbox[i], ybox[i], xbox[(i+1)% N], ybox[(i+1)%N], color_box[i]);
		
	}
	//draw_line(xbox[N-1], ybox[N-1], xbox[0], ybox[0], GREY);

        // code for updating the locations of boxes w bounds
	for (int i=0;i<N;i++){
		if ((xbox[i]<2)||(xbox[i]>309))
			dx_box[i]*=(-1);
		if ((ybox[i]<2)||(ybox[i]>229))
			dy_box[i]*=(-1);
		xbox[i]+=dx_box[i];
		ybox[i]+=dy_box[i];
	}	

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
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

void	draw_box(int x, int y, short int box_color){
	for(int i=x;i<=x+9;i++){
		for(int j=y;j<=y+9;j++)
			plot_pixel(i,j,box_color);
	}
}

void	draw_line(int x0, int y0, int x1, int y1, short int line_color){
	int is_steep= ABS(x1-x0) < ABS(y1-y0);
	/*if (ABS(x1-x0) < ABS(y1-y0))
		is_steep=1;
	else 
		is_steep=0;*/
	
	if (x0>x1){
		/*int temp=x0;
		x0=x1;
		x1=temp;
		temp=y0;
		y0=y1;
		y1=temp;*/
		swap(&x0,&x1);
		swap(&y0,&y1);
	}
	if (is_steep){
		/*int temp=x0;
		x0=y0;
		y0=temp;
		temp=x1;
		x1=y1;
		y1=temp;*/
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
	
	//int y=y0;
	for(int x=x0; x<x1; x++){////////////only<not<=
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

