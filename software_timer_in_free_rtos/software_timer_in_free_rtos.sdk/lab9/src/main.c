/*
 * main.c
 *
 *  Created on: Apr 17, 2019
 *      Author: bblouin
 */

// FreeRTOS includes
#include "FreeRTOS.h"
#include <task.h>
#include <timers.h>
#include <stdio.h>

// Xilinx includes
#include <xgpio.h>
#include <xil_printf.h>

// Preprocessor Definitions
#define TIMER_ID 1
#define BUTTONS 1
#define SWITCHES 2
#define FOURSEC 4000UL

static TickType_t four = pdMS_TO_TICKS(FOURSEC); // 4 seconds
static TaskHandle_t Task_BTN_SW = NULL; // Taskhandle object
static XGpio xBTNSWI, xLEDS; // gpio objects
static xTimerHandle xtimer1; // timer handle

// Function Prototypes
static void prvTask_BTN_SW(void *pvParameters); // Task
static void vTimer1Callback(TimerHandle_t pxTimer); // Timer
static void gpioInit(); //gpio initialization

int main (void){

	gpioInit();
	xTaskCreate(prvTask_BTN_SW,
			(const char *) "TASK_BTN_SW",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY + 4,
			&Task_BTN_SW);

	xtimer1 = xTimerCreate((const char *)"Timer1",
			four,
			pdFALSE,
			(void *) TIMER_ID,
			vTimer1Callback);

	if(xtimer1!=NULL){
			xTimerStart(xtimer1,0);
			vTaskStartScheduler();
	}


	while(1){};
}

// Task
static void prvTask_BTN_SW(void *pvParameters){
	while(1){
		if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==1&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)!=1){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,1);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==2&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)!=2){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,2);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==4&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)!=4){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,4);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==8&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)!=8){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,8);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==1&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)==1){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,14);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==2&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)==2){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,13);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==4&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)==4){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,11);
		}
		else if(XGpio_DiscreteRead(&xBTNSWI,BUTTONS)==8&&XGpio_DiscreteRead(&xBTNSWI,SWITCHES)==8){
			xTimerStart(xtimer1,0UL);
			XGpio_DiscreteWrite(&xLEDS,1,7);
		}
	}
}

// Timer
static void vTimer1Callback(xTimerHandle pxTimer){
	XGpio_DiscreteWrite(&xLEDS,1,0);
}

// Initialize GPIO
static void gpioInit(){
	XGpio_Initialize(&xBTNSWI,XPAR_AXI_GPIO_0_DEVICE_ID);
    XGpio_Initialize(&xLEDS,XPAR_AXI_GPIO_1_DEVICE_ID);
    XGpio_SetDataDirection(&xBTNSWI,1,0xf);
	XGpio_SetDataDirection(&xBTNSWI,2,0xf);
	XGpio_SetDataDirection(&xLEDS,1,0x0);
}
