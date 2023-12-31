
#define     IRQ_MODE          0b10010
#define     SVC_MODE          0b10011

#define     INT_ENABLE        0b01000000
#define     INT_DISABLE       0b11000000
#define     ENABLE            0x1
#define     KEYS_IRQ          73    //or 0x49
#define     PS2_IRQ           79    //dual is 89

#define     MPCORE_GIC_DIST    0xFFFED000 
#define     MPCORE_GIC_CPUIF    0xFFFEC100
#define       ICCICR          0x0           //offset of reg from base address of cpu interface
#define       ICCPMR          0x04
#define     ICCEOIR           0x10
#define     ICCIAR            0x0C
#define       ICDDCR          0x0

#define   A9_ONCHIP_END       0xC803FFFF    //i think?

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define PS2_BASE                0xFF200100
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030


void set_A9_IRQ_stack(void);
void config_GIC(void);

void config_KEYs(void);
void enable_A9_interrupts(void);

void __attribute__((interrupt)) __cs3_isr_irq(void);
void __attribute__((interrupt)) __cs3_reset(void);      //where are these called?
void __attribute__((interrupt)) __cs3_isr_undef(void);
void __attribute__((interrupt)) __cs3_isr_swi(void);
void __attribute__((interrupt)) __cs3_isr_pabort(void);
void __attribute__((interrupt)) __cs3_isr_dabort(void);
void __attribute__((interrupt)) __cs3_isr_fiq(void);

void pushbutton_ISR(void);

//put our own ISR variables here
/* key_dir and pattern are written by interrupt service routines; we have to
* declare these as volatile to avoid the compiler caching their values in
* registers */

volatile int key_dir = 0;
//volatile int pattern = 0x0F0F0F0F; // pattern for LED lights
//extern volatile int key_dir;

int main(void)
{
    //volatile int * HPS_GPIO1_ptr = (int *)HPS_GPIO1_BASE; // GPIO1 base address
    //volatile int HPS_timer_LEDG = 0x01000000; // value to turn on the HPS green light LEDG
    
    set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
    config_GIC(); // configure the general interrupt controller

    // enable interrupts on external devices
    config_KEYs(); // configure pushbutton KEYs to generate interrupts
    
    enable_A9_interrupts(); // enable interrupts on proc
    
    while (1) {   //program to loop while waiting for interrupts
           //or nothing
        
    }
}

/* setup the KEY interrupts in the FPGA */
void config_KEYs()
{
    volatile int * KEY_ptr = (int *)KEY_BASE; // pushbutton KEY address
    *(KEY_ptr + 2) = 0x3; // enable interrupts for KEY[1]
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

/* Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void)
{
    int address; // used to calculate register addresses
    /* configure the HPS timer interrupt */
    *((int *)0xFFFED8C4) = 0x01000000;
    *((int *)0xFFFED118) = 0x00000080;
    
    /* configure the FPGA interval timer and KEYs interrupts */
    *((int *)0xFFFED848) = 0x00000101;
    *((int *)0xFFFED108) = 0x00000300;
    
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


//IRQ EXCEPTION HANDLER
// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void)
{
    // Read the ICCIAR from the processor interface
    int address = MPCORE_GIC_CPUIF + ICCIAR;
    int int_ID = *((int *)address);
    
    if (int_ID == KEYS_IRQ) // check if interrupt is from the KEYS
        pushbutton_ISR();
    /*else if (int_ID ==INTERVAL_TIMER_IRQ) // check if interrupt is from the Altera timer SUB WITH SOMETHING ELSE ie hex interrupt ID
        interval_timer_ISR();*/
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
	volatile int * KEY_ptr = (int *)KEY_BASE;
	volatile int * LEDR_ptr= (int*)LEDR_BASE;
    int press;
    press = *(KEY_ptr + 3); // read the pushbutton interrupt register
    *(KEY_ptr + 3) = press; // Clear the interrupt
    
     *(LEDR_ptr)=key_dir;
    key_dir ^= 1; // Toggle key_dir value
    return;
}