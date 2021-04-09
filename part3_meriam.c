/* This files provides address values that exist in the system */

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
#define BLACK 0x0000

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 10
#define NUM_BOXES 8
#define NUM_ROWS 10
#define NUM_COLS 10
#define NUM_COLOURS 6

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>

// Begin part3.c code for Lab 7
volatile int pixel_buffer_start; // global variable

typedef struct {
    int x_pos;
    int y_pos;
    int flood;
    short int colour;
} BoxInfo;

// function definitions
void clear_screen();
void swap(int* x, int* y);
void draw_line(int x1, int y1, int x2, int y2, short int color);
void plot_pixel(int x, int y, short int line_color);
void wait_for_vsync();
void draw_box(int x, int y, int size, short int color);

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    //volatile int * previous_pixel_ctrl_ptr; // = pixel_ctrl_ptr;
    // int N = NUM_BOXES;
    int rows = RESOLUTION_X;
    int cols = RESOLUTION_Y;
    int size = BOX_LEN;
    
    // declare other variables(not shown)
    short int colours[] = {YELLOW, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE, WHITE, RED};
    
    //declare 2D array of structs
    BoxInfo **boxes;
    boxes = (BoxInfo**)malloc(sizeof(BoxInfo*)*(rows/size));
    for (int i = 0; i < (rows/size); ++i){
        boxes[i] = (BoxInfo*)malloc(sizeof(BoxInfo) * (cols/size));
    }
    
    // initialize 2D array of structs for the boxes
    for (int i = 0; i < (rows/size); ++i){
        for (int j = 0; j < (cols/size); ++j){
            // create element in array with necessary information
            BoxInfo element;
            element.x_pos = i*size;
            element.y_pos = j*size;
            // if border, colour should be black, otherwise should be random
            if (i == 0 || j == 0 || i == (rows/size - 1) || j == (cols/size - 1)){
                element.colour = BLACK;
            }
            else {
                element.colour = colours[rand() % NUM_COLOURS];
            }
            element.flood = FALSE;
            
            // put element into array
            boxes[i][j] = element;
        }
    }

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; // first store the address in the
                                        // back buffer
    
    /*now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    
    clear_screen();

    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        // could either redraw all to black or only redraw the ones that were drawn before
        
        // probably replace this with only redrawing boxes with colours changed
        
        /* if (iteration == 0){
            clear_screen();
        }
        else if (pixel_buffer_start == FPGA_ONCHIP_BASE){
            for (int i = 0; i < NUM_BOXES; i++){
                draw_box(prev_first_x[i], prev_first_y[i], BOX_LEN, BLACK);
                draw_line(prev_first_x[i], prev_first_y[i], prev_first_x[(i+1) % NUM_BOXES], prev_first_y[(i+1) % NUM_BOXES], BLACK);
            }
        }
        else {
            for (int i = 0; i < NUM_BOXES; i++){
                draw_box(prev_second_x[i], prev_second_y[i], BOX_LEN, BLACK);
                draw_line(prev_second_x[i], prev_second_y[i], prev_second_x[(i+1) % NUM_BOXES], prev_second_y[(i+1) % NUM_BOXES], BLACK);
            }
        }*/
        
        // code for drawing the boxes
        
        for (int i = 0; i < (rows/size); ++i){
            for (int j = 0; j < (cols/size); ++j){
                BoxInfo element = boxes[i][j];
                draw_box(element.x_pos, element.y_pos, size, element.colour);
            }
        }

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        
        
    }
}

void draw_box(int x, int y, int size, short int color){
    
    for (int i = 0; i < size; ++i){
        for (int j = 0; j < size; ++j){
            plot_pixel(x + i, y + j, color);
        }
    }
}

// using Bresenham's algorithm
void draw_line(int x0, int y0, int x1, int y1, short int color){
    // calculate slope first --> if abs(slope) < 1 then flat --> iterate through x coordinates
    // if abs(slope) > 1 --> swap x and y
    // also consider if the first ones are further along x / y
    
    int is_steep = abs(y1 - y0) > abs(x1 - x0);
    
    // if steep then traverse y
    if (is_steep){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    
    // if x0 > x1 , swap coordinates
    if (x0 > x1){
        swap (&x0, &x1);
        swap (&y0, &y1);
    }
    
    int delta_x = x1 - x0;
    int delta_y = abs(y1 - y0);
    int error = -(delta_x/2);
    
    int y = y0;
    int y_step;
    
    if (y0 < y1)
        y_step = 1;
    else
        y_step = -1;
    
    // iterating through x (independent) of coordinates of pixels
    for (int x = x0; x < x1; ++x){
        if (is_steep)
            plot_pixel(y, x, color);
        else
            plot_pixel(x, y, color);
        
        error += delta_y;
        
        //to determine wheter or not to increment y
        if (error >= 0){
            y += y_step;
            error -= delta_x;
        }
    }
}

//swaps pointers of x and y
void swap (int* x, int* y){
    int temp = *x;
    *x = *y;
    *y = temp;
}

//iterate through all pixels and make them black
void clear_screen (){
    for (int x = 0; x < RESOLUTION_X; ++x){
        for (int y = 0; y < RESOLUTION_Y; ++y){
            plot_pixel(x, y, BLACK);
        }
    }
}

// draws the pixel on the screen
void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void wait_for_vsync(){
    volatile int* pixel_ctrl_ptr = (int*)PIXEL_BUF_CTRL_BASE;
    int status;
    
    // launch the swap process
    *pixel_ctrl_ptr = 1; // sets 'S' bit to 1
    
    // poll status bit
    status = *(pixel_ctrl_ptr + 3);
    
    // reading the status register (polling loop)
    while ((status & 1) != 0){
        status = *(pixel_ctrl_ptr + 3); // 0xFF20302C
        
    }
    
    // status bit S = 0, so can exit
    
}

