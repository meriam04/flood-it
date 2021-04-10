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
#define PS2_BASE              0xFF200100
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
void apply_colour(short int colour);
void flood_cell(short int colour, CellInfo* cell);
short int colour_from_pos(int x_pos, int y_pos);
int check_won_game();
void display_turns_on_hex(int num_turns);
// int seven_segment_numbers(int number);
void display_hex(char b1, char b2, char b3);


// globals
CellInfo **board;
int rows;
int cols;
int size;
int x_cursor = 0;
int y_cursor = 0;

int main(void)
{
    volatile int * PS2_ptr = (int *)PS2_BASE;
    int PS2_data, RVALID;
    char byte1 = 0, byte2 = 0, byte3 = 0;
    
    int num_turns = 25;
    int won_game = FALSE;
  
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
    /*int iteration = 0;
    printf("iteration: %d\n",iteration);
    */
    // code for drawing the boxes
    for (int i = 0; i < (rows/size); ++i){
        for (int j = 0; j < (cols/size); ++j){
            CellInfo element =  board[i][j];
            draw_box(element.x_pos, element.y_pos, size, element.colour);
        }
    }
    
    // PS/2 mouse needs to be reset (must be already plugged in)
     *(PS2_ptr) = 0xFF; // reset
    
    while ( (!won_game) || (num_turns >= 0) )
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        // bootleg code for translating x-y to colour
        int x_pos = x_cursor % RESOLUTION_X;
        int y_pos = y_cursor % RESOLUTION_Y;
        /*
        int row = x_pos / BOX_LEN;
        int col = y_pos / BOX_LEN;
        
        int erase_colour = board[row][col].colour;*/
        short int erase_colour = colour_from_pos(x_pos, y_pos);
    
        draw_box(x_pos, y_pos, 3, erase_colour);
        
        display_hex(0x0,0x0, num_turns);
        // short int colour = colours[iteration % NUM_COLOURS];
        
        PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
        RVALID = PS2_data & 0x8000; // extract the RVALID field

        if (RVALID) {
            /* shift the next data byte into the display */
            byte1 = byte2;
            byte2 = byte3;
            byte3 = PS2_data & 0xFF;

            x_cursor += byte2;
            y_cursor += byte3;
            
            if (byte1 == 9){    //left button press
                //subroutine here
                short int clicked_colour = colour_from_pos(x_cursor % RESOLUTION_X, y_cursor % RESOLUTION_Y);
                // change colour to selected colour
                if ((clicked_colour != BLACK) && clicked_colour != board[1][1].colour){
                    apply_colour(clicked_colour);
                    num_turns--;
                }
				// iteration++;
				printf("xpos: %d, ypos: %d\n",x_cursor, y_cursor);
	  		}
            if ((byte2 == (char)0xAA) && (byte3 == (char)0x00)){
                // mouse inserted; initialize sending of data
                *(PS2_ptr) = 0xF4;
            }
        }
        
        // check if won
        printf("going in check won game -- won_game = %d\n", won_game);
        won_game = check_won_game();
        
        
        printf("out of check won game -- won_game = %d\n", won_game);
        if (won_game){
            printf("won the game i guess\n");
        }
        
        draw_box(x_cursor % RESOLUTION_X, y_cursor % RESOLUTION_Y, 3, WHITE);
        //printf("iteration: %d, colour: %x\n",iteration, colour);
        
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        
        // pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        
        
	}
    
    // exited while loop means either won or lost game
    if (won_game){
        // display win message
        printf("won game\n");
    }
    else{
        // display lost message
        printf("lost game :(\n");
    }
}

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

short int colour_from_pos(int x_pos, int y_pos){
   
    int row = x_pos / BOX_LEN;
    int col = y_pos / BOX_LEN;
    
    int return_colour = board[row][col].colour;
    return return_colour;
}

int check_won_game(){
    // iterate through game board and check if all same colour
    short int board_colour = board[1][1].colour;
    
    for (int i = 1; i < (rows/size - 1); ++i){
        for (int j = 1; j < (cols/size - 1); ++j){
            // if not same colour, did not win game
            if (board[i][j].colour != board_colour){
                return FALSE;
                break;
            }
        }
    }
    
    return TRUE;
}

void display_turns_on_hex(int num_turns){
    // first isolate the number of turns into different digits
    // num_turns should be a 2-digit number
    
    int ones_digit = num_turns % 10;
    int tens_digit = (num_turns - ones_digit)/10;
    
    // want to display on the hex so need HEX0-1 addresses
    
    /* come back to this when we have seven segment display function */
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

/**************************************************************************************
* Subroutine to show a string of HEX data on the HEX displays
****************************************************************************************/
void display_hex(char b1, char b2, char b3) {
    volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
    
    /* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
     * a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
     */
    
    unsigned char seven_seg_decode_table[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
        0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int shift_buffer, nibble;
    unsigned char code;
    int i;
    
    shift_buffer = (b1 << 16) | (b2 << 8) | b3;
    
    for (i = 0; i < 6; ++i) {
        nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
        code = seven_seg_decode_table[nibble];
        hex_segs[i] = code;
        shift_buffer = shift_buffer >> 4;
   }
  
    /* drive the hex displays */

    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}
