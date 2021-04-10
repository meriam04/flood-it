#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define PS2_BASE                0xFF200100
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050

/* function prototypes */

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */
  
  volatile int * PS2_ptr = (int *)PS2_BASE;
  int PS2_data, RVALID;
  char byte1 = 0, byte2 = 0, byte3 = 0;
  int x_cursor, y_cursor;
  
  // PS/2 mouse needs to be reset (must be already plugged in)
  *(PS2_ptr) = 0xFF; // reset
  
  while (1) {
    PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
    RVALID = PS2_data & 0x8000; // extract the RVALID field
    
    if (RVALID) {
      /* shift the next data byte into the display */
      byte1 = byte2;
      byte2 = byte3;
      byte3 = PS2_data & 0xFF;
     
      x_cursor += byte2;
      y_cursor += byte3;
      if (byte1 & 1)	//left button press
    	  //subroutine here
      
      if ((byte2 == (char)0xAA) && (byte3 == (char)0x00))
        // mouse inserted; initialize sending of data
        *(PS2_ptr) = 0xF4;
    }
  }
}
