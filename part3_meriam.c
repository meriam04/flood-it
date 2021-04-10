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
#define BOX_LEN 40
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
    int row;
    int col;
    int flood;
    int visited;
    short int colour;
} CellInfo;


// function definitions
void clear_screen();
void swap(int* x, int* y);
void draw_line(int x1, int y1, int x2, int y2, short int color);
void plot_pixel(int x, int y, short int line_color);
void wait_for_vsync();
void draw_box(int x, int y, int size, short int color);
// CellInfo* get_neighbours(CellInfo** board, CellInfo* box);
void apply_colour(short int colour);
void flood_cell(short int colour, CellInfo* cell);


//declare 2D array of structs (global)
CellInfo **board;
int rows;
int cols;
int size;

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    // int N = NUM_BOXES;
    rows = RESOLUTION_X;
    cols = RESOLUTION_Y;
    size = BOX_LEN;
    
    // declare other variables(not shown)
    short int colours[] = {YELLOW, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE, WHITE, RED};
    
    // allocate space for the 2D array (board)
    board = (CellInfo**)malloc(sizeof(CellInfo*)*(rows/size));
    for (int i = 0; i < (rows/size); ++i){
         board[i] = (CellInfo*)malloc(sizeof(CellInfo) * (cols/size));
    }
    
    // initialize 2D array of structs for the boxes
    for (int i = 0; i < (rows/size); ++i){
        for (int j = 0; j < (cols/size); ++j){
            // create element in array with necessary information
            CellInfo element;
            element.row = i;
            element.col = j;
            element.x_pos = i*size;
            element.y_pos = j*size;
            // if border, colour should be black, otherwise should be random
            if (i == 0 || j == 0 || i == (rows/size - 1) || j == (cols/size - 1)){
                element.colour = BLACK;
            }
            else {
                element.colour = colours[rand() % NUM_COLOURS];
                // element.colour = colours[j % NUM_COLOURS];
            }
            element.flood = FALSE;
            element.visited = FALSE;
            
            // put element into array
             board[i][j] = element;
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
    //*(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    //pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    
    clear_screen();
    int iteration = 0;
    printf("iteration: %d\n",iteration);
    
    // code for drawing the boxes
    for (int i = 0; i < (rows/size); ++i){
        for (int j = 0; j < (cols/size); ++j){
            CellInfo element =  board[i][j];
            draw_box(element.x_pos, element.y_pos, size, element.colour);
        }
    }

    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        
        short int colour = colours[iteration % NUM_COLOURS];
       
        // flood_cell(BLUE, &board[1][1]);
        /*if (iteration == 0){
            apply_colour(BLUE);
        }
        else if (iteration == 1){
            apply_colour(CYAN);
        }
        else {
            apply_colour(MAGENTA);
        }*/
        apply_colour(colour);
        iteration++;
        //printf("iteration: %d, colour: %x\n",iteration, colour);
        
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        
        // pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        
        
    }
}

// function that returns all neighbouring cells
/*CellInfo* get_neighbours(CellInfo** board, CellInfo cell){
    // iterate through all possible directions for neighbours
    // initialize to maximum number of neighbours and initialize with null
    CellInfo* neighbours[4] = {NULL, NULL, NULL, NULL};
    
    int arrayIndex = 0;
    for (int i = -1 ; i <= 1; i++){
        for (int j = -1; j <=1; j++){
            // if the direction is not diagonal and it is in bounds
           if ((i == 0 || j == 0) &&
               (i + cell.row > 0) && (i + cell.row < rows/size) &&
               (j + cell.col > 0) && (j + cell.col < cols/size)){
               
               // add to array the neighbour with this info
               CellInfo* neighbour;
               neighbour->row = cell.row + i;
               neighbour->col = cell.col + j;

               
               arrayIndex++;

               
           }
        }
    }
}
*/
void apply_colour(short int colour){
    // reinitialize board to have no visited nodes
    for (int i = 0; i < (rows/size); ++i){
        for (int j = 0; j < (cols/size); ++j){
            board[i][j].visited = FALSE;
        }
    }
    // call flood_cell on first element of board
    flood_cell(colour, &board[1][1]);

}
void flood_cell(short int colour, CellInfo* cell){
    cell->visited = TRUE;
    cell->colour = colour;
    cell->flood = TRUE;
    printf("flooding cell: %d, %d\n" , cell->row, cell->col);
    wait_for_vsync();
    draw_box(cell->x_pos, cell->y_pos, size, colour);
    // iterating through all neighbouring cells
    for (int i = -1 ; i <= 1; i++){
        for (int j = -1; j <=1; j++){
            // if the direction is not diagonal and it is in bounds
           if ((i == 0 || j == 0) && !((i == 0) && (j == 0)) &&
               (i + cell->row > 0) && (i + cell->row < rows/size) &&
               (j + cell->col > 0) && (j + cell->col < cols/size)){
               
               CellInfo* neighbour = &board[i + cell->row][j + cell->col];
               
               if (!(neighbour->visited) && (neighbour->colour == colour || neighbour->flood == TRUE)){
                   // flooding for each of the neighbours
                   printf("neighbour cell: %d, %d\n" , neighbour->row, neighbour->col);
                   flood_cell(colour, neighbour);
                   
               }
           }
        }
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

