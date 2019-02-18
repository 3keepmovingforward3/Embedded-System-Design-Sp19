/*
 * interrupt_counter_tut_2B.c
 *
 *  Created on:     2/20/2019
 *      Author:     Benjamin Blouin
 *     Version:        1.0
 */

/********************************************************************************************
 
 * VERSION HISTORY
 ********************************************************************************************
 *     v1.0 - 2/17/2019
 *         Working
 *    v0.8 - 2/15/2019
 *        First version created.
 *******************************************************************************************/

#include "xparameters.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "sleep.h"

// Parameter definitions
#define INTC_DEVICE_ID             XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID            XPAR_TMRCTR_0_DEVICE_ID
#define BTNS_DEVICE_ID            XPAR_AXI_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID            XPAR_AXI_GPIO_1_DEVICE_ID
#define SWTS_DEVICE_ID            XPAR_AXI_GPIO_2_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID     XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define INTC_TMR_INTERRUPT_ID     XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR
#define BTN_INT                 XGPIO_IR_CH1_MASK
#define TMR_LOAD                0xFE000000  //1sec

XGpio LEDInst, BTNInst, SWTInst;
XScuGic INTCInst;
XTmrCtr TMRInst;
static int led_data;
static int btn_value;
static int tmr_count;
static int swi_value;
volatile int countChange_tmr=3;
volatile signed int sign=1;
volatile signed int signTwo=1;
volatile int sentinel = 0;

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
static void BTN_Intr_Handler(void *baseaddr_p);
static void TMR_Intr_Handler(void *baseaddr_p,void *baseaddr_p2);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr);
static inline void GPIOInit();
static inline void SetupTimer();
static inline void InitializeInterruptController();
static void resetErrorStates();


//----------------------------------------------------
// INTERRUPT HANDLER FUNCTIONS
// - called by the timer, button interrupt, performs
// - LED flashing
//----------------------------------------------------


void BTN_Intr_Handler(void *InstancePtr)
{
    // Disable GPIO interrupts
    XGpio_InterruptDisable(&BTNInst, BTN_INT);
    // Ignore additional button presses
    if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) != BTN_INT) {return;}
    // Button read
    btn_value = XGpio_DiscreteRead(&BTNInst, 1);
    // Reassign for SW[2] case
    if(btn_value == 8 && swi_value == 4){swi_value=2;btn_value=2;}
    if(btn_value == 4 && swi_value == 4){swi_value=1;btn_value=1;}
    
    
    if(swi_value==0){} // Do nothing if switches are all off
    else{
        // Keeps timer max at 7
        if((btn_value==swi_value)||(swi_value == 4)){
            if((sign==1) && (countChange_tmr==7)){countChange_tmr = 7;}
            // Keeps timer minimum at 0
            else if((sign==-1) && (countChange_tmr==0)){countChange_tmr=0;}
            // Do nothing if switch-button combo mismatch
            else if(swi_value!=btn_value){}
            // Update value timer counts to, with ascend/descend controlled by sign value
            else{countChange_tmr = (countChange_tmr + sign);}
        }
    }
    
    // LED output sanity check
    if((swi_value!=0)&&(swi_value==btn_value)){
        // Gives ~1sec LED output for current value timer counts to
        XTmrCtr_Stop(&TMRInst,0);
        XGpio_DiscreteWrite(&LEDInst, 1, countChange_tmr);
        /* Global Timer is always clocked at half of the CPU frequency */
        usleep(500000);
        XTmrCtr_Reset(&TMRInst,0);
        XTmrCtr_Start(&TMRInst,0);
    }
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

void TMR_Intr_Handler(void *data,void *InstancePtr)
{
    
    // Checks if the specified timer counter of the device has expired. In capture
    // mode, expired is defined as a capture occurred. In compare mode, expired is
    // defined as the timer counter rolled over/under for up/down counting.
    
    // When interrupts are enabled, the expiration causes an interrupt. This function
    // is typically used to poll a timer counter to determine when it has expired.
    
    if (XTmrCtr_IsExpired(&TMRInst,0)){
        // Once timer has expired countChange_tmr times, stop, increment counter
        // reset timer and start running again
        // tmr_count default=3, reset sets tmr_count=3
        if(tmr_count == countChange_tmr){
            XTmrCtr_Stop(&TMRInst,0);
            tmr_count = 0;
            led_data++;
            XGpio_DiscreteWrite(&LEDInst, 1, led_data);
            
        }
        else if(tmr_count > 7){tmr_count=0;}else{tmr_count++;} // Catch for infinite zero loop
        
        
    }
    // Given as fix by Lab TAs
    XTmrCtr_Reset(&TMRInst,0);
    XTmrCtr_Start(&TMRInst,0);
}


//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
    GPIOInit();
    SetupTimer();
    InitializeInterruptController();
    
    XTmrCtr_Start(&TMRInst, 0);
    
    while(1){
        swi_value = XGpio_DiscreteRead(&SWTInst,1);
        if((swi_value>>3)==1){resetErrorStates();} // Right shifting so only SW[3,1,0] is tested
        else if ((swi_value>>1)==1){sign = -1;}    // SW[2] is not checked here as its behavior is
        else if(swi_value==1){sign = 1;}            // Same as SW[1,0] excepted buttons are shifted left one
    };                                            // Behavior is respective to position like SW[1,0]
    return 0;
}

//----------------------------------------------------
// INITIAL SETUP FUNCTIONS
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
    // Enable interrupt
    // The global interrupt must also be enabled by calling
    // XGpio_InterruptGlobalEnable() for interrupts to occur. This function will
    // assert if the hardware device has not been built with interrupt capabilities.
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
    
    // Enable the interrupt output signal. Interrupts enabled through
    // XGpio_InterruptEnable() will not be passed through until the global enable
    // bit is set by this function. This function is designed to allow all
    // interrupts (both channels) to be enabled easily for exiting a critical
    // section. This function will assert if the hardware device has not been
    // built with interrupt capabilities
    XGpio_InterruptGlobalEnable(&BTNInst);
    
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 XScuGicInstancePtr);
    Xil_ExceptionEnable();
    return XST_SUCCESS;
}

int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr)
{
    XScuGic_Config *IntcConfig;
    int status;
    
    // Interrupt controller initialization
    IntcConfig = XScuGic_LookupConfig(DeviceId);
    status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    // Call to interrupt setup
    status = InterruptSystemSetup(&INTCInst);
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    // Connect GPIO interrupt to handler
    status = XScuGic_Connect(&INTCInst,
                             INTC_GPIO_INTERRUPT_ID,
                             (Xil_ExceptionHandler)BTN_Intr_Handler,
                             (void *)GpioInstancePtr);
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    // Connect timer interrupt to handler
    status = XScuGic_Connect(&INTCInst,
                             INTC_TMR_INTERRUPT_ID,
                             (Xil_ExceptionHandler)TMR_Intr_Handler,
                             (void *)TmrInstancePtr);
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    // Enable GPIO interrupts interrupt
    XGpio_InterruptEnable(GpioInstancePtr, 1);
    XGpio_InterruptGlobalEnable(GpioInstancePtr);
    
    // Enable GPIO and timer interrupts in the controller
    XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
    XScuGic_Enable(&INTCInst, INTC_TMR_INTERRUPT_ID);
    return XST_SUCCESS;
}

void GPIOInit(){
    //----------------------------------------------------
    // INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
    //----------------------------------------------------
    // Initialize LEDs
    XGpio_Initialize(&LEDInst, LEDS_DEVICE_ID);
    // Initialize Push Buttons
    XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
    // Initialize Push Buttons
    XGpio_Initialize(&SWTInst, SWTS_DEVICE_ID);
    // Set LEDs direction to outputs
    XGpio_SetDataDirection(&LEDInst, 1, 0x0);
    // Set Switches direction to outputs
    XGpio_SetDataDirection(&SWTInst, 1, 0xF);
    // Set all buttons direction to inputs
    XGpio_SetDataDirection(&BTNInst, 1, 0xF);
}

void SetupTimer(){
    //----------------------------------------------------
    // SETUP THE TIMER
    //----------------------------------------------------
    
    //Initializes a specific timer/counter instance/driver. Initialize fields of
    // the XTmrCtr structure, then reset the timer/counter.If a timer is already
    // running then it is not initialized.
    XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
    
    // Sets the timer callback function, which the driver calls when the specified
    // timer times out.
    XTmrCtr_SetHandler(&TMRInst, (void *)TMR_Intr_Handler, &TMRInst);
    
    // Set the reset value for the specified timer counter. This is the value
    // that is loaded into the timer counter when it is reset. This value is also
    // loaded when the timer counter is started.
    XTmrCtr_SetResetValue(&TMRInst, 0, TMR_LOAD);
    
    // Enables the specified options for the specified timer counter. This function
    // sets the options without regard to the current options of the driver. To
    // prevent a loss of the current options, the user should call
    // XTmrCtr_GetOptions() prior to this function and modify the retrieved options
    // to pass into this function to prevent loss of the current options.
    XTmrCtr_SetOptions(&TMRInst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
    
}

void InitializeInterruptController(){
    // Initialize interrupt controller
    IntcInitFunction(INTC_DEVICE_ID, &TMRInst, &BTNInst);
}

void resetErrorStates(){
    // Reset happens when SW[3] is high and no other input
    // Error state happens when SW[3] is high and any other input
    XTmrCtr_Stop(&TMRInst,0); // timer off for reset
    XGpio_InterruptDisable(&BTNInst, BTN_INT); // interrupt off disables ISR
    countChange_tmr = 3; // reset what timer counts up to per reset state requirement
    tmr_count=0; // reset timer count
    led_data=0; // explicit led data reset
    btn_value=0; //explicit button value reset
    XGpio_DiscreteWrite(&LEDInst,LEDS_DEVICE_ID,0x0); // clear LEDs for reset state
    
    // Checks for error state
    if(XGpio_DiscreteRead(&SWTInst,1)!=8||XGpio_DiscreteRead(&BTNInst,1)!=0){
        XGpio_DiscreteWrite(&LEDInst,LEDS_DEVICE_ID,0x0);
        sleep(1);
        XGpio_DiscreteWrite(&LEDInst,LEDS_DEVICE_ID,0xF);
        sleep(1);
    }
    
    // Two seconds gives user enough time to remove error state
    // Therefore, we can initialize/start the timer, and reinitialize buttons
    XTmrCtr_Reset(&TMRInst,0);
    XTmrCtr_Start(&TMRInst,0);
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
    
    // Leaving reset error state returns to infinite while loop that
    // happens in the PS, which is fast enough to check switches again
    // in case reset/error state happens again
    
}

