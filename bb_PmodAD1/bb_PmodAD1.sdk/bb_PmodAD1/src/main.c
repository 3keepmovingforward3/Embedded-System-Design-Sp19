/*
 * Embedded Design Lab
 * PmodAD1 Vivado SDK Project
 * ECE3623-003
 * Dennis Silage, PhD
 * Benjamin Blouin
 * 2/4/2019
 *
 * The PmodAD1 analog-to-digital converter (ADC) SPI peripheral from the Digilent Vivado IP Library
 * is configured and used to perform measurements on analog test signals.
 *
 * The lab uses the Digilent Electronic Explorer Board (EE Board)
 * to generate an analog test signal interfaced to channel 1 of the PmodAD1 ADC.
 *
 *  SW0 OFF, SW1 OFF. The project prints ECE3622 Lab 2 on the SDK terminal every 5 seconds.
 *
 *  SW0 is ON, SW1 is OFF. The input voltage signal from the AWG is a unipolar ramp
 *  at 0.1 Hz (Waveforms indicates this as 100 mHz) with a range from 0 to 1 V.
 *  The LEDs are to be ON if the voltage input exceeds the thresholds listed for the four LEDs:
 *  LED0 0.2 V LED1 0.4 V LED2 0.6 V LED3 0.8V
 *  SDK terminal output is a single string that identifies the event.
 *
 *  SW0 is OFF, SW1 is ON. The input voltage signal from the AWG is again a unipolar ramp at 0.1 Hz with a range from 0 to 1 V.
 *  The calculation is output to SDK Terminal every 1 second is the running average value of the voltage signal input.
 *  The ramp and pulse unipolar signals are also to have symmetry differences
 *  to validate the running average calculation.
 *
 */

#include <stdio.h>
#include "PmodAD1.h"
#include "sleep.h"
#include "xgpio.h"
#include "xil_io.h"
#include <xil_types.h>
#include "xparameters.h"

// Macros
#define LEDS 1  //channel
#define SWITCHES 2  //channel
#define WINDOWSIZE 150 //moving average window size
#define OVERLAP 50	//Overlap of successive window

// Global Objects
XGpio LEDSWSInst; // GPIO Object for LEDS and Switches
PmodAD1 myDevice; // PmodAD1 Object for address
AD1_RawData RawData; //AD1 Object to hold RawData
AD1_PhysicalData PhysicalData; //AD1 Object to hold ActualData converted from RawData

// Global Variables
const float ReferenceVoltage = 3.3/2.0; //2.0 is vref offset calculated for calibration
float window[WINDOWSIZE]; //array to do moving averages
float avg = 0;
float sum = 0;
float sumOverlap = 0;
int t = 0;
int num = 0;

// Function prototypes
void ADInitialize();
void XGPIOInit();
void ADRun();

// Start main program
int main() {
    ADInitialize();
    XGPIOInit();
    ADRun();
}

// Functions

void XGPIOInit() {
    // One GPIO block with two channels
    XGpio_Initialize(&LEDSWSInst,  XPAR_AXI_GPIO_0_DEVICE_ID); // Pointer Init using devID

    XGpio_SetDataDirection(&LEDSWSInst,LEDS,0x00); //channel 1 output
    XGpio_SetDataDirection(&LEDSWSInst,SWITCHES,0xFF); //channel 2 input
}

void ADRun() {
    int t = XGpio_DiscreteRead(&LEDSWSInst,SWITCHES); //initial switch read
    sumOverlap = 0; // used to reset old window overlap so it doesn't influence
    				// the new average when behavioral states are changed
    while(1) {
    	// First case
        if(t == 0) {
            while(t == 0) {
                xil_printf("ECE3622 Lab 2\n");
                sleep(5);
                t = XGpio_DiscreteRead(&LEDSWSInst,SWITCHES);
            }
        }

        // Second Case
        else if (t == 2) {
            while(t == 2) {
                AD1_GetSample(&myDevice, &RawData); // Capture raw samples
                // Convert raw samples into floats scaled to 0 - VDD
                AD1_RawToPhysical(ReferenceVoltage, RawData, &PhysicalData);
                printf("Reading Output: %f\n",PhysicalData[0]);
                if(PhysicalData[0] <= 0.2) {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,0);
                }
                else if(0.2 < PhysicalData[0] && PhysicalData[0] <= 0.4) {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,1);
                }
                else if(0.4 < PhysicalData[0] && PhysicalData[0] <= 0.6) {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,3);
                }
                else if(0.6 < PhysicalData[0] && PhysicalData[0] <= 0.8) {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,7);
                }
                else if(0.8 < PhysicalData[0] ) {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,15);
                }
                else {
                    XGpio_DiscreteWrite(&LEDSWSInst,LEDS,0);
                }
                sleep(1);
                t = XGpio_DiscreteRead(&LEDSWSInst,SWITCHES);
            }
        }

        // Third Case
        else if (t == 4) {
                while(t == 4) {

                	// Fill window
                    for (int t = 0; t<WINDOWSIZE; t++) {
                        AD1_GetSample(&myDevice, &RawData); // Capture raw samples
                        // Convert raw samples into floats scaled to 0 - VDD
                        AD1_RawToPhysical(ReferenceVoltage, RawData, &PhysicalData);
                        window[t] =  PhysicalData[0];

                    }

                    // Sum of all values in window
                    for(int t = 0; t<WINDOWSIZE; t++) {
                        sum = window[t] + sum;
                    }

                    // Take the sum, remove the values to be removed (overlap),
                    // and divide by total number of samples in window
                    // Type-cast macro WINDOWSIZE because there's no explicit data type for macro
                    avg = (sum-sumOverlap) / (float) WINDOWSIZE;

                    sum = 0;
                    sumOverlap = 0;

                    // Summed Overlap, not the total sum, just the sum of values to be removed
                    // on next successive window array
                    for(int t = 0; t<OVERLAP; t++) {
                        sumOverlap = window[t] + sumOverlap;
                    }


                    printf("%f\n",avg); //output to terminal
                    sleep(1); //per lab
                    t = XGpio_DiscreteRead(&LEDSWSInst,SWITCHES); //read switches in case of change
                }
        }

    }

}

void ADInitialize() {
	//Initialize the PmodAD1 device - note that the AD1 IP is free-running
    AD1_begin(&myDevice, XPAR_PMODAD1_0_AXI_LITE_SAMPLE_BASEADDR);

    // Wait for AD1 to finish powering on, given in AD Technical Specifications
    usleep(1); // 1 us (minimum)
}





