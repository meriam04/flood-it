/* This files provides address values that exist in the system */
#define IRQ_MODE              0b10010
#define SVC_MODE              0b10011

#define INT_ENABLE            0b01000000
#define INT_DISABLE           0b11000000
#define ENABLE                0x1
#define KEYS_IRQ              73    //or 0x49
#define PS2_IRQ               79    //dual is 89

#define MPCORE_GIC_DIST       0xFFFED000
#define MPCORE_GIC_CPUIF      0xFFFEC100
#define ICCICR                0x0           //offset of reg from base address of cpu interface
#define ICCPMR                0x04
#define ICCEOIR               0x10
#define ICCIAR                0x0C
#define ICDDCR                0x0

#define A9_ONCHIP_END         0xC803FFFF    //i think?
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
#define OUTLINE 2

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

typedef struct {
    int row;
    int col;
} SelectedCell;


// function definitions
void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_interrupt(int N, int CPU_target);
void config_KEYs(void);
void config_PS2();
void enable_A9_interrupts(void);
//void disable_A9_interrupts(void);    //if use, disable then enable interrupts (prob unneccesary)

void __attribute__((interrupt)) __cs3_isr_irq(void);
void __attribute__((interrupt)) __cs3_reset(void);      //where are these called?
void __attribute__((interrupt)) __cs3_isr_undef(void);
void __attribute__((interrupt)) __cs3_isr_swi(void);
void __attribute__((interrupt)) __cs3_isr_pabort(void);
void __attribute__((interrupt)) __cs3_isr_dabort(void);
void __attribute__((interrupt)) __cs3_isr_fiq(void);

void pushbutton_ISR(void);
void keyboard_ISR(void);

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
void set_level(int level);
void display_hex(char b1, char b2, char b3);
void hex_to_dec(char* b1);
void display_win_lose_hex(int won_game);
void initialize_board();
void free_board();
void draw_board();
void reinitialize_board();
void erase_selected_cell();
void draw_selected_cell();

void draw_menu();

void wait();




// globals
CellInfo **board;
int rows;
int cols;
int size = BOX_LEN; // default length
int colour_num = NUM_COLOURS; // default colours
int x_cursor = 0;
int y_cursor = 0;
short int colours[] = {YELLOW, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE, WHITE, RED};
int num_turns = 25;
int won_game = FALSE;

SelectedCell selected_cell; // making the selected cell a global variable

/* key_dir and pattern are written by interrupt service routines; we have to
* declare these as volatile to avoid the compiler caching their values in
* registers */

volatile int key_dir = 0;
volatile int x_position = 0;
volatile int y_position = 0;

int main(void)
{
    set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
    config_GIC(); // configure the general interrupt controller

    // enable interrupts on external devices
    config_KEYs(); // configure pushbutton KEYs to generate interrupts
    config_PS2();
    enable_A9_interrupts(); // enable interrupts on proc
   
    //volatile int * PS2_ptr = (int *)PS2_BASE;
   //*(PS2_ptr) = 0xFF;    //reset the mouse
    
    
    
    //int num_turns = 25;
    //int won_game = FALSE;
  
    // determine level here using the keys
    // just using default level for now
    set_level(2);
    
    rows = RESOLUTION_X/size;
    cols = RESOLUTION_Y/size;
    
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

   
    initialize_board();

    
    // ensure that neighbours of first of same colour are part of flood already
    // apply_colour(board[1][1].colour);

    
    // initializing selected cell to be the bottom right
    selected_cell.row = rows - 2;
    selected_cell.col = cols - 2;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; // first store the address in the
                                        // back buffer
    
    /*now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    
    initialize_board();
    apply_colour(board[1][1].colour);
    
    /* set back pixel buffer to start of SDRAM memory */
    //*(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    //pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    
    /*clear_screen();
    int iteration = 0;
    printf("iteration: %d\n",iteration);
    */
    //draw_menu();
    //wait for click and then
    draw_board();
    draw_selected_cell();
    display_hex(0,0, num_turns);
    
    draw_box(x_cursor % RESOLUTION_X, y_cursor % RESOLUTION_Y, 3, WHITE);
    
    // while ( !won_game && (num_turns > 0))
    while (1)
    {
       // does nothing as it waits for interrupts
    }
    
    
    // free the board here probably
    free_board();
    
}

/*void draw_menu(){
    //iterate through colors for every cell, plotpixel
    //plot_pixel(int x, int y, short int line_color); 4 bytes per pixel?
    char menu_colours[]={};
    int i=0;//row
    int j=0;//col
    int l=0;//row
    for(int k=0;k<76241;k++){
        if (i>319){
            j++;
            i=0;
        }
        if(j>239)
            break;
        plot_pixel(i, j, menu_colour[k]);
        i++;
}*/

// allocates space for board
void initialize_board(){
    
    rows = RESOLUTION_X/size;
    cols = RESOLUTION_Y/size;
    
    // allocate space for the 2D array (board)
    board = (CellInfo**)malloc(sizeof(CellInfo*)*(rows));
    for (int i = 0; i < (rows); ++i){
         board[i] = (CellInfo*)malloc(sizeof(CellInfo) * (cols));
    }
    
    // initialize 2D array of structs for the boxes
    for (int i = 0; i < (rows); ++i){
        for (int j = 0; j < (cols); ++j){
            // create element in array with necessary information
            CellInfo element;
            element.row = i;
            element.col = j;
            element.x_pos = i*size;
            element.y_pos = j*size;
            // if border, colour should be black, otherwise should be random
            if (i == 0 || j == 0 || i == (rows - 1) || j == (cols - 1)){
                element.colour = BLACK;
            }
            else {
                element.colour = colours[rand() % colour_num];
                // element.colour = colours[j % NUM_COLOURS];
            }
            element.flood = FALSE;
            element.visited = FALSE;
            
            // put element into array
             board[i][j] = element;
        }
    }
    
    // apply flood to first element
    // short int flood_colour = board[1][1].colour;
    // apply_colour(flood_colour);
    
}
// free the board
void free_board(){
    
    rows = RESOLUTION_X/size;
    
    for (int i = 0; i < rows; i++){
        free(board[i]);
    }
    free(board);
}

void draw_board(){
    
    rows = RESOLUTION_X/size;
    cols = RESOLUTION_Y/size;
    
    // code for drawing the boxes
    for (int i = 0; i < (rows); ++i){
        for (int j = 0; j < (cols); ++j){
            CellInfo element =  board[i][j];
            draw_box(element.x_pos, element.y_pos, size, element.colour);
        }
    }
}

void reinitialize_board(){
    // free_board();
    num_turns = 25; // should change this depending on selected level
    won_game = FALSE;
    initialize_board();
    
    // ensure that neighbours of first one are also considered flooded initially
    apply_colour(board[1][1].colour);
    draw_board();
    
    // reinitialize selected cell to be bottom right
    selected_cell.row = rows - 2;
    selected_cell.col = cols - 2;
    draw_selected_cell();
}

void apply_colour(short int colour){
    // reinitialize board to have no visited nodes
    for (int i = 0; i < (rows); ++i){
        for (int j = 0; j < (cols); ++j){
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
    // printf("flooding cell: %d, %d\n" , cell->row, cell->col);
    wait_for_vsync(); // consider making this a shorter wait time
    draw_box(cell->x_pos, cell->y_pos, size, colour);
    // iterating through all neighbouring cells
    for (int i = -1 ; i <= 1; i++){
        for (int j = -1; j <=1; j++){
            // if the direction is not diagonal and it is in bounds
           if ((i == 0 || j == 0) && !((i == 0) && (j == 0)) &&
               (i + cell->row > 0) && (i + cell->row < rows) &&
               (j + cell->col > 0) && (j + cell->col < cols)){
               
               CellInfo* neighbour = &board[i + cell->row][j + cell->col];
               
               if (!(neighbour->visited) && (neighbour->colour == colour || neighbour->flood == TRUE)){
                   // flooding for each of the neighbours
                   // printf("neighbour cell: %d, %d\n" , neighbour->row, neighbour->col);
                   flood_cell(colour, neighbour);
                   
               }
           }
        }
    }
}

short int colour_from_pos(int x_pos, int y_pos){
    // make this more accurate later
    int row = x_pos / size;
    int col = y_pos / size;
    
    int return_colour = board[row][col].colour;
    return return_colour;
}

int check_won_game(){
    // iterate through game board and check if all same colour
    short int board_colour = board[1][1].colour;
    
    for (int i = 1; i < (rows - 1); ++i){
        for (int j = 1; j < (cols - 1); ++j){
            // if not same colour, did not win game
            if (board[i][j].colour != board_colour){
                return FALSE;
                break;
            }
        }
    }
    return TRUE;
}
// sets variables according to level selected
void set_level(int level){
    // set box size, number of colours
    
    // very beginner
    if (level == 0){
        size = 80;
        colour_num = 3;
    }
    // beginner
    else if (level == 1){
        size = 40;
        colour_num = 4;
    }
    // intermediate
    else if (level == 2){
        size = 20;
        colour_num = 5;
    }
    // hard
    else if (level == 3){
        size = 16;
        colour_num = 6;
    }
    // very hard
    else if (level == 4){
        size = 10;
        colour_num = 7;
    }
    else { // assume intermediate level
        size = 20;
        colour_num = 5;
        
    }
    return;
}

void draw_selected_cell(){
    // drawing selected box with a white outline of thickness 2
    CellInfo current_cell = board[selected_cell.row][selected_cell.col];
    // draw outline first
    draw_box(current_cell.x_pos, current_cell.y_pos, size, WHITE);
    // then draw actual box
    draw_box(current_cell.x_pos + OUTLINE, current_cell.y_pos + OUTLINE, size - 2*OUTLINE, current_cell.colour);
}

void erase_selected_cell(){
    CellInfo current_cell = board[selected_cell.row][selected_cell.col];
    draw_box(current_cell.x_pos, current_cell.y_pos, size, current_cell.colour);
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
    
    // convert values in shift buffer to decimal here
    hex_to_dec(&b1);
    hex_to_dec(&b2);
    hex_to_dec(&b3);
    
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

// converts hex number at address passed into decimal number
void hex_to_dec(char* num_turns){
    // first isolate the number of turns into different digits
    // num_turns should be a 2-digit number
    
    int ones_digit = (*num_turns) % 10;
    int tens_digit = ((*num_turns) - ones_digit)/10;
    
    int return_num = tens_digit << 4 | ones_digit;
    *num_turns = return_num;

}

// want to display message "passed"
void display_win_lose_hex(int won_game){
    volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
    
    // create an array with the letters for "passed"
    unsigned char passed_table[] = {
        0b0000000001110011 /*P*/, 0b0000000001110111  /*A*/, 0b0000000001101101 /*S*/,
        0b0000000001101101 /*S*/, 0b0000000001111001  /*E*/, 0b0000000001011110 /*d*/
    };
    
    unsigned char failed_table[] = {
        0b0000000001110001  /*F*/, 0b0000000001110111  /*A*/, 0b0000000000110000 /*I*/,
        0b0000000000111000 /*L*/, 0b0000000001111001  /*E*/, 0b0000000001011110 /*d*/
    };
    
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int shift_buffer, nibble;
    unsigned char code;
    int i;
    
    for (i = 0; i < 6; ++i) {
        if (won_game){
            code = passed_table[5 - i];
        }
        else{
            code = failed_table[5 - i];
        }
        hex_segs[i] = code;
   }
    // want to display the numbers in order
    /* drive the hex displays */

    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}







/**************************************************************************************************************** interrupt functions ************************************************************************************************************/


/* setup the KEY interrupts in the FPGA */
void config_KEYs()
{
    volatile int * KEY_ptr = (int *)KEY_BASE; // pushbutton KEY address
    *(KEY_ptr + 2) = 0x3; // enable interrupts for KEY[1]
}

void config_PS2()
{
    volatile int* PS2_ptr = (int*) PS2_BASE;
    *(PS2_ptr + 1) = 0x1;    //enable interrupts by setting RE bit =1
}

/*Initialize the banked stack pointer register for IRQ mode*/
void set_A9_IRQ_stack(void)
{
    int stack, mode;
    stack = A9_ONCHIP_END - 7; // top of A9 onchip memory, aligned to 8 bytes
    
    /* change processor to IRQ mode with interrupts disabled */
    mode = INT_DISABLE | IRQ_MODE;
    asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
    /* set banked stack pointer */
    asm("mov sp, %[ps]" : : [ps] "r"(stack));
    /* go back to SVC mode before executing subroutine return! */
    mode = INT_DISABLE | SVC_MODE;
    asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}

/*
* Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void)
{
    int status = SVC_MODE | INT_ENABLE;
    asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}

/*void disable_A9_interrupts(void) {
    int status = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}*/

/* Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void)
{
    int address; // used to calculate register addresses
    
    /* configure the FPGA PS2 and KEYs interrupts */
    //*((int *)0xFFFED848) = 0x00000100;    //to specify CPU target (ICDIPTRn) - one byte per interrupt
    //*((int *)0xFFFED84C) = 0x01000000;    //3rd byte for PS2: index=n%4=3 (has to be byte 0,1,2 or 3)
    //*((int *)0xFFFED108) = 0x00008200;    //for key AND ps2 set-enables (ICDISERn) - enable specific interrupts to be forwarded to the CPU interface - one BIT per interrupt
    config_interrupt(73,1);
    config_interrupt (79,1);
    
    // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
    //priorities
    address = MPCORE_GIC_CPUIF + ICCPMR;
    *((int *)address) = 0xFFFF;
    
    // Set CPU Interface Control Register (ICCICR). Enable signaling of
    // interrupts
    address = MPCORE_GIC_CPUIF + ICCICR;
    *((int *)address) = 1;//ENABLE
    
    // Configure the Distributor Control Register (ICDDCR) to send pending
    // interrupts to CPUs
    address = MPCORE_GIC_DIST + ICDDCR;
    *((int *)address) = 1;
}

void config_interrupt(int N, int CPU_target) {
    int reg_offset, index, value, address;
    /* Configure the Interrupt Set-Enable Registers (ICDISERn).
    reg_offset = (integer_div(N / 32) * 4
    value = 1 << (N mod 32) */
    reg_offset = (N >> 3) & 0xFFFFFFFC;
    index = N & 0x1F;
    value = 0x1 << index;
    address = 0xFFFED100 + reg_offset;
    // Now that we know the register address and value, set the appropriate bit */
    *(int*)address |= value;

    /* Configure the Interrupt Processor Targets Register (ICDIPTRn)
    * reg_offset = integer_div(N / 4) * 4
    * index = N mod 4 */
    reg_offset = (N & 0xFFFFFFFC);
    index = N & 0x3;
    address = 0xFFFED800 + reg_offset + index;
    /* Now that we know the register address and value, write to (only) the
    appropriate byte */
    *(char *)address = (char)CPU_target;
}
            
//IRQ EXCEPTION HANDLER
// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void)
{
    // Read the ICCIAR from the processor interface
    int address = MPCORE_GIC_CPUIF + ICCIAR;
    int int_ID = *((int *)address);
    
    if (int_ID == KEYS_IRQ) // check if interrupt is from the KEYS
        pushbutton_ISR();
    else if (int_ID == PS2_IRQ) // check if interrupt is from the PS2 port, add sw interrupt ID later
        keyboard_ISR();
    else
        while (1)
            ; // if unexpected, then stay here
    
    // Write to the End of Interrupt Register (ICCEOIR)
    address = MPCORE_GIC_CPUIF + ICCEOIR;
    *((int *)address) = int_ID;
    return;
}

// Define the remaining exception handlers (writes to
void __attribute__((interrupt)) __cs3_reset(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_undef(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_swi(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_pabort(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_dabort(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_fiq(void)
{
    while (1)
        ;
}
            
            

/***************************************************************************************
* Pushbutton - Interrupt Service Routine
*
* This routine toggles the key_dir variable from 0 <-> 1
****************************************************************************************/
void pushbutton_ISR(void)
{
    printf("in pushbutton interrupt\n");
    volatile int * KEY_ptr = (int *)KEY_BASE;
    volatile int * LEDR_ptr = (int*)LEDR_BASE;
    volatile int * SW_ptr = (int*)SW_BASE;
    
    int press;
    press = *(KEY_ptr + 3); // read the pushbutton interrupt register
    *(KEY_ptr + 3) = press; // Clear the interrupt
    printf("key %d was pressed\n",press-1);
    
    //if(press&0x1)//key0 is pressed
    //else if (press&0x2)//key1
    //else if (press&0x4)//key2
    //else//key3
    
    
    // printf("in interrupt\n");
    // change this part to read from switch and call the level setting function
    int switch_value = *(SW_ptr);
    // free board before changing size
    free_board();
    
    set_level(switch_value);
    reinitialize_board();
    
     //*(LEDR_ptr)=key_dir;
     //key_dir ^= 1; // Toggle key_dir value
    return;
}

void keyboard_ISR(void)    //interrupt triggered w ANY mvmt: clear it every time, and execute ONLY if click (byte1: 0000101, others are moot) (need to store all movement to see total x,y?)
{
    volatile int * PS2_ptr = (int *)PS2_BASE;
    volatile int * LEDR_ptr= (int*)LEDR_BASE;
    int PS2_data, PS2_control, RAVAIL, RVALID;
    unsigned char byte1 = 0, byte2 = 0;
    //have to read this first!!! first read below::::
    PS2_data = *(PS2_ptr);    //reading from ps2data (lower 8 bits are Data) automatically decrements ravail/num on stack=>next byte now in PS2_data
    RAVAIL = PS2_data & 0xFFFF0000;
    RVALID = PS2_data & 0x8000;
    
    printf("interruptedkey\n data %d\n",PS2_data);
    *(LEDR_ptr) = key_dir;
    key_dir ^=1;
    
    int flag = *(PS2_ptr +1) & 0x100;
    if (!flag)
        printf("interrupt flag cleared after oneread\n");
    
    if(RVALID && (RAVAIL==0)){ //RVALID means data is available, ravail=0 means data has stopped being sent (press is over)
        byte1 = PS2_data & 0xFF;
        
        if(byte1==(char)0xF0){//break code (key is done being pressed, read next byte -> break code sends 2 bytes
            
            PS2_data = *(PS2_ptr);/////second read (after check for break)
            RVALID = PS2_data & 0x8000;
            RAVAIL = PS2_data & 0xFFFF0000; //top 16 BITS!!!
            
            flag = *(PS2_ptr +1) & 0x100;
            if (!flag)
                printf("interrupt flag cleared after read 2\n char %x\n", byte1);
        
            if (RVALID && (RAVAIL==0)){
                byte2=PS2_data & 0xFF;
               
                if (byte2==(char)0x1C){
                    printf("move left\n");
                    // check if selected cell can move left first
                    // then decrememnt column if can
                    if (selected_cell.row > 1){ // check here not sure about this
                        erase_selected_cell();
                        selected_cell.row--;
                        draw_selected_cell();
                    }
                    return;
                    
                }
                else if (byte2==(char)0x23){
                    printf("move right\n");
                     
                    if (selected_cell.row < (rows - 2)){ // check here not sure about this
                        erase_selected_cell();
                        selected_cell.row++;
                        draw_selected_cell();
                    }
                    return;
                }
                else if (byte2==(char)0x1D){
                    printf("move up\n");
                    if (selected_cell.col > 1){
                        // redraw current selected cell first then draw new one too
                        erase_selected_cell();
                        selected_cell.col--;
                        draw_selected_cell();
                    }
                    return;
                }
                else if (byte2==(char)0x1B){
                    printf("move down\n");
                    if (selected_cell.col < (cols - 2)){
                        erase_selected_cell();
                        selected_cell.col++;
                        draw_selected_cell();
                    }
                    
                    return;
                    
                }
                else if (byte2==(char)0x5A){
                    printf("select box\n");
                    short int clicked_colour = board[selected_cell.row][selected_cell.col].colour;
                    // change colour to selected colour
                    if ((clicked_colour != BLACK) && clicked_colour != board[1][1].colour){
                        apply_colour(clicked_colour);
                        num_turns--;
                        display_hex(0,0, num_turns);
                        
                        // check if won game
                        won_game = check_won_game();
                        
                        if (won_game || num_turns <= 0){
                            // exited while loop means either won or lost game
                            display_win_lose_hex(won_game);
                            
                            // free the board here probably
                            // free_board();
                        }
                    }
                    // if selected cell was flooded, reinitialize it
                    if (board[selected_cell.row][selected_cell.col].flood){
                        selected_cell.row = rows - 2;
                        selected_cell.col = cols - 2;
                        draw_selected_cell();
                    }
                    return;
                }
                else {
                    printf("invalid key press\n");
                    return;
                }
            }
        }
    }
    
    flag = *(PS2_ptr +1) & 0x100;
    if (!flag)
        printf("interrupt flag cleared from none interrupt\n");
    
    /*do we need to detect edge of screen?
     if ((byte2 == (char)0xAA) && (byte3 == (char)0x00)){//chck if initialization of mouse is completed
        // mouse inserted; initialize sending of data //aka mouse hit left corner?
        *(PS2_ptr) = 0xF4;    //command to initialize data reporting, enables data reporting and resets its movement counters (need for edge of screen??)
    }*/
    //write to lower 8 bits of data reg to send commands => check CE to see if error recieving it
    
    
    return;
}


